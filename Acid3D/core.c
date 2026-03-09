#include "core.h"

#define GDI3D_CORE_INTERNAL
#include "coreinternal.h"

BOOL          bIsSse2Present = FALSE, bIsAvx2Present = FALSE;
LARGE_INTEGER liPerfFreq     = { 0 };
PDIRECT3D9    pD3D9          = NULL;
DWORD         dwTlsIndex     = 0;

BOOL GDI3D_Initialize(VOID) {
	if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
		GDI3D_Destroy();
		GDI3D_SetLastError(GDI3D_TLS_ERROR);

		return FALSE;
	} else {
		PGDI3DCORE_TLS_STATE pState;

		if (!(pState = (PGDI3DCORE_TLS_STATE)HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY, sizeof(GDI3DCORE_TLS_STATE))))
		{
			return FALSE;
		}

		if (!TlsSetValue(dwTlsIndex, pState)) {
			GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
			return FALSE;
		}
	}

	if (!QueryPerformanceFrequency(&liPerfFreq)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	if (!(pD3D9 = Direct3DCreate9(D3D_SDK_VERSION))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		return FALSE;
	}

	if (bIsSse2Present = IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
		GDI3D_PrepareSse2();

	if (bIsAvx2Present = IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE))
		GDI3D_PrepareAvx2();

	GDI3D_SetLastError(GDI3D_SUCCESS);
	return TRUE;
}

BOOL GDI3D_IsSse2Present(VOID) {
	return bIsSse2Present;
}

BOOL GDI3D_IsAvx2Present(VOID) {
	return bIsAvx2Present;
}

DWORD64 GDI3D_QueryMicroseconds64(VOID) {
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);
	count.QuadPart /= liPerfFreq.QuadPart / 1000000;

	return count.QuadPart;
}

DWORD GDI3D_QueryMicroseconds(VOID) {
	return (DWORD)GDI3D_QueryMicroseconds64();
}

DWORD64 GDI3D_QueryMilliseconds64(VOID) {
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);
	count.QuadPart /= liPerfFreq.QuadPart / 1000;

	return count.QuadPart;
}

DWORD GDI3D_QueryMilliseconds(VOID) {
	return (DWORD)GDI3D_QueryMilliseconds64();
}

PDIRECT3D9 GDI3D_GetD3D9(VOID) {
	return pD3D9;
}

GDI3D_ERROR GDI3D_GetLastError(VOID) {
	PGDI3DCORE_TLS_STATE pState;

	return (pState = (PGDI3DCORE_TLS_STATE)TlsGetValue(dwTlsIndex)) ?
		pState->eLastError : GDI3D_LAST_ERROR_TLS;
}

BOOL GDI3D_SetLastError(GDI3D_ERROR eCode) {
	PGDI3DCORE_TLS_STATE pState;

	if (!(pState = (PGDI3DCORE_TLS_STATE)TlsGetValue(dwTlsIndex)))
		return FALSE;

	pState->eLastError = eCode;
	
	return TRUE;
}

HRESULT GDI3D_GetLastD3D9Result(VOID) {
	PGDI3DCORE_TLS_STATE pState;

	return (pState = (PGDI3DCORE_TLS_STATE)TlsGetValue(dwTlsIndex)) ?
		pState->hLastResD3D9 : GDI3D_LAST_ERROR_TLS;
}

BOOL GDI3D_SetLastD3D9Result(HRESULT hRes) {
	PGDI3DCORE_TLS_STATE pState;

	if (!(pState = (PGDI3DCORE_TLS_STATE)TlsGetValue(dwTlsIndex)))
		return FALSE;

	pState->hLastResD3D9 = hRes;

	return TRUE;
}

PGDI3DCORE_WINDOW_DATA GDI3D_GetWindowCoreData(HWND hWnd) {
	PGDI3DCORE_WINDOW_DATA pCoreWindow = NULL;

	if (!(pCoreWindow = (PGDI3DCORE_WINDOW_DATA)GetWindowLongPtrW(hWnd, 0))) {
		DWORD dwErrorCode = GetLastError();

		if (dwErrorCode) {
			GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
			SetLastError(dwErrorCode);
		} else
			GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);

		return NULL;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return pCoreWindow;
}

BOOL GDI3D_Destroy(VOID) {
	PGDI3DCORE_TLS_STATE pState;

	if (!(pState = (PGDI3DCORE_TLS_STATE)TlsGetValue(dwTlsIndex)))
		return FALSE;

	HeapFree(GetProcessHeap(), 0, pState);

	return dwTlsIndex && !TlsFree(dwTlsIndex);
}