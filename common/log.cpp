
#include "stdafx.h"
#include "Utils.h"
#include <shfolder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "version.lib")
#pragma comment(lib, "winmm.lib")

void MillisecondToText(int nMs, TCHAR * pszText, BOOL bShowMillisec /*=TRUE*/, BOOL bModeMinute /*=FALSE*/)
{
	int nSecond = nMs / 1000;
	int nMillisecond = nMs % 1000;

	if(!bModeMinute)
	{
		int nHour = nSecond / 3600;
		nSecond = nSecond % 3600;
		int nMinute = nSecond / 60;
		nSecond = nSecond % 60;

		if(bShowMillisec)
			_stprintf(pszText, _T("%02d:%02d:%02d.%03d"), nHour, nMinute, nSecond, nMillisecond);
		else
			_stprintf(pszText, _T("%02d:%02d:%02d"), nHour, nMinute, nSecond);
	}
	else
	{
		int nMinute = nSecond / 60;
		nSecond %= 60;
		
		if(bShowMillisec)
			_stprintf(pszText,TEXT("%d:%02d.%03d"),nMinute,nSecond,nMillisecond);
		else
			_stprintf(pszText,TEXT("%d:%02d"),nMinute,nSecond);
	}
}

void ShowDebug(const TCHAR * pcszFormat, ... )
{
	TCHAR szBuffer[1024];

	va_list vl;
	va_start(vl, pcszFormat);
	wvsprintf(szBuffer, pcszFormat, vl);
	va_end(vl);

	::OutputDebugString(szBuffer);
}

BOOL SelectFile(HWND hOwner,
				BOOL bIsSave,
				const TCHAR * pcszTitle,
				const TCHAR * pcszFilter,
				TCHAR * pszFilePath,
				int nMaxFilePath,
				TCHAR * pszFileTitle,
				int nMaxFileTitle)
{
	OPENFILENAME ofn;
	
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hOwner;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = pcszFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = pszFilePath;
	ofn.nMaxFile = nMaxFilePath;
	ofn.lpstrFileTitle = pszFileTitle;
	ofn.nMaxFileTitle = nMaxFileTitle;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = pcszTitle;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	BOOL bResult = 0;
	if(bIsSave)
		bResult = GetSaveFileName(&ofn);
	else
		bResult = GetOpenFileName(&ofn);
	return bResult;
}

BOOL IsDirectoryExists(const TCHAR * pcszDirectory)
{
	DWORD dwAttributes = GetFileAttributes(pcszDirectory);
	if(dwAttributes == 0xFFFFFFFF)
		return FALSE;
	if(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return TRUE;
	else
		return FALSE;
}

BOOL IsFileExists(const TCHAR * pcszFileName)
{
	DWORD dwAttributes = GetFileAttributes(pcszFileName);
	if(dwAttributes == 0xFFFFFFFF)
		return FALSE;
	if(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FALSE;
	return TRUE;
}

BOOL IsKeyDown(int nKeyCode)
{
	SHORT nResult = ::GetKeyState(nKeyCode);
	BOOL bResult = ((nResult & 0x8000) == 0x8000);
	return bResult;
}

BOOL OpenDirectoryAndSelectFile(const TCHAR * pcszFileName)
{
	TCHAR szParameter[1024];
	wsprintf(szParameter, _T("/n, /select, \"%s\""), pcszFileName);
	SHELLEXECUTEINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SEE_MASK_FLAG_NO_UI;
	si.hwnd = NULL;
	si.lpVerb = _T("open");
	si.lpFile = _T("explorer");
	si.lpParameters = szParameter;
	si.nShow = SW_SHOWNORMAL;

	BOOL bOK = ::ShellExecuteEx(&si);
	return bOK;
}

BOOL GetFileVersionString(const TCHAR * pcszFileName, TCHAR * pszVersionString)
{
	if(pcszFileName == NULL || pszVersionString == NULL)
		return FALSE;

	BOOL bResult = FALSE;
	DWORD dwHandle = 0;
	DWORD dwSize = ::GetFileVersionInfoSize(pcszFileName, &dwHandle);
	if(dwSize > 0)
	{
		TCHAR * pVersionInfo = new TCHAR[dwSize+1];
		if(::GetFileVersionInfo(pcszFileName, dwHandle, dwSize, pVersionInfo))
		{
			VS_FIXEDFILEINFO * pvi;
			UINT uLength = 0;
			if(::VerQueryValue(pVersionInfo, _T("\\"), (void **)&pvi, &uLength))
			{
				wsprintf(pszVersionString, _T("%d.%d.%d.%d"),
					HIWORD(pvi->dwFileVersionMS), LOWORD(pvi->dwFileVersionMS),
					HIWORD(pvi->dwFileVersionLS), LOWORD(pvi->dwFileVersionLS));
				bResult = TRUE;
			}
		}
		delete pVersionInfo;
	}
	return bResult;
}

BOOL GetFileVersionStringA(const char* pcszFileName, char* pszVersionString)
{
	if (pcszFileName == NULL || pszVersionString == NULL)
		return FALSE;

	BOOL bResult = FALSE;
	DWORD dwHandle = 0;
	DWORD dwSize = ::GetFileVersionInfoSizeA(pcszFileName, &dwHandle);
	if(dwSize > 0)
	{
		char* pVersionInfo = new char[dwSize+1];
		if(::GetFileVersionInfoA(pcszFileName, dwHandle, dwSize, pVersionInfo))
		{
			VS_FIXEDFILEINFO * pvi;
			UINT uLength = 0;
			if(::VerQueryValueA(pVersionInfo, "\\", (void **)&pvi, &uLength))
			{
				sprintf(pszVersionString, "%d.%d.%d.%d",
					HIWORD(pvi->dwFileVersionMS), LOWORD(pvi->dwFileVersionMS),
					HIWORD(pvi->dwFileVersionLS), LOWORD(pvi->dwFileVersionLS));
				bResult = TRUE;
			}
		}
		delete pVersionInfo;
	}
	return bResult;
}

int GetFileBuildNumber(const TCHAR * pcszFileName)
{
	if(pcszFileName == NULL)
		return 0;

	int nResult = 0;
	DWORD dwHandle = 0;
	DWORD dwSize = ::GetFileVersionInfoSize(pcszFileName, &dwHandle);
	if(dwSize > 0)
	{
		char * pVersionInfo = new char[dwSize+1];
		if(::GetFileVersionInfo(pcszFileName, dwHandle, dwSize, pVersionInfo))
		{
			VS_FIXEDFILEINFO * pvi;
			UINT uLength = 0;
			if(::VerQueryValue(pVersionInfo, _T("\\"), (void **)&pvi, &uLength))
			{
				nResult = LOWORD(pvi->dwFileVersionLS);
			}
		}
		delete pVersionInfo;
	}
	return nResult;
}

BOOL GetTargetPath(TCHAR * pszPath)
{
	BOOL bOK = FALSE;
	// <alluser>
	if(::GetUsersPublicDirectory(pszPath))
	{
		bOK = TRUE;
	}
	else
	{
		// <allappdata>
		HRESULT hr = ::SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, pszPath);
		if(SUCCEEDED(hr))
			bOK = TRUE;
	}
	if(!bOK)
		return FALSE;

	int nLength = lstrlen(pszPath);
	if(nLength <= 0)
		return FALSE;

	if(pszPath[nLength-1] != '\\')
		lstrcat(pszPath, _T("\\"));
	lstrcat(pszPath, _T("Thunder Network\\APlayer"));
	return TRUE;
}

BOOL GetUsersPublicDirectory(TCHAR * pszDirectory)
{
	BOOL bResult = FALSE;
	HMODULE hModule = ::LoadLibrary(_T("shell32.dll"));
	if(hModule != NULL)
	{
		typedef HRESULT (WINAPI *SHGetKnownFolderPath)(const GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
		SHGetKnownFolderPath pfn = (SHGetKnownFolderPath)::GetProcAddress(hModule, "SHGetKnownFolderPath");
		if(pfn != NULL)
		{
			PWSTR pwszPath = NULL;
			static const GUID FOLDERID_Public = {0xDFDF76A2, 0xC82A, 0x4D63, {0x90, 0x6A, 0x56, 0x44, 0xAC, 0x45, 0x73, 0x85} };
			HRESULT hr = pfn(FOLDERID_Public, 0, NULL, &pwszPath);
			if(SUCCEEDED(hr))
			{
				USES_CONVERSION;
				lstrcpy(pszDirectory, W2T(pwszPath));
				CoTaskMemFree(pwszPath);
				bResult = TRUE;
			}
		}
		::FreeLibrary(hModule);
	}
	return bResult;
}

BOOL GetPathFromFullName(const TCHAR * pcszFullName, TCHAR * pszPath)
{
	int nLength = lstrlen(pcszFullName);
	if(nLength >= MAX_PATH)
		return FALSE;
	lstrcpy(pszPath, pcszFullName);
	TCHAR * pEnd = pszPath + nLength - 1;
	while(pEnd > pszPath && *pEnd != '\\') pEnd--;
	*pEnd = '\0';
	return TRUE;
}

BOOL GetModulePath(HMODULE hModule, TCHAR * pszPath)
{
	if(::GetModuleFileName(hModule, pszPath, MAX_PATH) <= 0)
		return FALSE;

	TCHAR * pEnd = pszPath + lstrlen(pszPath) - 1;
	while(pEnd > pszPath && *pEnd != '\\') pEnd--;
	*pEnd = '\0';
	return TRUE;
}

const TCHAR * GetFileNameFromPath(const TCHAR * pcszFilePath)
{
	const TCHAR * p = pcszFilePath + lstrlen(pcszFilePath) - 1;
	while(p > pcszFilePath && *p != '\\') p--;
	p++;
	return p;
}

void GetErrorInformation(DWORD dwLastError, LPTSTR * ppMessageBuffer)
{
	HMODULE hModule = NULL; // default to system source
	DWORD dwBufferLength;

	DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_FROM_SYSTEM ;

	//
	// If dwLastError is in the network range, 
	//  load the message source.
	//

	if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) 
	{
		hModule = LoadLibraryEx(TEXT("netmsg.dll"),
			NULL,
			LOAD_LIBRARY_AS_DATAFILE);

		if(hModule != NULL)
			dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
	}

	//
	// Call FormatMessage() to allow for message 
	//  text to be acquired from the system 
	//  or from the supplied module handle.
	//
	dwBufferLength = FormatMessage(dwFormatFlags,
		hModule, // module to get message from (NULL == system)
		dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
		(LPWSTR) &ppMessageBuffer,
		0,
		NULL);
	if(dwBufferLength)
	{
		//
		// Output message string.
		//
		//ShowDebug(_T("Error code = %d, MSG = %s"), dwLastError, MessageBuffer);

		//
		// Free the buffer allocated by the system.
		//
		//LocalFree(pMessageBuffer);
	}

	//
	// If we loaded a message source, unload it.
	//
	if(hModule != NULL)
		FreeLibrary(hModule);
}

///////////////////////////////////////////////////////////
//	CTool
///////////////////////////////////////////////////////////
CTool tool;

CTool::CTool()
{
	// Path
	lstrcpy(m_szExeFullName, EMPTY_STRING);
	lstrcpy(m_szExePath, EMPTY_STRING);
	lstrcpy(m_szExeName, EMPTY_STRING);
	lstrcpy(m_szSystemPath, EMPTY_STRING);
	lstrcpy(m_szTemporaryPath, EMPTY_STRING);

	m_OperationSystem = OS_OTHER;
	m_b64BitSystem = FALSE;
	m_nIEVersion = 0;

	// Log
	m_bLogEnabled = FALSE;
	memset(m_szLogFile, 0, sizeof(m_szLogFile));
	m_hFile = INVALID_HANDLE_VALUE;
	m_bDebugOutput = FALSE;
	::InitializeCriticalSection(&m_LogSection);
}


CTool::~CTool()
{
	::DeleteCriticalSection(&m_LogSection);
}


BOOL CTool::Initialize(void)
{
	/////////////////////////////////////////////
	// Get process id and all path
	/////////////////////////////////////////////

	// Get process id
	m_dwProcessId = ::GetCurrentProcessId();

	// Get EXE fullname/path/name
	if(!::GetModuleFileName(NULL, m_szExeFullName, countof(m_szExeFullName)))
		return FALSE;
	if(!::GetPathFromFullName(m_szExeFullName, m_szExePath))
		return FALSE;
	lstrcpy(m_szExeName, ::GetFileNameFromPath(m_szExeFullName));

	// Get system path
	UINT uResult = ::GetSystemDirectory(m_szSystemPath, countof(m_szSystemPath));
	if(uResult == 0)
		return FALSE;
	int nLength = lstrlen(m_szSystemPath);
	if(m_szSystemPath[nLength - 1] == '\\')
		m_szSystemPath[nLength - 1] = '\0';

	// Get temporary path
	uResult = ::GetTempPath(countof(m_szTemporaryPath), m_szTemporaryPath);
	if(uResult == 0)
		return FALSE;
	nLength = lstrlen(m_szTemporaryPath);
	if(m_szTemporaryPath[nLength - 1] == '\\')
		m_szTemporaryPath[nLength - 1] = '\0';

	lstrcat(m_szTemporaryPath, _T("\\Low"));
	if(!::IsDirectoryExists(m_szTemporaryPath))
		::CreateDirectory(m_szTemporaryPath, NULL);

	/////////////////////////////////////////////
	// Load & Apply profile
	/////////////////////////////////////////////

	// Log filename
	TCHAR szLogPrefix[MAX_PATH] = {0};
	lstrcpy(szLogPrefix, m_szExeName);
	::PathRemoveExtension(szLogPrefix);

	TCHAR szFileName[MAX_PATH];
	if(this->GetProfileInt(_T("FixLogFile"), 0))
		wsprintf(szFileName, _T("%s.txt"), szLogPrefix);
	else
		wsprintf(szFileName, _T("%s_%u.txt"), szLogPrefix, m_dwProcessId);
	wsprintf(m_szLogFile, _T("%s\\%s"), m_szTemporaryPath, szFileName); // Must set here for dynamic enable log

	// Log enable
	if(this->GetProfileInt(_T("Log"), 0))
	{
		this->EnableLog(TRUE);
	}
	else
	{
		// Support TSLOG
		const TCHAR * pcszTsLogIniFile = _T("C:\\TSLOG_Config\\TSLOG.ini");
		if(::IsFileExists(pcszTsLogIniFile))
		{
			// Check TSLOG enable
			TCHAR szOutput[20];
			::GetPrivateProfileString(_T("Output"), _T("FileLog"), _T("OFF"),
				szOutput, sizeof(szOutput)/sizeof(TCHAR), pcszTsLogIniFile);
			if(lstricmp(szOutput, _T("ON")) == 0)
			{
				// Get log filepath
				TCHAR szLogPath[MAX_PATH];
				::GetPrivateProfileString(_T("FileLogOption"), _T("LogFilePath"), _T(""),
					szLogPath, sizeof(szLogPath)/sizeof(TCHAR), pcszTsLogIniFile);
				int nLength = lstrlen(szLogPath);
				if(nLength > 0)
				{
					if(szLogPath[nLength - 1] == '\\')
						szLogPath[nLength - 1] = '\0';

					// Get log filename
					wsprintf(m_szLogFile, _T("%s\\%s"), szLogPath, szFileName);

					// Enable log
					this->EnableLog(TRUE);
				}
			}
		}
	}

	/////////////////////////////////////////////
	// Other initialize
	/////////////////////////////////////////////
	this->GetVersionInfo();
	return TRUE;
}

const TCHAR * CTool::GetExeFullName(void) { return m_szExeFullName; }
const TCHAR * CTool::GetExePath(void) { return m_szExePath; }
const TCHAR * CTool::GetExeName(void) { return m_szExeName; }
const TCHAR * CTool::GetSystemPath(void) { return m_szSystemPath; }
const TCHAR * CTool::GetTemporaryPath(void) { return m_szTemporaryPath; }


OPERATION_SYSTEM CTool::GetOS(void)
{
	return m_OperationSystem;
}

BOOL CTool::Is64BitSystem(void)
{
	return m_b64BitSystem;
}

int CTool::GetIEVersion(void)
{
	return m_nIEVersion;
}


DWORD CTool::GetProcessId(void)
{
	return m_dwProcessId;
}


BOOL CTool::IsDebug(void)
{
#ifdef DEBUG
	return TRUE;
#else
	return FALSE;
#endif // DEBUG
}


int CTool::GetProfileInt(const TCHAR * pcszKey, int nDefault)
{
	TCHAR szIniFile[MAX_PATH];
	this->GetInitFile(szIniFile);
	int nResult = ::GetPrivateProfileInt(PROFILE_APP_NAME, pcszKey, nDefault, szIniFile);
	return nResult;
}


BOOL CTool::GetProfileStr(const TCHAR * pcszKey,
						  const TCHAR * pcszDefault,
						  TCHAR * pszValue,
						  int nValueSize)
{
	TCHAR szIniFile[MAX_PATH];
	this->GetInitFile(szIniFile);
	::GetPrivateProfileString(PROFILE_APP_NAME, pcszKey, pcszDefault, pszValue, nValueSize, szIniFile);
	return TRUE;
}


void CTool::EnableLog(BOOL bEnable)
{
	if(m_bLogEnabled == bEnable)
		return;

	if(bEnable)
	{
		m_hFile = ::CreateFile(m_szLogFile, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, 0, NULL);
		if(m_hFile != INVALID_HANDLE_VALUE)
		{
			// Write unicode file header
			WORD dwHeader = 0xFEFF;
			DWORD dwWriteLen = 0;
			::WriteFile(m_hFile, &dwHeader, sizeof(dwHeader), &dwWriteLen, NULL);
		}
		m_bLogEnabled = TRUE;

		// Get debug output config
		m_bDebugOutput = this->GetProfileInt(_T("DebugOutput"), 1);
	}
	else
	{
		m_bLogEnabled = FALSE;
		if(m_hFile != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
	}
}


void CTool::Log(const TCHAR * pcszFormat, ...)
{
	if(!m_bLogEnabled)
		return;

	// Generic log info
	TCHAR szLog[MAX_URL_LENGTH + 512];
	va_list vl;
	va_start(vl, pcszFormat);
	wvsprintf(szLog, pcszFormat, vl);
	va_end(vl);

	// Generic log text
	TCHAR szText[MAX_URL_LENGTH + 1024];
	SYSTEMTIME st;
	::GetLocalTime(&st);

	TCHAR szLogPrefix[MAX_PATH] = {0};
	lstrcpy(szLogPrefix, m_szExeName);
	::PathRemoveExtension(szLogPrefix);

	::wsprintf(szText, _T("%s[%02d:%02d:%02d.%03d][%04X]: %s\r\n"),
		szLogPrefix, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ::GetCurrentThreadId(), szLog);

	// Output
	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		::EnterCriticalSection(&m_LogSection);
		DWORD dwWriteLen = 0;
		::WriteFile(m_hFile, szText, lstrlen(szText) * sizeof(TCHAR), &dwWriteLen, NULL);
		::LeaveCriticalSection(&m_LogSection);
	}
	if(m_bDebugOutput)
		::OutputDebugString(szText);
}

void CTool::LogA(const char * pcszFormat, ...)
{
	if(!m_bLogEnabled)
		return;

	// Generic log info
	char szLog[MAX_URL_LENGTH + 512];
	va_list vl;
	va_start(vl, pcszFormat);
	vsprintf(szLog, pcszFormat, vl);
	va_end(vl);

	TCHAR tszLog[MAX_URL_LENGTH + 512] = {0};
	MultiByteToWideChar(936, NULL, szLog, strlen(szLog), tszLog, MAX_URL_LENGTH + 512);

	// Generic log text
	TCHAR szText[MAX_URL_LENGTH + 1024];
	SYSTEMTIME st;
	::GetLocalTime(&st);

	TCHAR szLogPrefix[MAX_PATH] = {0};
	lstrcpy(szLogPrefix, m_szExeName);
	::PathRemoveExtension(szLogPrefix);

	::wsprintf(szText, _T("%s[%02d:%02d:%02d.%03d][%04X]: %s\r\n"),
		szLogPrefix, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ::GetCurrentThreadId(), tszLog);

	// Output
	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		::EnterCriticalSection(&m_LogSection);
		DWORD dwWriteLen = 0;
		::WriteFile(m_hFile, szText, _tcslen(szText) * sizeof(TCHAR), &dwWriteLen, NULL);
		::LeaveCriticalSection(&m_LogSection);
	}
	if(m_bDebugOutput)
		::OutputDebugString(szText);
}

BOOL CTool::HaveLog(void)
{
	return m_bLogEnabled;
}


void CTool::GetVersionInfo(void)
{
	// Show date
	SYSTEMTIME st;
	::GetLocalTime(&st);
	tool.Log(_T("Date: %04d-%02d-%02d"), st.wYear, st.wMonth, st.wDay);

	// Get OS Version
	OSVERSIONINFO osvi;
	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if(::GetVersionEx(&osvi))
	{
		const TCHAR * pcszOS = EMPTY_STRING;
		if(osvi.dwMajorVersion == 5)
		{
			if(osvi.dwMinorVersion == 0)
			{
				pcszOS = _T("OS: Windows 2000");
				m_OperationSystem = OS_WIN2K;
			}
			else
			{
				pcszOS = _T("OS: Windows XP");
				m_OperationSystem = OS_WINXP;
			}
		}
		else if(osvi.dwMajorVersion >= 6)
		{
			pcszOS = _T("OS: Windows Vista or Windows 7");
			m_OperationSystem = OS_VISTAWIN78;
		}
		else
		{
			pcszOS = _T("OS: Other Windows (Windows31, Windows9X, WindowsNT");
			m_OperationSystem = OS_OTHER;
		}

		this->Log(_T("%s (%d.%d.%d)"),
			pcszOS, osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	}
	else
		this->Log(_T("GetVersionEx failed, error = %d"), ::GetLastError());

	// Get is 64 bit
	m_b64BitSystem = FALSE;
	HMODULE hKernel32 = ::GetModuleHandle(_T("kernel32.dll"));
	if(hKernel32 != NULL)
	{
		typedef UINT (WINAPI *TypeGetSystemWow64Directory)(LPTSTR lpBuffer, UINT uSize);
		const char * pName = NULL;
#ifdef UNICODE
		pName = "GetSystemWow64DirectoryW";
#else
		pName = "GetSystemWow64DirectoryA";
#endif // UNICODE
		TypeGetSystemWow64Directory pfn = (TypeGetSystemWow64Directory)::GetProcAddress(hKernel32, pName);
		if(pfn != NULL)
		{
			TCHAR szPath[MAX_PATH];
			UINT uCount = pfn(szPath, countof(szPath));
			if(uCount > 0)
				m_b64BitSystem = TRUE;
		}
	}
	this->Log(_T("64-bit operating system = %d"), m_b64BitSystem);

	// Get IE Version
	TCHAR szTemp[MAX_PATH];
	lstrcpy(szTemp, _T("Unknown"));
	HKEY hKey = NULL;
	LONG lResult = ::RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Internet Explorer"), &hKey);
	if(lResult == ERROR_SUCCESS)
	{
		DWORD dwType = REG_SZ;
		DWORD dwLength = sizeof(szTemp);
		lResult = ::RegQueryValueEx(hKey, _T("Version"), NULL, &dwType, (LPBYTE)szTemp, &dwLength);
		if(lResult == ERROR_SUCCESS)
			m_nIEVersion = lstrtoi(szTemp);
		::RegCloseKey(hKey);
	}
	this->Log(_T("IE version from registry: (%s)"), szTemp);

	// Get exe module name
	if(GetFileVersionString(m_szExeFullName, szTemp))
		this->Log(_T("Application: %s (%s)"), m_szExeFullName, szTemp);
	else
		this->Log(_T("Get application version failed: %s"), m_szExeFullName);

	// Debug output info
	this->Log(_T("DebugOutput = %d"), m_bDebugOutput);
}


void CTool::GetInitFile(TCHAR * pszIniFile)
{
	wsprintf(pszIniFile, _T("%s\\..\\%s"), m_szExePath, _T("APlayer.ini"));
	if(!::IsFileExists(pszIniFile))
		wsprintf(pszIniFile, _T("%s\\..\\%s"), m_szExePath, _T("APlayer.txt"));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     