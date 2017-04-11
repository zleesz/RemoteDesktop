#pragma once

class CClientReciveCmdFactory
{
public:
	static bool CreateCmd(RD_CMD_CODE code, IClientReciveCmdBase** pClientReciveCmd)
	{
		switch (code)
		{
		case RDCC_LOGON_RESULT:
			*pClientReciveCmd = new CClientReciveCmdLogonResult();
			return true;
		case RDCC_TRANSFER_BITMAP:
			*pClientReciveCmd = new CClientReciveCmdTransferBitmap();
			return true;
		case RDCC_TRANSFER_MODIFY_BITMAP:
			*pClientReciveCmd = new CClientReciveCmdTransferModifyBitmap();
			return true;
		case RDCC_DISCONNECT:
			*pClientReciveCmd = new CClientReciveCmdDisconnect();
			return true;
		default:
			break;
		}
		return false;
	}
};