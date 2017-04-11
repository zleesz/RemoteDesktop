#pragma once


#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002

#define ULW_COLORKEY            0x00000001
#define ULW_ALPHA               0x00000002
#define ULW_OPAQUE              0x00000004

#define WS_EX_LAYERED			0x00080000


#define NERR_BASE				2100
#define MAX_NERR                (NERR_BASE+899) /* This is the last error in NERR range. */


#define EMPTY_STRING			_T("")
#define PROFILE_APP_NAME		_T("Config")
#define countof(x)				(sizeof(x)/sizeof(x[0]))
#define MAX_URL_LENGTH			2048

#ifdef UNICODE
	#define lstrncpy	wcsncpy
	#define lstrstr		wcsstr
	#define lstrlwr		_wcslwr
	#define lstrupr		_wcsupr
	#define lstrncmp	wcsncmp
	#define lstricmp	_wcsicmp
	#define lstrnicmp	_wcsnicmp
	#define lsscanf		swscanf
	#define lstrtoi		_wtoi
	#define lstrtol		wcstol
	#define lstrtoul	wcstoul
	#define lstrtoull	_wcstoui64
	#define lstring		wstring
	#define fgetls		fgetws
	#define fopenl		_wfopen
#else
	#define lstrncpy	strncpy
	#define lstrstr		strstr
	#define lstrlwr		strlwr
	#define lstrupr		strupr
	#define lstrncmp	strncmp
	#define lstricmp	stricmp
	#define lstrnicmp	strnicmp
	#define lsscanf		sscanf
	#define lstrtoi		atoi
	#define lstrtol		strtol
	#define lstrtoul	strtoul
	#define lstrtoull	_strtoui64
	#define lstring		string
	#define fgetls		fgets
	#define fopenl		fopen
#endif


void			MillisecondToText(int nMs, TCHAR * pszText, BOOL bShowMillisecs = TRUE, BOOL bModeMinute = FALSE);
void			ShowDebug(const TCHAR * pcszFormat, ... );
BOOL			SelectFile(HWND hOwner, BOOL bIsSave, const TCHAR * pcszTitle, const TCHAR * pcszFilter, TCHAR * pszFilePath, int nMaxFilePath, TCHAR * pszFileTitle, int nMaxFileTitle);
BOOL			IsDirectoryExists(const TCHAR * pcszDirectory);
BOOL			IsFileExists(const TCHAR * pcszFileName);
BOOL			IsKeyDown(int nKeyCode);
BOOL			OpenDirectoryAndSelectFile(const TCHAR * pcszFileName);
BOOL			GetFileVersionString(const TCHAR * pcszFileName, TCHAR * pszVersionString);
BOOL			GetFileVersionStringA(const char* pcszFileName, char* pszVersionString);
int				GetFileBuildNumber(const TCHAR * pcszFileName);
BOOL			GetTargetPath(TCHAR * pszPath);
BOOL			GetUsersPublicDirectory(TCHAR * pszDirectory);
BOOL			GetPathFromFullName(const TCHAR * pcszFullName, TCHAR * pszPath);
BOOL			GetModulePath(HMODULE hModule, TCHAR * pszPath);
const TCHAR *	GetFileNameFromPath(const TCHAR * pcszFilePath);

void			GetErrorInformation(DWORD dwLastError, LPTSTR * ppMessageBuffer);


///////////////////////////////////////////////////////////
//	CTool
///////////////////////////////////////////////////////////
enum OPERATION_SYSTEM
{
	OS_OTHER		= 0,
	OS_WIN2K		= 1,
	OS_WINXP		= 2,
	OS_VISTAWIN78	= 3,
};

class CTool
{
public:
	CTool();
	~CTool();
	BOOL				Initialize(void);

	// System info
	const TCHAR *		GetExeFullName(void);
	const TCHAR *		GetExePath(void);
	const TCHAR *		GetExeName(void);
	const TCHAR *		GetSystemPath(void);
	const TCHAR *		GetTemporaryPath(void);
	OPERATION_SYSTEM	GetOS(void);
	BOOL				Is64BitSystem(void);
	int					GetIEVersion(void);
	DWORD				GetProcessId(void);
	BOOL				IsDebug(void);

	// Profile
	int					GetProfileInt(const TCHAR * pcszKey, int nDefault);
	BOOL				GetProfileStr(const TCHAR * pcszKey,
										const TCHAR * pcszDefault,
										TCHAR * pszValue,
										int nValueSize);

	// Log
	void				EnableLog(BOOL bEnable);
	BOOL				HaveLog(void);
	void				Log(const TCHAR * pcszFormat, ... );
	void				LogA(const char * pcszFormat, ...);
protected:
	void				GetVersionInfo(void);
	void				GetInitFile(TCHAR * pszIniFile);

protected:
	// Generic variables
	TCHAR				m_szExeFullName[MAX_PATH];
	TCHAR				m_szExePath[MAX_PATH];
	TCHAR				m_szExeName[MAX_PATH];
	TCHAR				m_szSystemPath[MAX_PATH];
	TCHAR				m_szTemporaryPath[MAX_PATH];
	OPERATION_SYSTEM	m_OperationSystem;
	BOOL				m_b64BitSystem;
	int					m_nIEVersion;
	DWORD				m_dwProcessId;

	// Log settings
	BOOL				m_bLogEnabled;
	TCHAR				m_szLogFile[MAX_PATH];
	HANDLE				m_hFile;
	BOOL				m_bDebugOutput;
	CRITICAL_SECTION	m_LogSection;
};
