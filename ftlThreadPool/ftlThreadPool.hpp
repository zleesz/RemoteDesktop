#ifndef FTL_THREADPOOL_HPP
#define FTL_THREADPOOL_HPP
#pragma once

#ifdef USE_EXPORT
#  include "ftlThreadPool.h"
#endif

namespace FTL
{
	///////////////////////////////////////////// CFJobBase ///////////////////////////////////////////////////
	template <typename T>
	CFJobBase<T>::CFJobBase()
		:m_JobParam(T())		//初始化
	{
		m_nJobPriority = 0;
		m_nJobIndex = 0;
		m_pThreadPool = NULL;
		m_hEventJobStop = NULL;
		//m_JobStatus = jsWaiting;
	}
	template <typename T>
	CFJobBase<T>::CFJobBase(T& t)
		:m_JobParam(t)
	{
		m_nJobPriority = 0;
		m_nJobIndex = 0;
		m_pThreadPool = NULL;
		m_hEventJobStop = NULL;
		//m_JobStatus = jsWaiting;
	}

	template <typename T>
	CFJobBase<T>::~CFJobBase()
	{
		FTLASSERT(NULL == m_hEventJobStop);
	}

	template <typename T>
	bool CFJobBase<T>::operator < (const CFJobBase<T> & other) const
	{
		COMPARE_MEM_LESS(m_nJobPriority, other);
		COMPARE_MEM_LESS(m_nJobIndex, other);
		return true;
	}
	
	template <typename T>
	LONG CFJobBase<T>::SetJobPriority(LONG nNewPriority)
	{
		LONG nOldPriority = m_nJobPriority;
		m_nJobPriority = nNewPriority;
		return nOldPriority;
	}

	template <typename T>
	LONG CFJobBase<T>::GetJobIndex() const
	{
		return m_nJobIndex;
	}
	
	template <typename T>
	BOOL CFJobBase<T>::RequestCancel()
	{
		BOOL bRet = FALSE;
		API_VERIFY(SetEvent(m_hEventJobStop));
		return bRet;
	}

	template <typename T>
	void CFJobBase<T>::_NotifyProgress(LONG64 nCurPos, LONG64 nTotalSize)
	{
		FTLASSERT(m_pThreadPool);
		m_pThreadPool->_NotifyJobProgress(this, nCurPos, nTotalSize);
	}

	template <typename T>
	void CFJobBase<T>::_NotifyCancel()
	{
		m_pThreadPool->_NotifyJobCancel(this);
	}

	template <typename T>
	void CFJobBase<T>::_NotifyError(DWORD dwError, LPCTSTR pszDescription)
	{
		m_pThreadPool->_NotifyJobError(this, dwError, pszDescription);
	}

	template <typename T>
	BOOL CFJobBase<T>::Initialize()
	{
		FTLASSERT(NULL == m_hEventJobStop);
		BOOL bRet = TRUE;
		return bRet;
	}

	template <typename T>
	FTLThreadWaitType CFJobBase<T>::GetJobWaitType(DWORD dwMilliseconds /* = INFINITE*/) const
	{
		FUNCTION_BLOCK_TRACE(0);
		FTLThreadWaitType waitType = ftwtError;
		HANDLE waitEvent[] = 
		{
			m_hEventJobStop,
			m_pThreadPool->m_hEventStop,
			m_pThreadPool->m_hEventContinue
		};
		DWORD dwResult = ::WaitForMultipleObjects(_countof(waitEvent), waitEvent, FALSE, dwMilliseconds);
		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			waitType = ftwtStop;		//Job Stop Event
			break;
		case WAIT_OBJECT_0 + 1:
			waitType = ftwtStop;		//Thread Pool Stop Event
			break;
		case WAIT_OBJECT_0 + 2:
			waitType = ftwtContinue;	//Thread Pool Continue Event
			break;
		case WAIT_TIMEOUT:
			waitType = ftwtTimeOut;
			break;
		default:
			FTLASSERT(FALSE);
			waitType = ftwtError;
			break;
		}
		return waitType;
	}

	//////////////////////////////////////    CFThreadPool    ///////////////////////////////////////////////////
	template <typename T>  
	CFThreadPool<T>::CFThreadPool(IFThreadPoolCallBack<T>* pCallBack /* = NULL*/)
		:m_pCallBack(pCallBack)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);

		m_nMinNumThreads = 0;
		m_nMaxNumThreads = 1;

		m_nRunningJobNumber = 0;
		m_nJobIndex = 0;
		m_nCurNumThreads = 0;
		m_nRunningThreadNum = 0;

		m_pJobThreadHandles = NULL;
		m_pJobThreadIds = NULL;

		m_hEventStop = CreateEvent(NULL, TRUE, FALSE, NULL);
		FTLASSERT(NULL != m_hEventStop);

		m_hEventAllThreadComplete = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		FTLASSERT(NULL != m_hEventAllThreadComplete);

		m_hEventContinue = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		FTLASSERT(NULL != m_hEventContinue);

		//将可以同时做的工作数设置为 MAXLONG -- 目前暂时不考虑队列中的个数
		m_hSemaphoreJobToDo = ::CreateSemaphore(NULL, 0, MAXLONG, NULL);
		FTLASSERT(NULL != m_hSemaphoreJobToDo);

		//创建调整线程个数的信号量
		m_hSemaphoreSubtractThread = CreateSemaphore(NULL, 0, MAXLONG, NULL);
		FTLASSERT(NULL != m_hSemaphoreSubtractThread);
	}

	template <typename T>  
	CFThreadPool<T>::~CFThreadPool(void)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;
		API_VERIFY(StopAndWait(FTL_MAX_THREAD_DEADLINE_CHECK));
		_DestroyPool();

		FTLASSERT(m_WaitingJobs.empty());
		FTLASSERT(m_DoingJobs.empty());
		FTLASSERT(0 == m_nRunningThreadNum);  //析构时所有的线程都要结束
	}

	template <typename T>  
	BOOL CFThreadPool<T>::Start(LONG nMinNumThreads, LONG nMaxNumThreads)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLASSERT( 0 <= nMinNumThreads );
		FTLASSERT( nMinNumThreads <= nMaxNumThreads );       
		FTLTRACE(TEXT("CFThreadPool::Start, ThreadNum is [%d-%d]\n"), nMinNumThreads, nMaxNumThreads);

		BOOL bRet = TRUE;
		m_nMinNumThreads = nMinNumThreads;
		m_nMaxNumThreads = nMaxNumThreads;

		API_VERIFY(ResetEvent(m_hEventStop));
		API_VERIFY(ResetEvent(m_hEventAllThreadComplete));
		API_VERIFY(SetEvent(m_hEventContinue));				//设置继续事件，保证各个工作线程能运行
		
		{
			CFAutoLock<CFLockObject>   locker(&m_lockThreads);
			FTLASSERT(NULL == m_pJobThreadHandles);
			if(NULL == m_pJobThreadHandles)    //防止多次调用Start
			{
				m_pJobThreadHandles = new HANDLE[m_nMaxNumThreads];  //分配m_nMaxNumThreads个线程的空间
				ZeroMemory(m_pJobThreadHandles,sizeof(HANDLE) * m_nMaxNumThreads);

				m_pJobThreadIds = new DWORD[m_nMaxNumThreads];
				ZeroMemory(m_pJobThreadIds,sizeof(DWORD) * m_nMaxNumThreads);

				_AddJobThread(m_nMinNumThreads);		//开始时只创建 m_nMinNumThreads 个线程
				FTLASSERT(m_nCurNumThreads == m_nMinNumThreads);
			}
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::Stop()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLTRACE(TEXT("CFThreadPool::Stop\n"));

		BOOL bRet = TRUE;
		API_VERIFY(SetEvent(m_hEventStop));
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::StopAndWait(DWORD dwTimeOut /* = FTL_MAX_THREAD_DEADLINE_CHECK */)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = TRUE;
		API_VERIFY(Stop());
		API_VERIFY(Wait(dwTimeOut));
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Wait(DWORD dwTimeOut /* = FTL_MAX_THREAD_DEADLINE_CHECK */)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		FTLTRACE(TEXT("CFThreadPool::Wait, dwTimeOut=%d\n"), dwTimeOut);

		BOOL bRet = TRUE;
		DWORD dwResult = WaitForSingleObject(m_hEventAllThreadComplete, dwTimeOut);
		switch (dwResult)
		{
		case WAIT_OBJECT_0: //所有的线程都结束了
			bRet = TRUE;
			break;
		case WAIT_TIMEOUT:
			FTLTRACE(TEXT("!!!CFThreadPool::Wait, Not all thread over in %d millisec\n"), dwTimeOut);
			FTLASSERT(FALSE);
			SetLastError(ERROR_TIMEOUT);
			bRet = FALSE;
			break;
		default:
			FTLASSERT(FALSE);
			bRet = FALSE;
			break;
		}

		{
			CFAutoLock<CFLockObject> locker(&m_lockThreads);
			for (LONG i = 0; i < m_nCurNumThreads; i++)
			{
				SAFE_CLOSE_HANDLE(m_pJobThreadHandles[i],NULL);
			}
			SAFE_DELETE_ARRAY(m_pJobThreadHandles);
			SAFE_DELETE_ARRAY(m_pJobThreadIds);
			m_nCurNumThreads = 0;
		}

		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Pause()
	{
		FTLTRACE(TEXT("CFThreadPool::Pause\n"));
		BOOL bRet = FALSE;
		API_VERIFY(::ResetEvent(m_hEventContinue));
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::Resume()
	{
		FTLTRACE(TEXT("CFThreadPool::Resume\n"));
		BOOL bRet = FALSE;
		API_VERIFY(::SetEvent(m_hEventContinue));
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::ClearUndoWork()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);

		BOOL bRet = TRUE;
		{
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			FTLTRACE(TEXT("CFThreadPool::ClearUndoWork, waitingJob Number is %d\n"), m_WaitingJobs.size());
			while (!m_WaitingJobs.empty())
			{
				//释放对应的信标对象，其个数和 m_WaitingJobs 的个数是一致的
				DWORD dwResult = WaitForSingleObject(m_hSemaphoreJobToDo, FTL_MAX_THREAD_DEADLINE_CHECK); 
				API_VERIFY(dwResult == WAIT_OBJECT_0);

				WaitingJobContainer::iterator iterBegin = m_WaitingJobs.begin();
				CFJobBase<T>* pJob = *iterBegin;
				FTLASSERT(pJob);
				_NotifyJobCancel(pJob);
				pJob->OnCancelJob();
				m_WaitingJobs.erase(iterBegin);
			}
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::_AddJobThread(LONG nThreadNum)
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = TRUE;
		{
			CFAutoLock<CFLockObject> locker(&m_lockThreads);
			if (m_nCurNumThreads + nThreadNum > m_nMaxNumThreads)
			{
				FTLASSERT(FALSE);
				//超过最大个数，不能再加了
				SetLastError(ERROR_INVALID_PARAMETER);
				bRet = FALSE;
			}
			else
			{
				unsigned int threadId = 0;
				for(LONG i = 0;i < nThreadNum; i++)
				{
					FTLASSERT(NULL == m_pJobThreadHandles[m_nCurNumThreads]);
					m_pJobThreadHandles[m_nCurNumThreads] = (HANDLE) _beginthreadex( NULL, 0, JobThreadProc, this, 0, &threadId);
					FTLASSERT(NULL != m_pJobThreadHandles[m_nCurNumThreads]);

					m_pJobThreadIds[m_nCurNumThreads] = threadId;
					m_nCurNumThreads++;

					FTLTRACE(TEXT("CFThreadPool::_AddJobThread, ThreadId=%d(0x%x), CurNumThreads=%d\n"),
						threadId, threadId, m_nCurNumThreads);
				}
				bRet = TRUE;
			}
		}
		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::SubmitJob(CFJobBase<T>* pJob, LONG* pOutJobIndex)
	{
		FTLASSERT(NULL != m_hEventStop); //如果调用 _DestroyPool后，就不能再次调用该函数
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;

		//加入Job并且唤醒一个等待线程
		{
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			m_nJobIndex++;
			pJob->m_pThreadPool = this;         //访问私有变量，并将自己赋值过去
			pJob->m_nJobIndex = m_nJobIndex;	//访问私有变量，设置JobIndex

			if (pOutJobIndex)
			{
				*pOutJobIndex = pJob->m_nJobIndex;
			}
			
			m_WaitingJobs.insert(pJob);
			API_VERIFY(ReleaseSemaphore(m_hSemaphoreJobToDo, 1L, NULL));
		}
		SwitchToThread();//唤醒等待的线程，使得其他线程可以获取Job -- 注意 CFAutoLock 的范围

		{
			//当所有的线程都在运行Job时，则需要增加线程  -- 不对 m_nRunningJobNumber 加保护(只是读取)
			//CFAutoLock<CFLockObject> locker(&m_lockThreads);
			FTLASSERT(m_nRunningJobNumber <= m_nCurNumThreads);
			BOOL bNeedMoreThread = (m_nRunningJobNumber == m_nCurNumThreads) && (m_nCurNumThreads < m_nMaxNumThreads); 
			if (bNeedMoreThread)
			{
				API_VERIFY(_AddJobThread(1L));      //每次增加一个线程
			}

			FTLTRACE(TEXT("CFThreadPool::SubmitJob, pJob[%d] = %p, m_nRunningJobNumber=%d, m_nCurNumThreads=%d, bNeedMoreThread=%d\n"),
				pJob->m_nJobIndex, pJob, m_nRunningJobNumber, m_nCurNumThreads, bNeedMoreThread);
		}

		return bRet;	
	}

	template <typename T>  
	BOOL CFThreadPool<T>::CancelJob(LONG nJobIndex)
	{
		FTLTRACE(TEXT("CFThreadPool::CancelJob, JobIndex=%d\n"), nJobIndex);

		if (nJobIndex <= 0 || nJobIndex > m_nJobIndex)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		BOOL bRet = TRUE;
		BOOL bFoundWaiting = FALSE;
		BOOL bFoundDoing = FALSE;
		{
			//首先查找未启动的任务 -- 因为传入的参数中没有Priority的信息，无法快速查找。
			//因此采用遍历的方式 -- 用 boost::multi_index(依赖太大)
			CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
			for (WaitingJobContainer::iterator iterWaiting = m_WaitingJobs.begin();
				iterWaiting != m_WaitingJobs.end();
				++iterWaiting)
			{
				if ((*iterWaiting)->GetJobIndex() == nJobIndex)
				{
					//找到,说明这个Job还没有启动
					bFoundWaiting = TRUE;

					DWORD dwResult = WaitForSingleObject(m_hSemaphoreJobToDo, INFINITE); //释放对应的信标对象，避免个数不匹配
					FTLASSERT(dwResult == WAIT_OBJECT_0);

					CFJobBase<T>* pJob = *iterWaiting;
					FTLASSERT(pJob);
					FTLASSERT(pJob->GetJobIndex() == nJobIndex);

					_NotifyJobCancel(pJob);
					//pJob->m_JobStatus = jsCancel;
					pJob->OnCancelJob();
					
					m_WaitingJobs.erase(iterWaiting);
					break;
				}
			}
		}

		if (!bFoundWaiting)
		{
			//查找正在运行的任务
			CFAutoLock<CFLockObject> locker(&m_lockDoingJobs);
			DoingJobContainer::iterator iterDoing = m_DoingJobs.find(nJobIndex);
			if (iterDoing != m_DoingJobs.end())
			{
				bFoundDoing = TRUE;

				CFJobBase<T>* pJob = iterDoing->second;
				FTLASSERT(pJob);
				FTLASSERT(pJob->GetJobIndex() == nJobIndex);
				
				//注意：这里只是请求Cancel，实际上任务是否能真正Cancel，需要依赖Job的实现，
				pJob->RequestCancel();
				//不要 m_DoingJobs.erase(iterDoing) -- Job 结束后会 erase
			}
		}
		if (!bFoundWaiting && !bFoundDoing)
		{
			//Waiting 和 Doing 中都没有找到，已经执行完毕
			//do nothing
		}

		return bRet;
	}

	template <typename T>  
	BOOL CFThreadPool<T>::HadRequestStop() const
	{
		_ASSERT(NULL != m_hEventStop);
		BOOL bRet = (WaitForSingleObject(m_hEventStop, 0) == WAIT_OBJECT_0);
		return bRet;
	}

	template <typename T>
	BOOL CFThreadPool<T>::HadRequestPause() const
	{
		BOOL bRet = (WaitForSingleObject(m_hEventContinue, 0) == WAIT_TIMEOUT);
		return bRet;
	}

	template <typename T>  
	void CFThreadPool<T>::_DestroyPool()
	{
		FUNCTION_BLOCK_TRACE(DEFAULT_BLOCK_TRACE_THRESHOLD);
		BOOL bRet = FALSE;
		API_VERIFY(ClearUndoWork());
		
		SAFE_CLOSE_HANDLE(m_hSemaphoreJobToDo,NULL);
		SAFE_CLOSE_HANDLE(m_hSemaphoreSubtractThread,NULL);
		SAFE_CLOSE_HANDLE(m_hEventAllThreadComplete, NULL);
		SAFE_CLOSE_HANDLE(m_hEventContinue,NULL);
		SAFE_CLOSE_HANDLE(m_hEventStop,NULL);
	}

	template <typename T>  
	GetJobType CFThreadPool<T>::_GetJob(CFJobBase<T>** ppJob)
	{
		FUNCTION_BLOCK_TRACE(0);
		HANDLE hWaitHandles[] = 
		{
			//TODO: 优先响应 m_hSemaphoreJobToDo 还是 m_hSemaphoreSubtractThread ?
			//  1.优先响应 m_hSemaphoreJobToDo 可以避免线程的波动
			//  2.优先响应 m_hSemaphoreSubtractThread 可以优先满足用户手动要求减少线程的需求(虽然目前尚未提供该接口)
			m_hEventStop,                 //user stop thread pool
			m_hSemaphoreJobToDo,          //there are waiting jobs
			m_hSemaphoreSubtractThread,   //need subtract thread
		};

		DWORD dwResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, INFINITE);
		switch(dwResult)
		{
		case WAIT_OBJECT_0:				//m_hEventStop
			return typeStop;
		case WAIT_OBJECT_0 + 1:			//m_hSemaphoreJobToDo
			break;
		case WAIT_OBJECT_0 + 2:			//m_hSemaphoreSubtractThread
			return typeSubtractThread;
		default:
			FTLASSERT(FALSE);
			return typeStop;
		}

		{
			//从等待容器中获取用户作业
			CFAutoLock<CFLockObject> lockerWating(&m_lockWaitingJobs);
			FTLASSERT(!m_WaitingJobs.empty());
			WaitingJobContainer::iterator iterBegin = m_WaitingJobs.begin();

			CFJobBase<T>* pJob = *iterBegin;
			FTLASSERT(pJob);

			*ppJob = pJob;
			m_WaitingJobs.erase(iterBegin);
			{
				//放到进行作业的容器中
				CFAutoLock<CFLockObject> lockerDoing(&m_lockDoingJobs);
				m_DoingJobs.insert(DoingJobContainer::value_type(pJob->GetJobIndex(), pJob));			
			}
		}
		return typeGetJob;	
	}

	template <typename T>  
	void CFThreadPool<T>::_DoJobs()
	{
		BOOL bRet = FALSE;
		FUNCTION_BLOCK_TRACE(0);
		CFJobBase<T>* pJob = NULL;
		GetJobType getJobType = typeStop;
		while(typeGetJob == (getJobType = _GetJob(&pJob)))
		{
			InterlockedIncrement(&m_nRunningJobNumber);
			INT nJobIndex = pJob->GetJobIndex();
			FTLTRACE(TEXT("CFThreadPool Begin Run Job %d\n"), nJobIndex);

			API_VERIFY(pJob->Initialize());
			if (bRet)
			{
				//这个地方的设计和实现不是很好，是否有更好的方法?
				FTLASSERT(NULL == pJob->m_hEventJobStop);
				pJob->m_hEventJobStop = CreateEvent(NULL, TRUE, FALSE, NULL);
				//pJob->m_JobStatus = jsDoing;

				_NotifyJobBegin(pJob);
				pJob->Run();
				_NotifyJobEnd(pJob);

				//if (jsDoing == pJob->m_JobStatus)
				//{
				//	pJob->m_JobStatus = jsDone;
				//}
				SAFE_CLOSE_HANDLE(pJob->m_hEventJobStop, NULL);
				pJob->Finalize();
			}
			InterlockedDecrement(&m_nRunningJobNumber);

			FTLTRACE(TEXT("CFThreadPool End Run Job %d\n"), nJobIndex);
			{
				//Job结束，首先从运行列表中删除
				CFAutoLock<CFLockObject> lockerDoing(&m_lockDoingJobs);
				DoingJobContainer::iterator iter = m_DoingJobs.find(nJobIndex);
				FTLASSERT(m_DoingJobs.end() != iter);
				if (m_DoingJobs.end() != iter)
				{
					m_DoingJobs.erase(iter);
				}
			}

			//检查一下是否需要减少线程
			BOOL bNeedSubtractThread = FALSE;
			{
				CFAutoLock<CFLockObject> locker(&m_lockWaitingJobs);
				//CFAutoLock<CFLockObject> locker(&m_lockThreads);
				//当队列中没有Job，并且当前线程数大于最小线程数时
				bNeedSubtractThread = (m_WaitingJobs.empty() && (m_nCurNumThreads > m_nMinNumThreads) && !HadRequestStop());
				if (bNeedSubtractThread)
				{
					//通知减少一个线程
					ReleaseSemaphore(m_hSemaphoreSubtractThread, 1L, NULL);
				}
			}
		}
		if (typeSubtractThread == getJobType)  //需要减少线程,应该把自己退出 -- 注意：通知退出的线程和实际退出的线程可能不是同一个
		{
			//FUNCTION_BLOCK_NAME_TRACE(TEXT("typeSubtractThread, will remove self thread"),
			//	DEFAULT_BLOCK_TRACE_THRESHOLD);
			CFAutoLock<CFLockObject> locker(&m_lockThreads);
			LONG index = 0;
			DWORD dwCurrentThreadId = GetCurrentThreadId();
			for (; index < m_nCurNumThreads; index++)
			{
				if (m_pJobThreadIds[index] == dwCurrentThreadId)  //找到自己线程对应的位置
				{
					break;
				}
			}
			FTLASSERT(index < m_nCurNumThreads);
			if (index < m_nCurNumThreads)
			{
				//把最后一个线程的信息移到退出的线程位置 -- 如果退出的线程就是最后一个时也正确
				HANDLE hOldTemp = m_pJobThreadHandles[index];
				m_pJobThreadHandles[index] = m_pJobThreadHandles[m_nCurNumThreads - 1];
				m_pJobThreadHandles[m_nCurNumThreads - 1] = NULL;

				m_pJobThreadIds[index] = m_pJobThreadIds[m_nCurNumThreads - 1];
				m_pJobThreadIds[m_nCurNumThreads - 1] = 0;

				m_nCurNumThreads--;
				CloseHandle(hOldTemp);

				FTLTRACE(TEXT("CFThreadPool Subtract a thread, thread id = %d(0x%x), curThreadNum = %d\n"),
					dwCurrentThreadId, dwCurrentThreadId, m_nCurNumThreads);
			}
		}
		else //typeStop
		{
			//Do Nothing
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobBegin(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobBegin(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobEnd(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobEnd(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobCancel(CFJobBase<T>* pJob)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobCancel(pJob->GetJobIndex(), pJob);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobProgress(CFJobBase<T>* pJob, LONG64 nCurPos, LONG64 nTotalSize)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobProgress(pJob->GetJobIndex(), pJob, nCurPos, nTotalSize);
		}
	}

	template <typename T>  
	void CFThreadPool<T>::_NotifyJobError(CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription)
	{
		FTLASSERT(pJob);
		if (pJob && m_pCallBack)
		{
			m_pCallBack->OnJobError(pJob->GetJobIndex(), pJob, dwError, pszDescription);
		}
	}

	template <typename T>  
	unsigned int CFThreadPool<T>::JobThreadProc(void *pThis)
	{
		FUNCTION_BLOCK_TRACE(0);
		CFThreadPool<T>* pThreadPool = (CFThreadPool<T>*)pThis;
		LONG nRunningNumber = InterlockedIncrement(&pThreadPool->m_nRunningThreadNum);

		pThreadPool->_DoJobs();

		nRunningNumber = InterlockedDecrement(&pThreadPool->m_nRunningThreadNum);
		if (0 == nRunningNumber)
		{
			//线程结束后判断是否是最后一个线程，如果是，激发事件
			SetEvent(pThreadPool->m_hEventAllThreadComplete);
		}
		return(0);
	}
}

#endif //FTL_THREADPOOL_HPP