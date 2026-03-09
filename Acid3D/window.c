#include "window.h"

#pragma warning(push, 0)
#include <strsafe.h>
#pragma warning(pop)

#define GDI3D_CORE_INTERNAL
#define CINTERFACE
#include "core_internal.h"

#define CLASS_NAME      WIDE_CHAR(GDI3D_NAME) L"_"
#define CLASS_NAME_LEN (_countof(CLASS_NAME) - 1)
#define TID_STRING_LEN 8
#define ARRAY_LEN      (CLASS_NAME_LEN + TID_STRING_LEN + 1)

/// \cond
typedef struct {
	INT  nMonitor;
	RECT rcMonitor;
	INT  i;
} MONITORENUMPARAMS, *PMONITORENUMPARAMS;
/// \endcond

typedef LPCREATESTRUCT PCREATESTRUCT;

BOOL AdjustWindow(HWND hWnd, INT nClientWidth, INT nClientHeight, INT nMonitor);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PGDI3DCORE_WINDOW_DATA pCoreWindow = NULL;

	switch (uMsg) {
	case WM_CREATE: {
		PCREATESTRUCT pCreate;

		pCreate = (PCREATESTRUCT)lParam;
		if (!pCreate)
			break;

		pCoreWindow = (PGDI3DCORE_WINDOW_DATA)pCreate->lpCreateParams;
	}
	case WM_DISPLAYCHANGE:
		if (!pCoreWindow && !(pCoreWindow = (PGDI3DCORE_WINDOW_DATA)GetWindowLongPtrW(hWnd, 0)))
			break;

		switch (pCoreWindow->dmWindow) {
		case GDI3D_FULLSCREEN:
			AdjustWindow(hWnd, -1, -1, pCoreWindow->nMonitor);
			return 0;
		case GDI3D_VIRTUAL_DESKTOP:
			SetWindowPos(hWnd, NULL,
				GetSystemMetrics(SM_XVIRTUALSCREEN),  GetSystemMetrics(SM_YVIRTUALSCREEN),
				GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
				SWP_NOZORDER);
			return 0;
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY: {
		if (!(pCoreWindow = (PGDI3DCORE_WINDOW_DATA)GetWindowLongPtrW(hWnd, 0)))
			goto end;

		if (pCoreWindow->pClass) {
			UnregisterClassW(pCoreWindow->pClass->lpszClassName, pCoreWindow->pClass->hInstance);

			if (!pCoreWindow->hHeap)
				goto end;

			HeapFree(pCoreWindow->hHeap, 0, (PVOID)pCoreWindow->pClass->lpszClassName);
			HeapFree(pCoreWindow->hHeap, 0, pCoreWindow->pClass);
		}

		if (pCoreWindow->pD3D9Data) {
			HeapFree(pCoreWindow->hHeap, 0, pCoreWindow->pD3D9Data);

			if (pCoreWindow->pD3D9Data->pDevice)
				pCoreWindow->pD3D9Data->pDevice->lpVtbl->Release(pCoreWindow->pD3D9Data->pDevice);

		}

		HeapFree(pCoreWindow->hHeap, 0, pCoreWindow);

	end:
		PostQuitMessage(0);

		return 0;
	}
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hDC, PRECT prcClip, LPARAM lParam) {
	PMONITORENUMPARAMS pParams = (PMONITORENUMPARAMS)lParam;

	UNREFERENCED_PARAMETER(hMonitor);
	UNREFERENCED_PARAMETER(hDC);

	if (pParams->i < pParams->nMonitor) {
		pParams->i++;
		return TRUE;
	} else {
		pParams->rcMonitor = *prcClip;
		return FALSE;
	}
}

BOOL AdjustWindow(HWND hWnd, INT nClientWidth, INT nClientHeight, INT nMonitor) {
	MONITORENUMPARAMS params = { 0 };
	INT  nBorderWidth, nBorderHeight, x, y, nMonWidth, nMonHeight;
	RECT rcWindow = { 0 }, rcClient = { 0 };
	
	if (!GetWindowRect(hWnd, &rcWindow))
		return FALSE;
	if (!GetClientRect(hWnd, &rcClient))
		return FALSE;

	nBorderWidth  = rcWindow.right  - rcWindow.left - (rcClient.right  - rcClient.left);
	nBorderHeight = rcWindow.bottom - rcWindow.top  - (rcClient.bottom - rcClient.top);

	params.nMonitor = nMonitor;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&params);

	x = params.rcMonitor.left;
	y = params.rcMonitor.top;
	nMonWidth  = params.rcMonitor.right  - x;
	nMonHeight = params.rcMonitor.bottom - y;

	if (nClientWidth == -1)
		nClientWidth = nMonWidth;
	if (nClientHeight == -1)
		nClientHeight = nMonHeight;

	return SetWindowPos(hWnd, NULL,
		x + nMonWidth  / 2 - nClientWidth  / 2 - nBorderWidth  / 2,
		y + nMonHeight / 2 - nClientHeight / 2 - nBorderHeight / 2,
		nClientWidth + nBorderWidth, nClientHeight + nBorderHeight, 0);
}

HWND GDI3D_CreateWindow(PGDI3D_WINDOW_INFO pWindowInfo) {
	PGDI3DCORE_WINDOW_DATA pCoreWindow;
	GDI3D_WINDOW_INFO      windowInfo;
	HWND       hWnd        = NULL;
	DWORD      dwErrorCode = 0;
	DWORD      dwStyle;
	PWNDCLASSW pClass;
	HANDLE     hHeap;
	ATOM       atom;

	hHeap = GetProcessHeap();

	windowInfo = *pWindowInfo;
	pCoreWindow = (PGDI3DCORE_WINDOW_DATA)HeapAlloc(
		hHeap, HEAP_ZERO_MEMORY, sizeof(GDI3DCORE_WINDOW_DATA));
	pClass = (PWNDCLASSW)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(WNDCLASSW));

	if (!pCoreWindow || !pClass) {
		dwErrorCode = GetLastError();
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto error;
	}

	pCoreWindow->hHeap    = hHeap;
	pCoreWindow->pClass   = pClass;
	pCoreWindow->dmWindow = windowInfo.dmWindow;
	pCoreWindow->nMonitor = windowInfo.nMonitor;

	pClass->hInstance     = GetModuleHandleW(NULL);
	pClass->style         = windowInfo.dwClassStyle;
	pClass->lpfnWndProc   = WindowProc;
	pClass->hCursor       = (HCURSOR)LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0,LR_SHARED);
	pClass->hIcon         = windowInfo.hIcon;
	pClass->hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	pClass->cbWndExtra    = sizeof(PGDI3DCORE_WINDOW_DATA);
	pClass->lpszClassName = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, ARRAY_LEN * sizeof(WCHAR));

	if (!pClass->lpszClassName) {
		dwErrorCode = GetLastError();
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto error;
	}
	
	RtlCopyMemory((PVOID)pClass->lpszClassName, CLASS_NAME, CLASS_NAME_LEN * sizeof(WCHAR));

	StringCchPrintfW((PWSTR)pClass->lpszClassName + CLASS_NAME_LEN,
		TID_STRING_LEN + 1, L"%08x", GetCurrentThreadId());

	if (!(atom = RegisterClassW(pClass))) {
		dwErrorCode = GetLastError();
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto error;
	}

	if (windowInfo.dmWindow > GDI3D_WINDOWED) {
		windowInfo.nWidth  = -1;
		windowInfo.nHeight = -1;
	}

	dwStyle = windowInfo.dwStyle;
	if (windowInfo.dmWindow == GDI3D_WINDOWED)
		dwStyle &= ~WS_VISIBLE;

	hWnd = CreateWindowExW(windowInfo.dwExStyle, pClass->lpszClassName,
		windowInfo.szTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		windowInfo.nWidth, windowInfo.nHeight, NULL, NULL, pClass->hInstance, pCoreWindow);

	if (!hWnd) {
		dwErrorCode = GetLastError();
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto error;
	}

	SetWindowLongPtrW(hWnd, 0, (INT_PTR)pCoreWindow);

	if (windowInfo.dmWindow == GDI3D_WINDOWED && !AdjustWindow(hWnd,
		windowInfo.nWidth, windowInfo.nHeight, windowInfo.nMonitor))
	{
		dwErrorCode = GetLastError();
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto error;
	}

	if (dwStyle != windowInfo.dwStyle)
		ShowWindow(hWnd, SW_SHOW);

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return hWnd;
error:
	if (pCoreWindow) {
		if (pClass && pClass->lpszClassName) {
			HeapFree(hHeap, 0, (PVOID)pClass->lpszClassName);
			UnregisterClassW(pClass->lpszClassName, pClass->hInstance);
		}
		
		if (hWnd)
			DestroyWindow(hWnd);

		HeapFree(hHeap, 0, pCoreWindow);
	}
	
	if (pClass)
		HeapFree(hHeap, 0, pClass);

	SetLastError(dwErrorCode);

	return NULL;
}

VOID GDI3D_PollEvents(VOID) {
	MSG msg;
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

BOOL GDI3D_WindowExists(HWND hWnd) {
	return GetWindowLongPtrW(hWnd, 0) > 0;
}

BOOL GDI3D_DestroyWindow(HWND hWnd) {
	if (!DestroyWindow(hWnd)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);
	return TRUE;
}