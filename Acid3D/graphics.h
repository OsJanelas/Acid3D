#pragma once

#pragma warning(push, 0)
#include <d3d9.h>
#pragma warning(pop)

#include "def.h"
#include "d3dx9.h"

/// @file
/// <summary>
/// Contains functions to display hardware accelerated graphics using Direct3D 9.
/// </summary>

/// \cond
typedef D3DPRESENT_PARAMETERS *PD3DPRESENT_PARAMETERS;
typedef D3DSURFACE_DESC       *PD3DSURFACE_DESC;
typedef D3DLOCKED_RECT        *PD3DLOCKED_RECT;
typedef ID3DXBuffer           *PD3DXBUFFER;
typedef ID3DXEffect           *PD3DXEFFECT;
typedef ID3DXMesh             *PD3DXMESH;
typedef D3DXMATERIAL          *PD3DXMATERIAL;
typedef D3DXEFFECTINSTANCE    *PD3DXEFFECTINSTANCE;
typedef ID3DXSprite           *PD3DXSPRITE;
/// \endcond

/// <summary>An union that is composed of an ARGB 32-bit color value.</summary>
typedef union {
	/// <summary>The 32-bit color value.</summary>
	DWORD argb;
	struct {
		/// <summary>The blue 8-bit channel component of the color value.</summary>
		BYTE b;
		/// <summary>The green 8-bit channel component of the color value.</summary>
		BYTE g;
		/// <summary>The red 8-bit channel component of the color value.</summary>
		BYTE r;
		/// <summary>The alpha 8-bit channel component of the color value.</summary>
		BYTE a;
	};
} ARGBQUAD, *PARGBQUAD;

/// <summary>
/// Creates a 32-bit ARGB color value from the alpha, red, green and blue 8-bit components.
/// </summary>
#define ARGB(a, r, g, b) \
	((DWORD)(BYTE)(a) << 24 | (DWORD)(BYTE)(r) << 16 | (DWORD)(BYTE)(g) << 8 | (DWORD)(BYTE)(b))

/// <summary>
/// Retrieves the 8-bit alpha component of a 32-bit ARGB color value.
/// </summary>
#define GetAValue(argb) (BYTE)((DWORD)(argb) >> 16)

/// <summary>Specifies the Direct3D 9 vertex processing behavior.</summary>
typedef enum {
	/// <summary>Indicates that vertex processing shouldn't be hardware accelerated.</summary>
	GDI3D_VP_SOFTWARE,
	/// <summary>
	/// Indicates that vertex processing should only partly take advantage of hardware
	/// acceleration.
	/// </summary>
	GDI3D_VP_MIXED,
	/// <summary>Indicates that vertex processing should be hardware accelerated.</summary>
	GDI3D_VP_HARDWARE
} GDI3D_VERTEX_PROCESSING_TYPE, *PGDI3D_VERTEX_PROCESSING_TYPE;

/// <summary>
/// Contains a D3DX mesh along with its materials and textures.
/// </summary>
typedef struct {
	/// <summary>The contained D3DX mesh.</summary>
	PD3DXMESH pD3DXMesh;
	/// <summary>A buffer containing DWORD adjacency values for the mesh.</summary>
	PD3DXBUFFER pAdjacencyBuffer;
	/// <summary>
	/// A buffer containing the D3DX materials of type D3DXMATERIAL used in the mesh.
	/// </summary>
	PD3DXBUFFER pMaterialBuffer;
	/// <summary>
	/// A buffer containing the Direct3D 9 textures of type PDIRECT3DTEXTURE9 for each subset of
	/// the mesh.
	/// </summary>
	PD3DXBUFFER pTextureBuffer;
	/// <summary>
	/// A buffer containing structures of type D3DXEFFECTINSTANCE in order to create effect
	/// instances and assign default values for each subset of the mesh.
	/// </summary>
	PD3DXBUFFER pEffectInstanceBuffer;
	/// <summary>
	/// A buffer containing effects of type D3DXEFFECT to use for each subset of the mesh.
	/// </summary>
	PD3DXBUFFER pEffectBuffer;
	/// <summary>
	/// A buffer containing error messages of type D3DXBUFFER that when dereferenced are of type
	/// PSTR, created during effect compilation.
	/// </summary>
	PD3DXBUFFER pEffectErrorBuffers;
} GDI3D_MESH, *PGDI3D_MESH;

/// <summary>The signature for a function pointer in a gdi3d scene.</summary>
/// <param name="pDevice">The Direct3D 9 device used to run the scene.</param>
/// <param name="lParam">An additional argument in which a block of memory is allocated.</param>
typedef BOOL(__cdecl *GDI3D_SCENE_CALLBACK)(PDIRECT3DDEVICE9 pDevice, LPARAM lParam);

/// <summary>Represents a scene in which to draw graphics with gdi3d.</summary>
typedef struct {
	/// <summary>
	/// An optional function pointer that is called first in order to create any resources that the
	/// scene needs for rendering.
	/// </summary>
	GDI3D_SCENE_CALLBACK pInitialize;
	/// <summary>
	/// An optional function pointer that is called whenever the scene needs to be displayed.
	/// </summary>
	GDI3D_SCENE_CALLBACK pPreRender;
	/// <summary>
	/// The function pointer that is called whenever the scene needs to be displayed.
	/// </summary>
	GDI3D_SCENE_CALLBACK pRender;
	/// <summary>
	/// An optional function pointer that is called when the scene is no longer needed to be
	/// rendered in order to clean up all resources used by it. <para/>
	/// </summary>
	/// <remarks>
	/// The function pointer assigned will still be called even if the initialization or rendering
	/// fails.
	/// </remarks>
	GDI3D_SCENE_CALLBACK pDestroy;
	/// <summary>
	/// This boolean value indicates that the scene should stop whenever the Direct3D 9 device is
	/// lost and resources must be recreated. If this value is not set, gdi3d will attempt to call
	/// <see cref="pInitialize" /> and <see cref="pDestroy" /> before rendering again.
	/// </summary>
	BOOL bStopIfDeviceLost;
} GDI3D_SCENE, *PGDI3D_SCENE;

/// <summary>Retrieves the Direct3D 9 object initialized by gdi3d.</summary>
/// <returns>The Direct3D 9 object.</returns>
extern PDIRECT3D9 GDI3D_GetD3D9(VOID);

/// <summary>Creates a Direct3D 9 device for a gdi3d window.</summary>
/// <param name="hWnd">The Win32 window handle.</param>
/// <param name="nWidth">The backbuffer width.</param>
/// <param name="nHeight">The backbuffer height.</param>
/// <param name="vpType">Specifies the Direct3D 9 vertex processing behavior.</param>
/// <param name="ppDevice">An optional output pointer to store the Direct3D 9 device.</param>
/// <remarks>
/// This function will only succeed on a window created using gdi3d. <para/>
/// The window handle is required as most gdi3d functions will use the swap chain that won't be
/// created if the window isn't provided to the Direct3D 9 device creation parameters.
/// </remarks>
/// <returns>
/// A boolean value indicating whether the creation of the device was successful.
/// </returns>
extern GDI3DAPI BOOL GDI3D_CreateD3D9Device(HWND hWnd, INT nWidth, INT nHeight,
	GDI3D_VERTEX_PROCESSING_TYPE vpType, PDIRECT3DDEVICE9 *ppDevice);

/// <summary>Finds the Direct3D 9 device associated to a gdi3d window.</summary>
/// <param name="hWnd">The Win32 window handle.</param>
/// <remarks>This function will only succeed on a window created using gdi3d.</remarks>
/// <returns>The Direct3D 9 device associated to the window.</returns>
extern GDI3DAPI PDIRECT3DDEVICE9 GDI3D_GetD3D9Device(HWND hWnd);

/// <summary>
/// Finds the Direct3D 9 presentation paramaters associated to a gdi3d window, used to create the
/// Direct3D 9 device.
/// </summary>
/// <param name="hWnd">The Win32 window handle.</param>
/// <param name="pPresent">A pointer that receives the Direct3D 9 presentation parameters.</param>
/// <remarks>This function will only succeed on a window created using gdi3d.</remarks>
/// <returns>The Direct3D 9 device associated to the window.</returns>
extern GDI3DAPI BOOL GDI3D_GetD3D9Present(HWND hWnd, PD3DPRESENT_PARAMETERS pPresent);

/// <summary>Creates a D3DX effect from HLSL code.</summary>
/// <param name="pDevice">The Direct3D 9 device to use.</param>
/// <param name="szRelativePath">The relative path to the effect file.</param>
/// <param name="ppErrors">A pointer to set the error message buffer into.</param>
/// <returns>
/// A D3DX effect. <para/>
/// If the call fails, the return value is a null pointer and the error message buffer may be
/// written to.
/// </returns>
extern GDI3DAPI PD3DXEFFECT GDI3D_CreateEffect(
	PDIRECT3DDEVICE9 pDevice, PCWSTR szRelativePath, PD3DXBUFFER *ppErrors);

/// <summary>Loads a mesh from a Direct3D .x mesh file.</summary>
/// <param name="pDevice">The Direct3D 9 device to use.</param>
/// <param name="szRelativePath">The relative path to the mesh file.</param>
/// <param name="szTexturePath">
/// The relative path to the folder containing textures required for the mesh, including the
/// trailing backslash.
/// </param>
/// <param name="szEffectPath">
/// The relative path to the folder containing effects required for the mesh, including the
/// trailing backslash.
/// </param>
/// <returns>
/// A structure containing D3DX mesh information.
/// If the call fails, the contained D3DX mesh is a null pointer.
/// </returns>
extern GDI3DAPI GDI3D_MESH GDI3D_CreateMesh(
	PDIRECT3DDEVICE9 pDevice, PCWSTR szRelativePath, PCWSTR szTexturePath,PCWSTR szEffectPath);

/// <summary>
/// Releases all resources in a gdi3d mesh created using <see cref="GDI3D_CreateMesh" />.
/// </summary>
/// <param name="pMesh">The mesh to destroy.</param>
/// <remarks>All members of the structure that are null pointers will be ignored.</remarks>
extern GDI3DAPI VOID GDI3D_DestroyMesh(PGDI3D_MESH pMesh);

/// <summary>
/// Creates a 32-bit ARGB Direct3D 9 texture from its size and mipmap levels.
/// </summary>
/// <param name="pDevice">The Direct3D 9 device to create the texture with.</param>
/// <param name="uWidth">The texture's width in pixels.</param>
/// <param name="uHeight">The texture's height in pixels.</param>
/// <param name="uLevels">Amount of texture levels. Set to 0 to generate all mipmaps.</param>
/// <param name="dwUsage">A bitfield consisting of values in the D3DUSAGE enumeration.</param>
/// <remarks>
/// gdi3d will try to create mipmaps when possible. If the device doesn't support it, the texture
/// will instead have only one level, the gdi3d last error will be set to GDI3D_D3D9_WARNING and
/// the last Direct3D 9 result will be set to D3DOK_NOAUTOGEN.
/// </remarks>
/// <returns>A Direct3D 9 texture.</returns>
extern GDI3DAPI PDIRECT3DTEXTURE9 GDI3D_CreateTexture(PDIRECT3DDEVICE9 pDevice,
	UINT uWidth, UINT uHeight, UINT uLevels, DWORD dwUsage);

/// <summary>
/// Renders a gdi3d scene on a Direct3D 9 device and blocks the current thread for a specified
/// amount of time.
/// </summary>
/// <param name="pDevice">The Direct3D 9 device to use.</param>
/// <param name="pScene">The gdi3d scene.</param>
/// <param name="qwDuration">The duration of the scene in milliseconds.</param>
/// <param name="ulParamSize">
/// The size of the 'lParam' argument in the scene callback to allocate a block of heap memory in.
/// </param>
/// <returns>
/// A boolean value indicating whether the initialization, rendering or destruction of the scene
/// was successful.
/// </returns>
extern GDI3DAPI BOOL GDI3D_SceneLoop(PDIRECT3DDEVICE9 pDevice,
	PGDI3D_SCENE pScene, DWORD64 qwDuration, ULONG_PTR ulParamSize);

/// <summary>
/// Locks and copies pixel data from a Direct3D 9 surface, such as the back buffer. <para/>
/// </summary>
/// <param name="pSurface">
/// The surface in which to read pixel data from.
/// </param>
/// <param name="pDstBits">
/// A pointer to the pixel data. The data must have 32-bit depth.
/// The data must have 32-bit depth and should be of the same size as the back buffer!
/// A pointer to the pixel data. The data must have 32-bit depth and must be large enough to
/// contain all of the surface's pixels.
/// </param>
/// <remarks>
/// Only pixel formats D3DFMT_X8R8G8B8 and D3DFMT_A8R8G8B8 are supported. <para/>
/// This function applies alpha pre-multiplication only if the surface format is of D3DFMT_A8R8G8B8
/// to make it compatible with <see cref="AlphaBlend" />.
/// </remarks>
/// <returns>A boolean value indicating whether the bit-block transfer was successful.</returns>
extern GDI3DAPI BOOL GDI3D_BlitSurface(PDIRECT3DSURFACE9 pSurface, PVOID pDstBits);

/// <summary>
/// Locks and copies pixel data to a Direct3D 9 surface, such as the back buffer. <para/>
/// </summary>
/// <param name="pSurface">
/// The surface in which to write pixel data to.
/// </param>
/// <param name="pSrcBits">
/// A pointer to the pixel data. The data must have 32-bit depth and must be large enough to
/// contain all of the surface's pixels.
/// </param>
/// <remarks>
/// Only pixel formats D3DFMT_X8R8G8B8 and D3DFMT_A8R8G8B8 are supported. <para/>
/// This function applies alpha pre-multiplication only if the surface format is of D3DFMT_A8R8G8B8
/// to make it compatible with <see cref="AlphaBlend" />.
/// </remarks>
/// <returns>A boolean value indicating whether the bit-block transfer was successful.</returns>
extern GDI3DAPI BOOL GDI3D_BlitBitmap(PVOID pSrcBits, PDIRECT3DSURFACE9 pSurface);

/// <summary>
/// Prints text on a GDI device context using the alpha channel.
/// </summary>
/// <param name="hDC">
/// The device context to print the string of characters onto using its currently selected font.
/// </param>
/// <param name="szText">The zero-terminated string of characters to print.</param>
/// <param name="dwColor">The text's 32-bit ARGB color.</param>
/// <param name="pptText">
/// A pointer to the location of the text specifiying where to draw at.
/// Set this argument to a null pointer to draw the text at zero coordinates.
/// </param>
/// <param name="prcCrop">
/// A pointer to the rectangle specifying how to crop the text.
/// Set this argument to a null pointer to draw the full text area.
/// </param>
/// <param name="uFormat">A bitfield of the DT_* enumeration that is passed to DrawText.</param>
/// <returns>A boolean value indicating whether printing the string succeeded.</returns>
extern GDI3DAPI BOOL GDI3D_PrintString(HDC hDC, PCWSTR szText,
	DWORD dwColor, PPOINT pptText, PRECT prcCrop, UINT uFormat);

/// <summary>
/// Creates a GDI device context with a bitmap consisting of a Direct3D 9 surface's pixels.
/// </summary>
/// <param name="pSurface">The Direct3D 9 surface to copy from.</param>
/// <param name="pHdc">A pointer to receive the GDI device context.</param>
/// <param name="ppBits">A pointer to receive the pixel buffer.</param>
/// <returns>
/// A boolean value indicating whether copying the surface to a GDI device context was successful.
/// </returns>
extern GDI3DAPI BOOL GDI3D_GetSurfaceDC(PDIRECT3DSURFACE9 pSurface, HDC *pHdc, PVOID *ppBits);

/// <summary>
/// Releases memory from a GDI device context created using GDI3D_GetSurfaceDC and eventually
/// copies it back to a Direct3D 9 surface.
/// </summary>
/// <param name="pSurface">The optional Direct3D 9 surface to copy to.</param>
/// <param name="hDC">The optional GDI device context.</param>
/// <param name="pBits">The pixel buffer to copy from.</param>
/// <remarks>
/// The Direct3D 9 surface must have the same format and size as the one passed to
/// GDI3D_GetSurfaceDC, or the same surface can be passed.
/// </remarks>
/// <returns>
/// A boolean value indicating whether copying the GDI device context to a surface was successful.
/// </returns>
extern GDI3DAPI BOOL GDI3D_ReleaseSurfaceDC(PDIRECT3DSURFACE9 pSurface, HDC hDC, PVOID pBits);