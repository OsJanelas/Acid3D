#pragma once

#pragma warning(disable: 4201)
#pragma warning(disable: 4706)

/// \cond
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)
/// \endcond

/// @file
/// <summary>Contains macros and type definitions to use in gdi3d.</summary>

#ifdef GDI3D_EXPORTS
/// <summary>Defines the macro that is used to export gdi3d functions in the output DLL.</summary>
#define GDI3DAPI __declspec(dllexport)
#else
/// <summary>Defines the macro that is used to import gdi3d functions from its DLL.</summary>
#define GDI3DAPI __declspec(dllimport)
#endif

/// <summary>An ANSI string composed of 'gdi3d,' the name of this project.</summary>
#define GDI3D_NAME "gdi3d"

/// <summary>A wide character string composed of 'gdi3d,' the name of this project.</summary>
#define GDI3D_NAME_WIDE L"gdi3d"

/// <summary>The approximate maximum length of a path that supports wide characters.</summary>
#define MAX_PATH_WIDE 32767

#ifdef _MSC_VER
#if _M_X64 == 100 || _M_IX86_FP >= 1
/// <summary>Indicates that SSE code can be generated.</summary>
#define __SSE__
#endif

#if _M_X64 == 100 || _M_IX86_FP >= 2
/// <summary>Indicates that SSE2 code can be generated.</summary>
#define __SSE2__
#endif
#endif

#ifndef PF_AVX2_INSTRUCTIONS_AVAILABLE
#define PF_AVX2_INSTRUCTIONS_AVAILABLE 40
#endif

/// \cond
typedef const void *PCVOID;
/// \endcond

/// <summary>
/// An enumeration that consists of all gdi3d errors that are set when a call fails.
/// </summary>
typedef enum {
	/// <summary>
	/// Indicates that the last gdi3d error code couldn't be retrieved or modified due to an error
	/// in the thread local storage.
	/// </summary>
	GDI3D_LAST_ERROR_TLS = -1,
	/// <summary>Indicates that last gdi3d call was successful.</summary>
	GDI3D_SUCCESS,
	/// <summary>
	/// Indicates that an internal error within gdi3d has occured. gdi3d may not have been
	/// initialized properly.
	/// </summary>
	GDI3D_INTERNAL_ERROR,
	/// <summary>Indicates that the API is currently unimplemented.</summary>
	GDI3D_UNIMPLEMENTED,
	/// <summary>
	/// Indicates that a Win32 call failed. For extended information, call
	/// <see cref="GetLastError" />.
	/// </summary>
	GDI3D_WIN32_FAILURE,
	/// <summary>
	/// Indicates that an internal gdi3d value couldn't be retrieved or modified due to an error
	/// in the thread local storage.
	/// </summary>
	GDI3D_TLS_ERROR,
	/// <summary>
	/// Indicates that a Direct3D 9 call failed. For extended information, call
	/// <see cref="GDI3D_GetLastD3D9Result" />.
	/// </summary>
	GDI3D_D3D9_FAILURE,
	/// <summary>
	/// Indicates that a Direct3D 9 call succeeded with one unexpected detail. For extended
	/// information, call <see cref="GDI3D_GetLastD3D9Result" />.
	/// </summary>
	GDI3D_D3D9_WARNING,
	/// <summary>
	/// Indicates that the gdi3d function encountered unexpected behavior from an internal call and
	/// cannot recover.
	/// </summary>
	GDI3D_UNEXPECTED_BEHAVIOR,
	/// <summary>
	/// Indicates that a specified bitmap or surface's color format is not supported by the gdi3d
	/// function.
	/// </summary>
	GDI3D_UNSUPPORTED_PIXEL_FORMAT,
	/// <summary>
	/// Indicates that a gdi3d scene call has failed during either initialization, rendering or
	/// destruction.
	/// </summary>
	GDI3D_SCENE_FAILURE
} GDI3D_ERROR, *PGDI3D_ERROR;