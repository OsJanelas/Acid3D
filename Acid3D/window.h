#pragma once

#include "def.h"

/// @file
/// <summary>Provides simple 64-bit file I/O helper functions.</summary>

/// <summary>Loads a file on disk using a relative path.</summary>
/// <param name="szRelativePath">The relative path to the file.</param>
/// <returns>A file handle.</returns>
extern GDI3DAPI HANDLE GDI3D_LoadFile(PCWSTR szRelativePath);

/// <summary>Retrieves a file's size using its handle.</summary>
/// <param name="hFile">The file handle.</param>
/// <returns>
/// A 64-bit unsigned integer containing the file size. <para/>
/// If the function fails or the file is empty, the return value is 0.
/// </returns>
extern GDI3DAPI DWORD64 GDI3D_GetFileSize(HANDLE hFile);

/// <summary>Reads data from a file using its handle.</summary>
/// <param name="hFile">The file handle.</param>
/// <param name="pBuffer">The buffer to write the file data to.</param>
/// <param name="qwBufferSize">The buffer's size.</param>
/// <returns>
/// A 64-bit unsigned integer containing the amount of bytes read from the file.
/// If the function fails or the buffer size is 0, the return value is 0.
/// </returns>
extern GDI3DAPI DWORD64 GDI3D_ReadFile(HANDLE hFile, PVOID pBuffer, DWORD64 qwBufferSize);

/// <summary>Writes data to a file using its handle.</summary>
/// <param name="hFile">The file's handle.</param>
/// <param name="pBuffer">The buffer to read the data from.</param>
/// <param name="qwBufferSize">The buffer's size.</param>
/// <returns>
/// A 64-bit unsigned integer containing the amount of bytes written to the file.
/// If the function fails or the buffer size is 0, the return value is 0.
/// </returns>
extern GDI3DAPI DWORD64 GDI3D_WriteFile(HANDLE hFile, PCVOID pBuffer, DWORD64 qwBufferSize);