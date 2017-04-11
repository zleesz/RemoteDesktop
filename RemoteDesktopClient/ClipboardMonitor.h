#pragma once

class CClipboardMonitor
{
public:
	CClipboardMonitor(void);
	~CClipboardMonitor(void);

public:
	void OnClipboardChanged();
};
