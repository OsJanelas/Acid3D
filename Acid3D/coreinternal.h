#pragma once

#pragma warning(push, 0)
#include <d3d9.h>
#pragma warning(pop)

#include "def.h"

#include "window.h"

/// @file
/// <summary>
/// Contains the functions used by gdi3d internally.
/// </summary>

#ifndef GDI3D_CORE_INTERNAL
#error The header file 'core_internal.h' should not be included in your application! \
It is reserved for gdi3d internally. Remove the include directive for this header or \
define the macro "GDI3D_CORE_INTERNAL" before your include directive to override this error.
#endif

/// \cond
#define CONCAT(x, y) x ## y
#define WIDE_CHAR(x) CONCAT(L, x)

#define CLASS_NAME_MAX_SIZE 256

typedef D3DPRESENT_PARAMETERS *PD3DPRESENT_PARAMETERS;
/// \endcond

/// <summary>
/// A structure that contains values used internally by gdi3d per thread.
/// </summary>
typedef struct {
	/// <summary>The last gdi3d error code.</summary>
	GDI3D_ERROR eLastError;
	/// <summary>The last Direct3D 9 HRESULT code.</summary>
	HRESULT hLastResD3D9;
} GDI3DCORE_TLS_STATE, *PGDI3DCORE_TLS_STATE;

/// <summary>
/// A structure representing Direct3D 9 data needed internally for gdi3d graphics functions.
/// </summary>
typedef struct {
	/// <summary>The Direct3D 9 device associated with the window.</summary>
	PDIRECT3DDEVICE9 pDevice;
	/// <summary>
	/// A pointer to the Direct3D 9 present parameters used to swap the device's buffers.
	/// </summary>
	D3DPRESENT_PARAMETERS present;
} GDI3DCORE_D3D9_DATA, *PGDI3DCORE_D3D9_DATA;

/// <summary>
/// A structure that consists of values assigned to a window that are used internally by gdi3d.
/// </summary>
typedef struct {
	/// <summary>The heap handle used to allocate the window class.</summary>
	HANDLE hHeap;
	/// <summary>
	/// A pointer to the structure containing the window class used to create the window.
	/// </summary>
	PWNDCLASSW pClass;
	/// <summary>Specifies how to display the window on screen.</summary>
	GDI3D_DISPLAY_MODE dmWindow;
	/// <summary>Indicates the monitor on which to display the window.</summary>
	INT nMonitor;
	/// <summary>
	/// A structure representing Direct3D 9 data needed internally for gdi3d graphics functions.
	/// </summary>
	PGDI3DCORE_D3D9_DATA pD3D9Data;
} GDI3DCORE_WINDOW_DATA, *PGDI3DCORE_WINDOW_DATA;

/// <summary>
/// Modifies the last gdi3d error code. This function is called internally by gdi3d. <para/>
/// While calling this function manually will have no effect on the execution of gdi3d,
/// it should be avoided.
/// </summary>
/// <returns>
/// A boolean value indicating that the value in the thread local storage was successfully
/// modified.
/// </returns>
extern BOOL GDI3D_SetLastError(GDI3D_ERROR eCode);

/// <summary>
/// Modifies the last Direct3D 9 HRESULT code. This function is called internally by gdi3d. <para/>
/// While calling this function manually will have no effect on the execution of gdi3d,
/// it should be avoided.
/// </summary>
/// <returns>
/// A boolean value indicating that the value in the thread local storage was successfully
/// modified.
/// </returns>
extern BOOL GDI3D_SetLastD3D9Result(HRESULT hRes);

/// <summary>Gets the data that gdi3d uses internally for a window.</summary>
/// <param name="hWnd">The Win32 window handle.</param>
/// <remarks>This function will only succeed on a window created using gdi3d.</remarks>
/// <returns>A pointer to the structure representing the data.</returns>
extern PGDI3DCORE_WINDOW_DATA GDI3D_GetWindowCoreData(HWND hWnd);

/// <summary>
/// Initializes values in GDI3D to optimize functions that use SSE2 instructions.
/// </summary>
extern VOID GDI3D_PrepareSse2(VOID);

/// <summary>
/// Initializes values in GDI3D to optimize functions that use AVX2 instructions.
/// </summary>
extern VOID GDI3D_PrepareAvx2(VOID);