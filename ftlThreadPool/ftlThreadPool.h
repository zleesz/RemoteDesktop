///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlthread.h
/// @brief  Functional Template Library Base Header File.
/// @author fujie
/// @version 0.6 
/// @date 03/30/2008
/// @defgroup ftlThreadPool ftl thread pool function and class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_THREADPOOL_H
#define FTL_THREADPOOL_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlThreadPool.h requires ftlbase.h to be included first
#endif

#include "ftlThread.h"
#include <set>
#include <map>
//#include "ftlSharePtr.h"
//#include "ftlFunctional.h"

namespace FTL
{
	//前向声明
	//! 具有模板参数的线程池类，除能可以方便的进行参数传递外，还拥有以下特点：
	//  1.能自动根据任务和线程的多少在 最小/最大 线程个数之间调整
	//  2.能方便的对单个任务进行取消，如任务尚未运行则由框架代码处理，如任务已经运行，则需要 JobBase 的子类根据 GetJobWaitType 的返回值进行处理
	//  3.能对整个线程池进行 暂停、继续、停止 处理 -- 需要 JobBase 的子类根据 GetJobWaitType 的返回值进行处理
	//  4.支持回调方式的反馈通知( Progress/Error 等)
	//  5.使用的是微软的基本API，能支持WinXP、Vista、Win7等各种操作系统
	template <typename T> class CFThreadPool;  

	//enum FJobStatus
	//{
	//	jsWaiting,
	//	jsDoing,
	//	jsCancel,	//不会主动设置为cancel, 如果Job子类支持Cancel的话，自行在 GetJobWaitType 调用后处理并设置
	//	jsDone,
	//	jsError,	
	//};

	//! 如果想实现多个具有不同参数类型的Job，可以将模板设为 DWORD_PTR 等可以转换为指针的类型即可
	template <typename T>
	class CFJobBase
	{
		friend class CFThreadPool<T>;   //允许Threadpool设置 m_pThreadPool/m_nJobIndex 的值
	public:
		FTLINLINE CFJobBase();
		FTLINLINE CFJobBase(T& t);
		FTLINLINE virtual ~CFJobBase();

		//! 比较Job的大小，用于确定在 Waiting 容器中的队列， 排序依据为 Priority -> Index
		bool operator < (const CFJobBase & other) const;

		//! 获取或设置Job的优先级, 数字越小，优先级越高(在等待队列中拍在越前面)，缺省值是 0
		//  注意：优先级必须在放入 Pool 前设置，放入后就不能再调整了
		FTLINLINE LONG GetJobPriority() const { return m_nJobPriority; }
		FTLINLINE LONG SetJobPriority(LONG nNewPriority);

		FTLINLINE LONG GetJobIndex() const;
		//FTLINLINE FJobStatus GetJobStatus() const;

		T		m_JobParam;			//! Job会使用的参数，此处为了简化，直接采用公有变量的方式
		//FTLINLINE T& GetJobParam();
		//FTLINLINE const T& GetJobParam() const;

		//如果Job正在运行过程中被取消，会调用这个方法
		FTLINLINE BOOL RequestCancel();
	protected:
		//这个三个函数一组, 用于运行起来的Job： if( Initialize ){ Run -> Finalize }
		virtual BOOL Initialize();
		// 在这个Run中通常需要循环 调用 GetJobWaitType 方法检测
		virtual BOOL Run() = 0;
		//! 如果是new出来的，通常需要在 Finalize 中调用 delete this (除非又有另外的生存期管理容器)
		virtual VOID Finalize() = 0;

		//这个函数用于未运行的Job(直接取消或线程池停止), 用于清除内存等资源, 如 delete this 等
		virtual VOID OnCancelJob() = 0;
	protected:
		FTLINLINE void _NotifyProgress(LONG64 nCurPos, LONG64 nTotalSize);
		FTLINLINE void _NotifyError(DWORD dwError, LPCTSTR pszDescription);
		FTLINLINE void _NotifyCancel();

		//! 通过该函数，获取线程池的状态(Stop/Pause)，以及Job自己的Stop, 用法同 CFThread:GetThreadWaitType:
		//! 如果想支持暂停，参数是 INFINITE；如不想支持暂停(如网络传输)，则参数传 0
		FTLINLINE FTLThreadWaitType GetJobWaitType(DWORD dwMilliseconds = INFINITE) const;
	private:
		//设置为私有的变量和方法，即使是子类也不要直接更改，由Pool调用进行控制
		LONG		m_nJobPriority;
		LONG		m_nJobIndex;
		HANDLE		m_hEventJobStop;					//停止Job的事件，该变量将由Pool创建和释放(TODO:Pool中缓存?)
		//FJobStatus	m_JobStatus;
		CFThreadPool<T>* m_pThreadPool;
	};

	typedef enum tagGetJobType
	{
		typeStop,
		typeSubtractThread,
		typeGetJob,
		typeError,		//发生未知错误(目前尚不清楚什么情况下会发生)
	}GetJobType;

	//回调函数 -- 通过 pJob->m_JobParam 可以访问类型为 T 的 参数
	FTLEXPORT template <typename T>
	class IFThreadPoolCallBack
	{
	public:
		//当Job运行起来以后，会由 Pool 激发 Begin 和 End 两个函数
		FTLINLINE virtual void OnJobBegin(LONG nJobIndex, CFJobBase<T>* pJob )
		{
		} 
		FTLINLINE virtual void OnJobEnd(LONG nJobIndex, CFJobBase<T>* pJob)
		{
		}

		//如果尚未到达运行状态就被取消的Job，会由Pool调用这个函数
		FTLINLINE virtual void OnJobCancel(LONG nJobIndex, CFJobBase<T>* pJob)
		{
		}

		//Progress 和 Error 由 JobBase 的子类激发
		FTLINLINE virtual void OnJobProgress(LONG nJobIndex , CFJobBase<T>* pJob, LONG64 nCurPos, LONG64 nTotalSize)
		{
		}
		FTLINLINE virtual void OnJobError(LONG nJobIndex , CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription)
		{
		}
	};

	FTLEXPORT template <typename T>  
	class CFThreadPool
	{
		//typedef CFSharePtr<CFJobBase< T> > CFJobBasePtr;
		//friend class CFJobBasePtr;
		friend class CFJobBase<T>;  //允许Job在 GetJobWaitType 中获取 m_hEventStop/m_hEventContinue
	public:
		FTLINLINE CFThreadPool(IFThreadPoolCallBack<T>* pCallBack = NULL);
		FTLINLINE virtual ~CFThreadPool(void);

		//! 开始线程池,此时会创建 nMinNumThreads 个线程，然后会根据任务数在 nMinNumThreads -- nMaxNumThreads 之间自行调节线程的个数
		FTLINLINE BOOL Start(LONG nMinNumThreads, LONG nMaxNumThreads);

		//! 请求停止线程池
		//! 注意：
		//!   1.只是设置StopEvent，需要Job根据GetJobWaitType处理 
		//!   2.不会清除当前注册的但尚未进行的工作，如果需要删除，需要调用ClearUndoWork
		FTLINLINE BOOL Stop();

		FTLINLINE BOOL StopAndWait(DWORD dwTimeOut = FTL_MAX_THREAD_DEADLINE_CHECK);

		//! 等待所有线程都结束并释放Start中分配的线程资源
		FTLINLINE BOOL Wait(DWORD dwTimeOut = FTL_MAX_THREAD_DEADLINE_CHECK);

		//! 清除当前未完成的工作，
		FTLINLINE BOOL ClearUndoWork();

		//! 向线程池中注册工作 -- 如果当前没有空闲的线程，并且当前线程数小于最大线程数，则会自动创建新的线程，
		//! 成功后会通过 outJobIndex 返回Job的索引号，可通过该索引定位、取消特定的Job
		FTLINLINE BOOL SubmitJob(CFJobBase<T>* pJob, LONG* pOutJobIndex);

		//! 取消指定的Job,
		//! TODO:如果取出Job给客户，可能调用者得到指针时，Job执行完毕 delete this，会照成野指针异常
		FTLINLINE BOOL CancelJob(LONG nJobIndex);

		//FTLINLINE BOOL PauseJob(LONG nJobIndex);
		//FTLINLINE BOOL ResumeJob(LONG nJobIndex);

		//! 请求暂停线程池的操作
		FTLINLINE BOOL Pause();

		//! 请求继续线程池的操作
		FTLINLINE BOOL Resume();

		//! 是否已经请求了暂停线程池
		FTLINLINE BOOL HadRequestPause() const;

		//! 是否已经请求了停止线程池
		FTLINLINE BOOL HadRequestStop() const;
	protected:
		//! 增加运行的线程,如果 当前线程数 + nThreadNum <= m_nMaxNumThreads 时 会成功执行
		FTLINLINE BOOL _AddJobThread(LONG nThreadNum);
		FTLINLINE void _DestroyPool();
		FTLINLINE void _DoJobs();

		FTLINLINE GetJobType _GetJob(CFJobBase<T>** ppJob);

		FTLINLINE void _NotifyJobBegin(CFJobBase<T>* pJob);
		FTLINLINE void _NotifyJobEnd(CFJobBase<T>* pJob);
		FTLINLINE void _NotifyJobCancel(CFJobBase<T>* pJob);

		FTLINLINE void _NotifyJobProgress(CFJobBase<T>* pJob, LONG64 nCurPos, LONG64 nTotalSize);
		FTLINLINE void _NotifyJobError(CFJobBase<T>* pJob, DWORD dwError, LPCTSTR pszDescription); 

	protected:
		LONG m_nMinNumThreads;					//! 线程池中最少的线程个数
		LONG m_nMaxNumThreads;					//! 线程池中最大的线程个数
		IFThreadPoolCallBack<T>* m_pCallBack;	//! 回调接口
		LONG m_nJobIndex;						//! Job的索引，每 SubmitJob 一次，则递增1

		LONG m_nRunningJobNumber;				//! 当前正在运行的Job个数

		//TODO: 两个最好统一？
		LONG m_nCurNumThreads;                  //! 当前的线程个数(主要用来维护 m_pJobThreadHandles 数组)
		LONG m_nRunningThreadNum;				//! 当前运行着的线程个数(用来在所有的线程结束时激发 Complete 事件)

		HANDLE* m_pJobThreadHandles;            //! 保存线程句柄的数组
		DWORD*  m_pJobThreadIds;                //! 保存线程 Id 的数组(为了在线程结束后调整数组中的位置)

		//HANDLE	m_hMgrThread;					//! Pool管理线程的句柄

		//! 保存等待Job的信息，由于有优先级的问题，而且一般是从最前面开始取，因此保存成 set，
		//! 保证优先级高、JobIndex小(同优先级时FIFO) 的Job在最前面
		typedef typename UnreferenceLess< CFJobBase<T> * >	JobBaseUnreferenceLess;
		typedef std::set<CFJobBase<T>*, JobBaseUnreferenceLess > WaitingJobContainer;
		WaitingJobContainer		m_WaitingJobs;	//! 等待运行的Job

		//! 保存运行Job的信息， 由于会频繁加入、删除，且需要按照JobIndex查找，因此保存成 map
		typedef std::map<LONG, CFJobBase<T>* >	DoingJobContainer;
		DoingJobContainer		m_DoingJobs;	//! 正在运行的Job

		HANDLE m_hEventStop;                    //! 停止Pool的事件
		HANDLE m_hEventAllThreadComplete;		//! 所有的线程都结束时激发这个事件
		HANDLE m_hEventContinue;				//! 整个Pool继续运行的事件
		HANDLE m_hSemaphoreJobToDo;             //! 保存还有多少个Job的信号量,每Submit一个Job,就增加一个
		HANDLE m_hSemaphoreSubtractThread;      //! 用于减少线程个数时的信号量,初始时个数为0,每要释放一个，就增加一个，

		CFCriticalSection m_lockDoingJobs;		//访问 m_DoingJobs 时互斥
		CFCriticalSection m_lockWaitingJobs;    //访问 m_WaitingJobs 时互斥
		CFCriticalSection m_lockThreads;        //访问 m_pJobThreadHandles/m_pJobThreadIds 时互斥

		static unsigned int CALLBACK JobThreadProc(void *pParam);    //! 工作线程的执行函数
		//static unsigned int CALLBACK MgrThreadProc(void* pParam);	 //! 管理线程的执行函数
	};
}

#endif //FTL_THREADPOOL_H

#ifndef USE_EXPORT
#  include "ftlThreadPool.hpp"
#endif