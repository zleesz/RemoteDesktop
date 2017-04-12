#include "stdafx.h"
#include "MainWindow.h"
#include "event2\thread.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
	int nRet = 0;
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	CHECKMEMORY();
	// 线程安全
	evthread_use_windows_threads();

	DWORD dwTimeStart = ::timeGetTime();
	tool.Initialize();
	DWORD dwTime = ::timeGetTime() - dwTimeStart;
	tool.Log(_T("It takes %d ms for tool initialization."), dwTime);
	tool.Log(_T(""));

	HRESULT hRes = ::CoInitialize(NULL);

	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2) , &wsaData);
#endif

	HWND hDesktopWnd = ::GetDesktopWindow();
	CRect rcDesktop;
	::GetWindowRect(hDesktopWnd, &rcDesktop);

	CMainWindow MainWindow;
	CRect rcBound(100, 100, 1000, 600);
 	MainWindow.Create(rcBound, SW_SHOW);
 	MainWindow.SetWindowText(L"P2PTracker服务器");

	int nRet = Run(lpstrCmdLine, nCmdShow);

	tool.Log(_T("Run ret:%d"), nRet);
	int r = WSACleanup();
	tool.Log(_T("WSACleanup ret:%d"), r);

	_Module.Term();
	::CoUninitialize();
	return nRet;
}
