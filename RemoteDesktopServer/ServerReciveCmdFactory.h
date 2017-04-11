#pragma once
#include "ServerReciveCmd.h"

class CServerReciveCmdFactory
{
public:
	static bool CreateCmd(RD_CMD_CODE code, IServerReciveCmdBase** pServerReciveCmd)
	{
		switch (code)
		{
		case RDCC_PROTOCOL_VERSION:
			*pServerReciveCmd = new CServerReciveCmdProtocalVersion();
			return true;
		case RDCC_LOGON:
			*pServerReciveCmd = new CServerReciveCmdLogon();
			return true;
		case RDCC_COMPRESS_MODE:
			*pServerReciveCmd = new CServerReciveCmdCompressMode();
			return true;
		case RDCC_TRANSFER_BITMAP:
			*pServerReciveCmd = new CServerReciveCmdTransferBitmap();
			return true;
		case RDCC_TRANSFER_BITMAP_RESPONSE:
			*pServerReciveCmd = new CServerReciveCmdTransferBitmapResponse();
			return true;
		case RDCC_TRANSFER_MOUSE_EVENT:
			*pServerReciveCmd = new CServerReciveCmdTransferMouseEvent();
			return true;
		case RDCC_TRANSFER_KEYBOARD_EVENT:
			*pServerReciveCmd = new CServerReciveCmdTransferKeyboardEvent();
			return true;
		case RDCC_TRANSFER_CLIPBOARD:
			*pServerReciveCmd = new CServerReciveCmdTransferClipboard();
			return true;
		case RDCC_TRANSFER_FILE:
			*pServerReciveCmd = new CServerReciveCmdTransferFile();
			return true;
		case RDCC_TRANSFER_DIRECTORY:
			*pServerReciveCmd = new CServerReciveCmdTransferDirectory();
			return true;
		case RDCC_DISCONNECT:
			*pServerReciveCmd = new CServerReciveCmdDisconnect();
			return true;
		default:
			break;
		}
		return false;
	}
};
