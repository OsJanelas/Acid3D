#include "io.h"

#define GDI3D_CORE_INTERNAL
#include "core_internal.h"

HANDLE GDI3D_LoadFile(PCWSTR szRelativePath) {
	HANDLE hHeap, hFile;
	PWSTR  szDirectory;
	DWORD  dwSize;

	if (!(dwSize = GetFullPathNameW(szRelativePath, 0, NULL, NULL))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if (!(hHeap = GetProcessHeap())) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if (!(szDirectory = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(WCHAR)))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if (!GetFullPathNameW(szRelativePath, dwSize, szDirectory, NULL)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	hFile = CreateFileW(szDirectory, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	HeapFree(hHeap, 0, szDirectory);

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return hFile;
}

DWORD64 GDI3D_GetFileSize(HANDLE hFile) {
	LARGE_INTEGER li;

	if (!GetFileSizeEx(hFile, &li)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return 0;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return li.QuadPart;
}

DWORD64 GDI3D_ReadFile(HANDLE hFile, PVOID pBuffer, DWORD64 qwBufferSize) {
	DWORD64 qwBytesRead = 0, i = 0;
	DWORD   dwBytesRead = 0;

	for (; i < qwBufferSize; i += MAXDWORD, qwBytesRead += dwBytesRead) {
		PBYTE pbPointer;
		DWORD dwSize;

		pbPointer = (PBYTE)pBuffer + i * sizeof(BYTE);
		dwSize    = (DWORD)min(qwBufferSize - i, (DWORD64)MAXDWORD);

		if (!(ReadFile(hFile, pbPointer, dwSize, &dwBytesRead, NULL))) {
			GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
			return qwBytesRead + dwBytesRead;
		}
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return qwBytesRead;
}

DWORD64 GDI3D_WriteFile(HANDLE hFile, PCVOID pBuffer, DWORD64 qwBufferSize) {
	DWORD64 qwBytesWritten = 0, i = 0;
	DWORD   dwBytesWritten = 0;

	for (; i < qwBufferSize; i += MAXDWORD, qwBytesWritten += dwBytesWritten) {
		PBYTE pbPointer;
		DWORD dwSize;

		pbPointer = (PBYTE)pBuffer + i * sizeof(BYTE);
		dwSize    = (DWORD)min(qwBufferSize - i, (DWORD64)MAXDWORD);

		if (!WriteFile(hFile, pbPointer, dwSize, &dwBytesWritten, NULL)) {
			GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
			return qwBytesWritten + dwBytesWritten;
		}
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return qwBytesWritten;
}