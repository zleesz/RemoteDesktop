#pragma once
#include <string>

#define RD_PROTOCAL_VERSION "1.0"
#define RD_REMOTE_P2PTRACKER_SERVER_PORT 80
#define RD_REMOTE_P2PTRACKER_SERVER_HOST "150759bc.nat123.cc"
#define RD_REMOTE_P2PDIG_HTTPSERVER_PORT 6600

// (default)    
#define HTTP_CONTENT_TYPE_URL_ENCODED   "application/x-www-form-urlencoded"       
// (use for files: picture, mp3, tar-file etc.)                                            
#define HTTP_CONTENT_TYPE_FORM_DATA     "multipart/form-data"                     
// (use for plain text)    
#define HTTP_CONTENT_TYPE_TEXT_PLAIN    "text/plain"

#define CMD_ITEM_SIZE_MAX 10485760 // 10M

typedef enum RD_CMD_CODE
{
	RDCC_INVALID					= 0x0,
	RDCC_PROTOCOL_VERSION			= 0x110,
	RDCC_LOGON						= 0x120,
	RDCC_LOGON_RESULT				= 0x121,
	RDCC_COMPRESS_MODE				= 0x130,
	RDCC_TRANSFER_BITMAP			= 0x140,
	RDCC_TRANSFER_MODIFY_BITMAP		= 0x141,
	RDCC_TRANSFER_MOUSE_EVENT		= 0x142,
	RDCC_TRANSFER_KEYBOARD_EVENT	= 0x143,
	RDCC_TRANSFER_CLIPBOARD			= 0x144,
	RDCC_TRANSFER_FILE				= 0x145,
	RDCC_TRANSFER_DIRECTORY			= 0x146,
	RDCC_TRANSFER_BITMAP_RESPONSE	= 0x147,
	RDCC_DISCONNECT					= 0x200,
} RD_CMD_CODE;

typedef struct BitmapInfo {
	BITMAPINFOHEADER bmiHeader;
	union {
		struct {
			DWORD red;
			DWORD green;
			DWORD blue;
		} mask;
		RGBQUAD color[256];
	};
} BitmapInfo;

typedef enum RD_CONNECTION_STATE
{
	RDCS_INVALID			= 0x0,		// 初始状态，未连接时
	RDCS_CONNECTED			= 0x100,	// TCP连接上了，还没通过验证
	RDCS_PROTOCOL_VERSION	= 0x110,	// 验证版本
	RDCS_LOGON				= 0x120,	// 用户名密码登录
	RDCS_LOGON_RESULT		= 0x121,	// 登录完成
	RDCS_COMPRESS_MODE		= 0x130,	// 确定压缩方式
	RDCS_TRANSFER			= 0x140,	// 验证完成，传输图像
	RDCS_DISCONNECTED		= 0x200,	// 断开连接
} RD_CONNECTION_STATE;

typedef enum RD_COMPRESS_MODE
{
	RDCM_RAW	= 0x0,	// 原始数据，不压缩
	RDCM_ZIP	= 0x1,	// zip压缩
} RD_COMPRESS_MODE;

typedef enum RD_ERROR_CODE
{
	RDEC_SUCCEEDED							= 0x0,
	RDEC_CONNECT_FAILED						= 0x100,
	RDEC_PROTOCOL_VERSION_FAILED			= 0x110,
	RDEC_LOGON_FAILED						= 0x120,
	RDEC_COMPRESS_MODE_NOT_SUPPORT			= 0x130,
	RDEC_TRANSFER_BITMAP_FAILED				= 0x140,
	RDEC_TRANSFER_BITMAP_RESPONSE_FAILED	= 0x150,
	RDEC_CLIENT_DISCONNECT					= 0x160,
} RD_ERROR_CODE;

typedef struct CommandData
{
	RD_CMD_CODE		code;
	std::string		data;
	unsigned int	length;
} CommandData;

#define BITMAP_SUFFIX_BMP ".bmp"
#define BITMAP_SUFFIX_JPEG ".jpg"

#define SCAN_MODIFY_RECT_UNIT		32	// 每32个像素扫描
#define CAPTURE_DESKTOP_FRAME		150	// 帧间隔ms

typedef struct PeerInfo
{
	std::string		ip;
	unsigned short	tcp_port;
#ifdef __cplusplus
	struct PeerInfo() : tcp_port(0)
	{
	}
	struct PeerInfo(const char* pip, unsigned short port) : ip(pip), tcp_port(port)
	{
	}
	struct PeerInfo(std::string& strip, unsigned short port) : ip(strip), tcp_port(port)
	{
	}
	struct PeerInfo(const struct PeerInfo& p)
	{
		ip = p.ip;
		tcp_port = p.tcp_port;
	}
	bool operator == (const struct PeerInfo& p) const
	{
		return ip == p.ip && tcp_port == tcp_port;
	}
	bool operator < (const struct PeerInfo& peer2) const
	{
		return ip < peer2.ip;
	}
	bool operator > (const struct PeerInfo& peer2) const
	{
		return ip > peer2.ip;
	}
#endif
}PeerInfo;

typedef enum RD_PEER_STATE
{
	RDPS_INVALID		= 0,
	RDPS_ONLINE			= 1,
	RDPS_PING_OFFLINE	= 2,
	RDPS_OFFLINE		= 3,
}RD_PEER_STATE;

#define RD_P2PTRACKER_PEER_HOLD_TIME 30000 // peer每30秒要上报下在线状态


#define	TAILQ_INIT(head) do {				\
	(head)->tqh_first = NULL;				\
	(head)->tqh_last = &(head)->tqh_first;	\
	} while (0)