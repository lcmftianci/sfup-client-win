
#include "stdafx.h"
#include "TrayIcon.h"
#include "func.h"
#include "setdebugnew.h"

typedef struct _TBBUTTON32 {
	int iBitmap;
	int idCommand;
	BYTE fsState;
	BYTE fsStyle;
	BYTE bReserved[2];          // padding for alignment
	DWORD_PTR dwData;
	INT_PTR iString;
} TBBUTTON32;

typedef struct _TBBUTTON64 {
	int iBitmap;
	int idCommand;
	BYTE fsState;
	BYTE fsStyle;
	BYTE bReserved[6];          // padding for alignment
	DWORD_PTR dwData;
	INT_PTR iString;
} TBBUTTON64;

struct TRAYDATA  
{  
	HWND hwnd;                                 
	UINT uID;                                 
	UINT uCallbackMessage;         
	DWORD Reserved[2];                 
	HICON hIcon;                                 
}; 



CTrayIcon::CTrayIcon(void)
{
	memset(&m_stNotifyIconData, 0, sizeof(m_stNotifyIconData));
	m_stNotifyIconData.cbSize = sizeof(m_stNotifyIconData);
	m_bHover = FALSE;
	m_dwTimerId = 0;
}

CTrayIcon::~CTrayIcon(void)
{
}

BOOL CTrayIcon::AddIcon(HWND hCallBackWnd, UINT uCallBackMsg, 
			 UINT uID, HICON hIcon, LPCTSTR lpszTip/* = NULL*/)
{
	m_stNotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE;
	m_stNotifyIconData.hWnd = hCallBackWnd;
	m_stNotifyIconData.uCallbackMessage = uCallBackMsg;
	m_stNotifyIconData.uID = uID;
	m_stNotifyIconData.hIcon = hIcon;
	if (lpszTip != NULL && _tcslen(lpszTip) > 0)
	{
		m_stNotifyIconData.uFlags |= NIF_TIP;
		_tcsncpy(m_stNotifyIconData.szTip, lpszTip, 
			sizeof(m_stNotifyIconData.szTip) / sizeof(TCHAR));
	}
	return ::Shell_NotifyIcon(NIM_ADD, &m_stNotifyIconData);
}

BOOL CTrayIcon::ModifyIcon(HICON hIcon, LPCTSTR lpszTip/* = NULL*/)
{
	m_stNotifyIconData.uFlags = NIF_ICON;
	m_stNotifyIconData.hIcon = hIcon;
	if (lpszTip != NULL)
	{
		m_stNotifyIconData.uFlags |= NIF_TIP;
		_tcsncpy(m_stNotifyIconData.szTip, lpszTip, 
			sizeof(m_stNotifyIconData.szTip) / sizeof(TCHAR));
	}
	return ::Shell_NotifyIcon(NIM_MODIFY, &m_stNotifyIconData);
}

BOOL CTrayIcon::RemoveIcon()
{
	return ::Shell_NotifyIcon(NIM_DELETE, &m_stNotifyIconData);
}

LRESULT CTrayIcon::OnTrayIconNotify(WPARAM wParam, LPARAM lParam)
{
	UINT uID = (UINT)wParam;
	UINT uMsg = (UINT)lParam;

	if (uID == m_stNotifyIconData.uID)
	{
		if (uMsg == WM_MOUSEMOVE)
		{
			if (!m_bHover)
			{
				m_bHover = TRUE;
				::PostMessage(m_stNotifyIconData.hWnd, m_stNotifyIconData.uCallbackMessage, 
					m_stNotifyIconData.uID, WM_MOUSEHOVER);
				m_dwTimerId = ::SetTimer(m_stNotifyIconData.hWnd, 990, 160, NULL);
			}
		}
	}

	return 0;
}

void CTrayIcon::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_dwTimerId)
	{
		RECT rect;
		GetTrayIconRect(&rect);

		POINT pt = {0};
		::GetCursorPos(&pt);

		if (!::PtInRect(&rect, pt))
		{
			m_bHover = FALSE;
			::PostMessage(m_stNotifyIconData.hWnd, m_stNotifyIconData.uCallbackMessage, 
				m_stNotifyIconData.uID, WM_MOUSELEAVE);
			::KillTimer(m_stNotifyIconData.hWnd, m_dwTimerId);
			m_dwTimerId = NULL;
		}
	}
}

// ��ȡ����ͼ������λ��
BOOL CTrayIcon::GetTrayIconRect(RECT * lpRect)
{
	if (NULL == lpRect)
		return FALSE;

	::SetRectEmpty(lpRect);

	HWND hWnd = FindTrayNotifyWnd();
	if (hWnd != NULL)
	{
		if (!EnumNotifyWindow(hWnd, *lpRect))		// ���û����ͨ������
		{
			hWnd = FindNotifyIconOverflowWindow();	// ���������win7��
			if (hWnd != NULL)
			{
				EnumNotifyWindow(hWnd, *lpRect);
			}
		}
	}

	return TRUE;
}

// ö�ٻ�ȡ����ͼ������λ��
BOOL CTrayIcon::EnumNotifyWindow(HWND hWnd, RECT &rect)
{
	BOOL bSuc = FALSE;
	DWORD dwProcessId = 0, dwThreadId = 0, dwDesiredAccess;
	HANDLE hProcess;
	LPVOID lpBuffer;
	int nButtons;
	HWND hOwnerWnd = NULL;

	if (NULL == hWnd)
		return FALSE;

	dwThreadId = ::GetWindowThreadProcessId(hWnd, &dwProcessId);
	if (0 == dwProcessId || 0 == dwThreadId)
		return FALSE;

	dwDesiredAccess = PROCESS_ALL_ACCESS|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE;
	hProcess = ::OpenProcess(dwDesiredAccess, 0, dwProcessId);
	if (NULL == hProcess)
		return FALSE;

	lpBuffer = ::VirtualAllocEx(hProcess, 0, 1024, MEM_COMMIT, PAGE_READWRITE);
	if (lpBuffer != NULL)
	{
		nButtons = ::SendMessage(hWnd, TB_BUTTONCOUNT, 0, 0);	// ��ȡ����ͼ������
		for (int i = 0; i < nButtons; i++)
		{
			RECT rc = {0}; 
			TBBUTTON32 stButton32 = {0};
			TBBUTTON64 stButton64 = {0};
			TRAYDATA stTrayData = {0};
			BOOL bRet;

			::SendMessage(hWnd, TB_GETBUTTON, i, (LPARAM)lpBuffer);	// ��ȡ��i������ͼ����Ϣ

			if (!OSVerIs64Bit())	// 32λ����ϵͳ
			{
				bRet = ::ReadProcessMemory(hProcess, lpBuffer, &stButton32, sizeof(TBBUTTON32), 0);
				bRet = ::ReadProcessMemory(hProcess, (LPVOID)stButton32.dwData, &stTrayData, sizeof(TRAYDATA), 0);
			}
			else					// 64λ����ϵͳ
			{
				bRet = ::ReadProcessMemory(hProcess, lpBuffer, &stButton64, sizeof(TBBUTTON64), 0);
				bRet = ::ReadProcessMemory(hProcess, (LPVOID)stButton64.dwData, &stTrayData, sizeof(TRAYDATA), 0);
			}
			
			if (bRet != 0 && stTrayData.hwnd == m_stNotifyIconData.hWnd)
			{
				::SendMessage(hWnd, TB_GETITEMRECT, (WPARAM)i, (LPARAM)lpBuffer); // ��ȡ��i������ͼ������
				bRet = ::ReadProcessMemory(hProcess, lpBuffer, &rc, sizeof(rc),0);  // ��ȡ����ͼ������
				if (bRet != 0)
				{
					::ClientToScreen(hWnd, (LPPOINT)&rc);
					::ClientToScreen(hWnd, ((LPPOINT)&rc)+1);
					rect = rc;
				}
				bSuc = TRUE;
				break;
			}
		}
	}

	if (lpBuffer != NULL)
		::VirtualFreeEx(hProcess, lpBuffer, 0, MEM_RELEASE);
	::CloseHandle(hProcess);

	return bSuc;
}

// ��ȡ��ͨ���������ھ��
HWND CTrayIcon::FindTrayNotifyWnd()
{
	HWND hWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);
	if (hWnd != NULL)
	{
		hWnd = ::FindWindowEx(hWnd, 0, _T("TrayNotifyWnd"), NULL);
		if (hWnd != NULL)
		{
			HWND hWndPaper = ::FindWindowEx(hWnd, 0, _T("SysPager"), NULL);
			if(hWndPaper != NULL)
				hWnd = ::FindWindowEx(hWndPaper, 0, _T("ToolbarWindow32"), NULL);
			else
				hWnd = ::FindWindowEx(hWnd, 0, _T("ToolbarWindow32"), NULL);
		}
	}
	return hWnd;
}

// ��ȡ������������ھ��
HWND CTrayIcon::FindNotifyIconOverflowWindow()
{
	HWND hWnd = ::FindWindow(_T("NotifyIconOverflowWindow"), NULL);
	if (hWnd != NULL)
		hWnd = ::FindWindowEx(hWnd, NULL, _T("ToolbarWindow32"), NULL);
	return hWnd;
}

