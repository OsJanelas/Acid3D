#define CINTERFACE
#include "graphics.h"

#include "core.h"
#include "window.h"

#pragma warning(push, 0)
#include <immintrin.h>
#include <intrin.h>
#include <strsafe.h>
#pragma warning(pop)

#define GDI3D_CORE_INTERNAL
#include "core_internal.h"

struct {
	__m128i byteMask, alphaMask;
	__m128  byteMaskInv;
} varsSse2;

struct {
	__m256i byteMask, alphaMask;
	__m256  byteMaskInv;
} varsAvx2;

BOOL GDI3D_CreateD3D9Device(
	HWND hWnd, INT nWidth, INT nHeight,
	GDI3D_VERTEX_PROCESSING_TYPE vpType, PDIRECT3DDEVICE9 *ppDevice
) {
	D3DPRESENT_PARAMETERS  present     = { 0 };
	PGDI3DCORE_WINDOW_DATA pCoreWindow = NULL;
	PDIRECT3DDEVICE9       pDevice     = NULL;
	D3DDEVTYPE devType = (D3DDEVTYPE)0;
	HRESULT    hRes    = S_OK;
	DWORD      dwFlags = 0;
	PDIRECT3D9 pD3D9;

	if (hWnd && !(pCoreWindow = (PGDI3DCORE_WINDOW_DATA)GetWindowLongPtrW(hWnd, 0))) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	if (pCoreWindow && !(pCoreWindow->pD3D9Data = (PGDI3DCORE_D3D9_DATA)HeapAlloc(
		pCoreWindow->hHeap, 0, sizeof(GDI3DCORE_D3D9_DATA))))
	{
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	if (!(pD3D9 = GDI3D_GetD3D9())) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);
		goto cleanup;
	}

	present.BackBufferFormat           = D3DFMT_A8R8G8B8;
	present.BackBufferWidth            = nWidth;
	present.BackBufferHeight           = nHeight;
	present.EnableAutoDepthStencil     = TRUE;
	present.AutoDepthStencilFormat     = D3DFMT_D16;
	present.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;
	present.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	present.hDeviceWindow              = hWnd;
	present.FullScreen_RefreshRateInHz = 0;
	present.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	if (pCoreWindow)
		present.Windowed = pCoreWindow->dmWindow != GDI3D_D3D_FULLSCREEN;
	else
		present.Windowed = TRUE;
	
	switch (vpType) {
	case GDI3D_VP_HARDWARE:
		dwFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
		devType = D3DDEVTYPE_HAL;
		break;
	case GDI3D_VP_MIXED:
		dwFlags |= D3DCREATE_MIXED_VERTEXPROCESSING;
		devType = D3DDEVTYPE_HAL;
		break;
	case GDI3D_VP_SOFTWARE:
		dwFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		devType = D3DDEVTYPE_REF;
		break;
	}

	if (FAILED(pD3D9->lpVtbl->CheckDeviceType(pD3D9, D3DADAPTER_DEFAULT, devType,
		D3DFMT_X8R8G8B8, present.BackBufferFormat, present.Windowed)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (pCoreWindow)
		pCoreWindow->pD3D9Data->present = present;
	
	if (FAILED(hRes = pD3D9->lpVtbl->CreateDevice(pD3D9, D3DADAPTER_DEFAULT,
		devType, hWnd, dwFlags, &present, &pDevice)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}
	
	if (ppDevice)
		*ppDevice = pDevice;
	if (pCoreWindow)
		pCoreWindow->pD3D9Data->pDevice = pDevice;
	
	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	if (FAILED(hRes) && pCoreWindow && pCoreWindow->pD3D9Data) {
		HeapFree(pCoreWindow->hHeap, 0, pCoreWindow->pD3D9Data);
		pCoreWindow->pD3D9Data = NULL;
	}

	return SUCCEEDED(hRes);
}

PDIRECT3DDEVICE9 GDI3D_GetD3D9Device(HWND hWnd) {
	PGDI3DCORE_WINDOW_DATA pCoreWindow = NULL;

	if (!(pCoreWindow = GDI3D_GetWindowCoreData(hWnd)))
		return NULL;

	if (!pCoreWindow->pD3D9Data) {
		GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);
		return NULL;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return pCoreWindow->pD3D9Data->pDevice;
}

BOOL GDI3D_GetD3D9Present(HWND hWnd, PD3DPRESENT_PARAMETERS pPresent) {
	PGDI3DCORE_WINDOW_DATA pCoreWindow = NULL;

	if (!(pCoreWindow = GDI3D_GetWindowCoreData(hWnd)))
		return FALSE;

	if (!pCoreWindow->pD3D9Data) {
		GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);
		return FALSE;
	}

	*pPresent = pCoreWindow->pD3D9Data->present;

	GDI3D_SetLastError(GDI3D_SUCCESS);

	return TRUE;
}

PD3DXEFFECT GDI3D_CreateEffect(
	PDIRECT3DDEVICE9 pDevice, PCWSTR szRelativePath, PD3DXBUFFER *ppErrors
) {
	PD3DXEFFECT pEffect = NULL;
	PD3DXBUFFER pErrors = NULL;
	UINT        uFlags;
	HRESULT     hRes;

	uFlags = D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
#if _DEBUG
	uFlags |= D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
#endif

	if (FAILED(hRes = D3DXCreateEffectFromFileW(pDevice, szRelativePath,
		NULL, NULL, uFlags, NULL, &pEffect, !ppErrors ? &pErrors : ppErrors)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
	} else
		GDI3D_SetLastError(GDI3D_SUCCESS);

	if (pErrors)
		pErrors->lpVtbl->Release(pErrors);

	return pEffect;
}

PWSTR HeapCombinePaths(HANDLE hHeap, PSTR szaFilename, PCWSTR szDirectory) {
	UINT_PTR uDirLength = 0, uFileLength = 0, uPathLength;
	PWSTR    szFilename, szPath;
	PSTR     szaTemp;
	HRESULT  hRes;

	if (FAILED(hRes = StringCchLengthW(szDirectory, MAX_PATH_WIDE, &uDirLength))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if ((szaTemp = strchr(szaFilename, '@') + 1) - 1)
		szaFilename = szaTemp;

	if (FAILED(hRes = StringCchLengthA(szaFilename, MAX_PATH_WIDE, &uFileLength))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if (uFileLength > INT_MAX) {
		GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);
		return NULL;
	}

	if (!(szFilename = (PWSTR)HeapAlloc(
		hHeap, HEAP_ZERO_MEMORY, (uFileLength + 1) * sizeof(WCHAR))))
	{
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return NULL;
	}

	if (!MultiByteToWideChar(
		CP_UTF8, 0, szaFilename, (INT)uFileLength * sizeof(CHAR), szFilename, (INT)uFileLength))
	{
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		HeapFree(hHeap, 0, szFilename);
		return NULL;
	}

	uPathLength = uDirLength + uFileLength + 1;

	if (!(szPath = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, uPathLength * sizeof(WCHAR)))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		HeapFree(hHeap, 0, szFilename);
		return NULL;
	}

	if (FAILED(hRes = StringCchCatW(szPath, uPathLength, szDirectory)) ||
		FAILED(hRes = StringCchCatW(szPath, uPathLength, szFilename)))
	{
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		HeapFree(hHeap, 0, szPath);
		HeapFree(hHeap, 0, szFilename);
		return NULL;
	}

	HeapFree(hHeap, 0, szFilename);

	return szPath;
}

BOOL LoadMeshTextures(
	PDIRECT3DDEVICE9 pDevice, PGDI3D_MESH pMesh, DWORD dwMaterialCount, PCWSTR szTexturePath
) {
	PD3DXMATERIAL      pMaterials;
	PDIRECT3DTEXTURE9 *pTextures;
	HANDLE   hHeap;
	HRESULT  hRes;
	DWORD    i;

	if (FAILED(hRes = D3DXCreateBuffer(
		dwMaterialCount * sizeof(PDIRECT3DTEXTURE9), &pMesh->pTextureBuffer)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		return FALSE;
	}

	pMaterials = (PD3DXMATERIAL)
		pMesh->pMaterialBuffer->lpVtbl->GetBufferPointer(pMesh->pMaterialBuffer);
	pTextures  = (PDIRECT3DTEXTURE9*)
		pMesh->pTextureBuffer->lpVtbl->GetBufferPointer(pMesh->pTextureBuffer);

	RtlZeroMemory(pTextures, dwMaterialCount * sizeof(PDIRECT3DTEXTURE9));

	if (!(hHeap = GetProcessHeap())) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	for (i = 0; i < dwMaterialCount; i++) {
		PWSTR szPath;
		
		if (!pMaterials[i].pTextureFilename)
			break;

		if (!(szPath = HeapCombinePaths(hHeap, pMaterials[i].pTextureFilename, szTexturePath)))
			return FALSE;
		
		if (FAILED(hRes = D3DXCreateTextureFromFileW(pDevice, szPath, &pTextures[i]))) {
			GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
			GDI3D_SetLastD3D9Result(hRes);
			HeapFree(hHeap, 0, szPath);
			return FALSE;
		}

		HeapFree(hHeap, 0, szPath);
	}

	return TRUE;
}

BOOL LoadMeshEffects(
	PDIRECT3DDEVICE9 pDevice, PGDI3D_MESH pMesh, DWORD dwMaterialCount, PCWSTR szEffectPath
) {
	PD3DXEFFECTINSTANCE pInstances;
	PD3DXEFFECT        *ppEffects;
	PD3DXBUFFER        *ppErrors;
	HANDLE  hHeap;
	HRESULT hRes;
	DWORD   i;
	
	if (FAILED(hRes = D3DXCreateBuffer(
		dwMaterialCount * sizeof(PD3DXEFFECT), &pMesh->pEffectBuffer)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		return FALSE;
	}

	if (FAILED(hRes = D3DXCreateBuffer(
		dwMaterialCount * sizeof(PD3DXBUFFER), &pMesh->pEffectErrorBuffers)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		return FALSE;
	}
	
	pInstances = (PD3DXEFFECTINSTANCE)
		pMesh->pEffectInstanceBuffer->lpVtbl->GetBufferPointer(pMesh->pEffectInstanceBuffer);
	ppEffects  = (PD3DXEFFECT*)
		pMesh->pEffectBuffer->lpVtbl->GetBufferPointer(pMesh->pEffectBuffer);
	ppErrors   = (PD3DXBUFFER*)
		pMesh->pEffectErrorBuffers->lpVtbl->GetBufferPointer(pMesh->pEffectErrorBuffers);

	RtlZeroMemory(ppEffects, dwMaterialCount * sizeof(PD3DXEFFECT));
	RtlZeroMemory(ppErrors,  dwMaterialCount * sizeof(PD3DXBUFFER));

	if (!(hHeap = GetProcessHeap())) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	for (i = 0; i < dwMaterialCount; i++) {
		PWSTR szPath;

		if (!pInstances[i].pEffectFilename)
			break;

		if (!(szPath = HeapCombinePaths(hHeap, pInstances[i].pEffectFilename, szEffectPath)))
			return FALSE;

		if (!(ppEffects[i] = GDI3D_CreateEffect(pDevice, szPath, &ppErrors[i]))) {
			HeapFree(hHeap, 0, szPath);
			return FALSE;
		}

		HeapFree(hHeap, 0, szPath);
	}

	return TRUE;
}

GDI3D_MESH GDI3D_CreateMesh(
	PDIRECT3DDEVICE9 pDevice, PCWSTR szRelativePath, PCWSTR szTexturePath, PCWSTR szEffectPath
) {
	DWORD      dwFlags  = 0, dwMaterials = 0;
	GDI3D_MESH mesh     = { 0 };
	BOOL       bSuccess = TRUE;
	HRESULT    hRes     = S_OK;

	if (pDevice->lpVtbl->GetSoftwareVertexProcessing(pDevice))
		dwFlags |= D3DXMESH_SOFTWAREPROCESSING;

	if (FAILED(hRes = D3DXLoadMeshFromXW(szRelativePath, dwFlags, pDevice, &mesh.pAdjacencyBuffer,
		&mesh.pMaterialBuffer, &mesh.pEffectInstanceBuffer, &dwMaterials, &mesh.pD3DXMesh)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		bSuccess = FALSE;
		goto cleanup;
	}

	if (szTexturePath && !(bSuccess = LoadMeshTextures(
		pDevice, &mesh, dwMaterials, szTexturePath)))
	{
		goto cleanup;
	}
		
	if (szEffectPath && !(bSuccess = LoadMeshEffects(
		pDevice, &mesh, dwMaterials, szEffectPath)))
	{
		goto cleanup;
	}

cleanup:
	if (!bSuccess) {
		GDI3D_DestroyMesh(&mesh);
		mesh.pD3DXMesh = NULL;
	}

	return mesh;
}

VOID GDI3D_DestroyMesh(PGDI3D_MESH pMesh) {
	if (pMesh->pD3DXMesh)
		pMesh->pD3DXMesh->lpVtbl->Release(pMesh->pD3DXMesh);
	if (pMesh->pAdjacencyBuffer)
		pMesh->pAdjacencyBuffer->lpVtbl->Release(pMesh->pAdjacencyBuffer);
	if (pMesh->pMaterialBuffer)
		pMesh->pMaterialBuffer->lpVtbl->Release(pMesh->pMaterialBuffer);

	if (pMesh->pTextureBuffer) {
		PDIRECT3DTEXTURE9 *ppTextures;
		DWORD dwSize, i;

		ppTextures = (PDIRECT3DTEXTURE9*)
			pMesh->pTextureBuffer->lpVtbl->GetBufferPointer(pMesh->pTextureBuffer);
		dwSize = pMesh->pTextureBuffer->lpVtbl->GetBufferSize(pMesh->pTextureBuffer) /
			sizeof(PDIRECT3DTEXTURE9);

		for (i = 0; i < dwSize; i++)
			if (ppTextures[i])
				ppTextures[i]->lpVtbl->Release(ppTextures[i]);

		pMesh->pTextureBuffer->lpVtbl->Release(pMesh->pTextureBuffer);
	}

	if (pMesh->pEffectInstanceBuffer)
		pMesh->pEffectInstanceBuffer->lpVtbl->Release(pMesh->pEffectInstanceBuffer);

	if (pMesh->pEffectBuffer) {
		PD3DXEFFECT *ppEffects;
		DWORD dwSize, i;

		ppEffects = (PD3DXEFFECT*)
			pMesh->pEffectBuffer->lpVtbl->GetBufferPointer(pMesh->pEffectBuffer);
		dwSize = pMesh->pEffectBuffer->lpVtbl->GetBufferSize(pMesh->pEffectBuffer) /
			sizeof(PD3DXEFFECT);

		for (i = 0; i < dwSize; i++)
			if (ppEffects[i])
				ppEffects[i]->lpVtbl->Release(ppEffects[i]);

		pMesh->pEffectBuffer->lpVtbl->Release(pMesh->pEffectBuffer);
	}

	if (pMesh->pEffectErrorBuffers) {
		PD3DXBUFFER *ppErrorBuffers;
		DWORD dwSize, i;

		ppErrorBuffers = (PD3DXBUFFER*)
			pMesh->pEffectErrorBuffers->lpVtbl->GetBufferPointer(pMesh->pEffectErrorBuffers);
		dwSize = pMesh->pEffectErrorBuffers->lpVtbl->GetBufferSize(pMesh->pEffectErrorBuffers) /
			sizeof(PD3DXBUFFER);

		for (i = 0; i < dwSize; i++)
			if (ppErrorBuffers[i])
				ppErrorBuffers[i]->lpVtbl->Release(ppErrorBuffers[i]);

		pMesh->pEffectErrorBuffers->lpVtbl->Release(pMesh->pEffectErrorBuffers);
	}
}

PDIRECT3DTEXTURE9 GDI3D_CreateTexture(
	PDIRECT3DDEVICE9 pDevice, UINT uWidth, UINT uHeight, UINT uLevels, DWORD dwUsage
) {
	D3DDEVICE_CREATION_PARAMETERS create = { 0 };
	PDIRECT3DTEXTURE9 pTexture = NULL;
	PDIRECT3D9 pD3D9;
	HRESULT    hRes;

	GDI3D_SetLastError(GDI3D_SUCCESS);

	if (!(pD3D9 = GDI3D_GetD3D9())) {
		GDI3D_SetLastError(GDI3D_INTERNAL_ERROR);
		return NULL;
	}

	if (FAILED(hRes = pDevice->lpVtbl->GetCreationParameters(pDevice, &create))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		return NULL;
	}

	if (SUCCEEDED(pD3D9->lpVtbl->CheckDeviceFormat(pD3D9, create.AdapterOrdinal, create.DeviceType,
		D3DFMT_A8R8G8B8, D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8)))
	{
		dwUsage |= D3DUSAGE_AUTOGENMIPMAP;
	} else {
		GDI3D_SetLastError(GDI3D_D3D9_WARNING);
		GDI3D_SetLastD3D9Result(D3DOK_NOAUTOGEN);
	}

	if (pDevice->lpVtbl->GetSoftwareVertexProcessing(pDevice))
		dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;

	if (FAILED(hRes = pDevice->lpVtbl->CreateTexture(pDevice, uWidth, uHeight,
		uLevels, dwUsage, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTexture, NULL)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
	}
	
	if (hRes == D3DOK_NOAUTOGEN) {
		GDI3D_SetLastError(GDI3D_D3D9_WARNING);
		GDI3D_SetLastD3D9Result(hRes);
	}

	return pTexture;
}

BOOL GDI3D_SceneLoop(
	PDIRECT3DDEVICE9 pDevice, PGDI3D_SCENE pScene, DWORD64 qwDuration, ULONG_PTR ulParamSize
) {
	PDIRECT3DSTATEBLOCK9 pStateBlock = NULL;
	BOOL    bSuccess = TRUE;
	HANDLE  hHeap    = NULL;
	LPARAM  lParam   = 0;
	DWORD64 qwTime;
	HRESULT hRes;

	if (FAILED(hRes = pDevice->lpVtbl->CreateStateBlock(pDevice, D3DSBT_ALL, &pStateBlock))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	hHeap  = GetProcessHeap();
	lParam = (LPARAM)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, ulParamSize);

	if (pScene->pInitialize && !(bSuccess = pScene->pInitialize(pDevice, lParam)))
		goto cleanup;

	qwTime = GDI3D_QueryMilliseconds64();

	while ((bSuccess = pScene->pRender(pDevice, lParam)) &&
		(qwDuration == (DWORD64)-1 || GDI3D_QueryMilliseconds64() < qwTime + qwDuration))
	{
		if (pScene->pPreRender && !(bSuccess = pScene->pPreRender(pDevice, lParam)))
			goto cleanup;

		if (FAILED(hRes = pDevice->lpVtbl->Present(pDevice, NULL, NULL, NULL, NULL))) {
			if (hRes != D3DERR_DEVICELOST || pScene->bStopIfDeviceLost) {
				GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
				GDI3D_SetLastD3D9Result(hRes);
				goto cleanup;
			} else {
				if (pScene->pDestroy)
					pScene->pDestroy(pDevice, lParam);
				if (pScene->pInitialize)
					pScene->pInitialize(pDevice, lParam);
			}
		}
	}

cleanup:
	if (pScene->pDestroy)
		bSuccess &= pScene->pDestroy(pDevice, lParam);

	if (lParam)
		HeapFree(hHeap, 0, (PVOID)lParam);

	if (pStateBlock) {
		pStateBlock->lpVtbl->Apply(pStateBlock);
		pStateBlock->lpVtbl->Release(pStateBlock);
	}
	
	if (SUCCEEDED(hRes))
		GDI3D_SetLastError(bSuccess ? GDI3D_SUCCESS : GDI3D_SCENE_FAILURE);

	return bSuccess;
}

VOID GDI3D_PrepareSse2(VOID) {
	varsSse2.byteMask    = _mm_set1_epi32(0xFF);
	varsSse2.alphaMask   = _mm_set1_epi32(0xFF000000);
	varsSse2.byteMaskInv = _mm_set_ps1(1.0f / 255.0f);
}

VOID GDI3D_PrepareAvx2(VOID) {
	varsAvx2.byteMask    = _mm256_set1_epi32(0xFF);
	varsAvx2.alphaMask   = _mm256_set1_epi32(0xFF000000);
	varsAvx2.byteMaskInv = _mm256_set1_ps(1.0f / 255.0f);
}

VOID PremultiplyAlphaAvx2(__m256i color, __m256i *output) {
	__m256  alphaPs, r, g, b;
	__m256i alpha;

	alpha   = _mm256_srli_epi32(color, 24);
	alphaPs = _mm256_mul_ps(_mm256_cvtepi32_ps(alpha), varsAvx2.byteMaskInv);

	r = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(
		_mm256_srli_epi32(color, 16), varsAvx2.byteMask)), alphaPs);

	g = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(
		_mm256_srli_epi32(color, 8), varsAvx2.byteMask)), alphaPs);

	b = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(color, varsAvx2.byteMask)), alphaPs);

	color = _mm256_or_si256(_mm256_cvtps_epi32(b),
		_mm256_or_si256(_mm256_slli_epi32(_mm256_cvtps_epi32(g), 8),
		_mm256_or_si256(_mm256_slli_epi32(_mm256_cvtps_epi32(r), 16),
		_mm256_and_si256(color, varsAvx2.alphaMask))));

	_mm256_storeu_si256(output, color);
}

VOID PremultiplyAlphaSse2(__m128i color, __m128i *output) {
	__m128  alphaPs, r, g, b;
	__m128i alpha;

	alpha   = _mm_srli_epi32(color, 24);
	alphaPs = _mm_mul_ps(_mm_cvtepi32_ps(alpha), varsSse2.byteMaskInv);

	r = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(
		_mm_srli_epi32(color, 16), varsSse2.byteMask)), alphaPs);

	g = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(
		_mm_srli_epi32(color, 8), varsSse2.byteMask)), alphaPs);

	b = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(color, varsSse2.byteMask)), alphaPs);

	color = _mm_or_si128(_mm_cvtps_epi32(b),
		_mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(g), 8),
		_mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(r), 16),
		_mm_and_si128(color, varsSse2.alphaMask))));

	_mm_storeu_si128(output, color);
}

VOID CopyPixelsAvx2(D3DFORMAT format, UINT uCount, PVOID pSrc, PVOID pDst) {
	if (format == D3DFMT_A8R8G8B8) {
		UINT i;

		for (i = 0; i < uCount; i += 8) {
			PremultiplyAlphaAvx2(
				_mm256_loadu_si256((const __m256i*)&((PARGBQUAD)pSrc)[i]),
				(__m256i*)&((PARGBQUAD)pDst)[i]);
		}
	} else
		RtlCopyMemory(pDst, pSrc, uCount * 4);
}

VOID CopyPixelsSse2(D3DFORMAT format, UINT uCount, PVOID pSrc, PVOID pDst) {
	if (format == D3DFMT_A8R8G8B8) {
		UINT i;

		for (i = 0; i < uCount; i += 4) {
			PremultiplyAlphaSse2(
				_mm_loadu_si128((const __m128i*)&((PARGBQUAD)pSrc)[i]),
				(__m128i*)&((PARGBQUAD)pDst)[i]);
		}
	} else
		RtlCopyMemory(pDst, pSrc, uCount * 4);
}

VOID CopyPixels(D3DFORMAT format, UINT uCount, PVOID pSrc, PVOID pDst) {
	if (format == D3DFMT_A8R8G8B8) {
		UINT i;

#pragma omp parallel for
		for (i = 0; i < uCount; i++) {
			ARGBQUAD color;

			color = ((PARGBQUAD)pSrc)[i];

			color.r = (BYTE)((FLOAT)color.r * (FLOAT)color.a / 255.0f);
			color.g = (BYTE)((FLOAT)color.g * (FLOAT)color.a / 255.0f);
			color.b = (BYTE)((FLOAT)color.b * (FLOAT)color.a / 255.0f);

			((PARGBQUAD)pDst)[i] = color;
		}
	} else
		RtlCopyMemory(pDst, pSrc, uCount * 4);
}

VOID CopyPixelsFixWidthAvx2(
	D3DFORMAT format, UINT uWidth, UINT uHeight,
	PVOID pSrc, INT nSrcWidth, PVOID pDst, INT nDstWidth
) {
	if (format == D3DFMT_A8R8G8B8) {
		UINT i;

#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i += 8) {
			UINT x, y;

			x = i % uWidth;
			y = i / uWidth;

			PremultiplyAlphaAvx2(
				_mm256_loadu_si256((const __m256i*)&((PARGBQUAD)pSrc)[y * nSrcWidth + x]),
				(__m256i*)&((PARGBQUAD)pDst)[y * nDstWidth + x]);
		}
	} else {
		UINT i;

#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i += 8) {
			UINT x, y;

			x = i % uWidth;
			y = i / uWidth;

			_mm256_storeu_si256((__m256i*)&(((PARGBQUAD)pDst)[y * nDstWidth + x]),
				_mm256_loadu_si256((const __m256i*)&(((PARGBQUAD)pSrc)[y * nSrcWidth + x])));
		}
	}
}

VOID CopyPixelsFixWidthSse2(
	D3DFORMAT format, UINT uWidth, UINT uHeight,
	PVOID pSrc, INT nSrcWidth, PVOID pDst, INT nDstWidth
) {
	if (format == D3DFMT_A8R8G8B8) {
		UINT i;

#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i += 4) {
			UINT x, y;

			x = i % uWidth;
			y = i / uWidth;

			PremultiplyAlphaSse2(
				_mm_loadu_si128((const __m128i*)&((PARGBQUAD)pSrc)[y * nSrcWidth + x]),
				(__m128i*)&((PARGBQUAD)pDst)[y * nDstWidth + x]);
		}
	} else {
		UINT i;

#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i += 4) {
			UINT x, y;

			x = i % uWidth;
			y = i / uWidth;

			_mm_storeu_si128((__m128i*)&(((PARGBQUAD)pDst)[y * nDstWidth + x]),
				_mm_loadu_si128((const __m128i*)&(((PARGBQUAD)pSrc)[y * nSrcWidth + x])));
		}
	}
}

VOID CopyPixelsFixWidth(
	D3DFORMAT format, UINT uWidth, UINT uHeight,
	PVOID pSrc, INT nSrcWidth, PVOID pDst, INT nDstWidth
) {
	UINT i;

	if (format == D3DFMT_A8R8G8B8) {
#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i++) {
			ARGBQUAD color;
			UINT     x, y;

			x = i % uWidth;
			y = i / uWidth;

			color = ((PARGBQUAD)pSrc)[y * nSrcWidth + x];

			color.r = (BYTE)((FLOAT)color.r * (FLOAT)color.a / 255.0f);
			color.g = (BYTE)((FLOAT)color.g * (FLOAT)color.a / 255.0f);
			color.b = (BYTE)((FLOAT)color.b * (FLOAT)color.a / 255.0f);

			((PARGBQUAD)pDst)[y * nDstWidth + x] = color;
		}
	} else {
#pragma omp parallel for
		for (i = 0; i < uWidth * uHeight; i++) {
			UINT x, y;

			x = i % uWidth;
			y = i / uWidth;

			((PARGBQUAD)pDst)[y * nDstWidth + x] = ((PARGBQUAD)pSrc)[y * nSrcWidth + x];
		}
	}
}

BOOL GDI3D_BlitSurface(
	PDIRECT3DSURFACE9 pSurface, PVOID pDstBits
) {
	D3DSURFACE_DESC surfaceDesc = { (D3DFORMAT)0 };
	D3DLOCKED_RECT  lockRect    = { 0 };
	HRESULT         hRes        = S_OK;

	if (FAILED(hRes = pSurface->lpVtbl->GetDesc(pSurface, &surfaceDesc))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (surfaceDesc.Format != D3DFMT_X8R8G8B8 && surfaceDesc.Format != D3DFMT_A8R8G8B8) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_UNSUPPORTED_PIXEL_FORMAT);
		goto cleanup;
	}

	if (FAILED(hRes = pSurface->lpVtbl->LockRect(
		pSurface, &lockRect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (lockRect.Pitch > 0 && (UINT)lockRect.Pitch / 4 > surfaceDesc.Width) {
		if (GDI3D_IsAvx2Present()) {
			CopyPixelsFixWidthAvx2(surfaceDesc.Format, surfaceDesc.Width, surfaceDesc.Height,
				lockRect.pBits, lockRect.Pitch / 4, pDstBits, surfaceDesc.Width);
		} else if (GDI3D_IsSse2Present()) {
			CopyPixelsFixWidthSse2(surfaceDesc.Format, surfaceDesc.Width, surfaceDesc.Height,
				lockRect.pBits, lockRect.Pitch / 4, pDstBits, surfaceDesc.Width);
		} else {
			CopyPixelsFixWidth(surfaceDesc.Format, surfaceDesc.Width, surfaceDesc.Height,
				lockRect.pBits, lockRect.Pitch / 4, pDstBits, surfaceDesc.Width);
		}
	} else if (lockRect.Pitch <= 0 || (UINT)lockRect.Pitch / 4 < surfaceDesc.Width) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_UNEXPECTED_BEHAVIOR);
		goto cleanup;
	} else {
		if (GDI3D_IsAvx2Present()) {
			CopyPixelsAvx2(surfaceDesc.Format, surfaceDesc.Width * surfaceDesc.Height,
				lockRect.pBits, pDstBits);
		} else if (GDI3D_IsSse2Present()) {
			CopyPixelsSse2(surfaceDesc.Format, surfaceDesc.Width * surfaceDesc.Height,
				lockRect.pBits, pDstBits);
		} else {
			CopyPixels(surfaceDesc.Format, surfaceDesc.Width * surfaceDesc.Height,
				lockRect.pBits, pDstBits);
		}
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);
	
cleanup:
	if (pSurface && lockRect.pBits)
		pSurface->lpVtbl->UnlockRect(pSurface);

	return SUCCEEDED(hRes);
}

BOOL GDI3D_BlitBitmap(
	PVOID pSrcBits, PDIRECT3DSURFACE9 pSurface
) {
	D3DSURFACE_DESC surfaceDesc = { (D3DFORMAT)0 };
	D3DLOCKED_RECT  lockRect    = { 0 };
	HRESULT         hRes        = S_OK;
	
	if (FAILED(hRes = pSurface->lpVtbl->GetDesc(pSurface, &surfaceDesc))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (surfaceDesc.Format != D3DFMT_X8R8G8B8 && surfaceDesc.Format != D3DFMT_A8R8G8B8 &&
		surfaceDesc.Format != D3DFMT_X8B8G8R8 && surfaceDesc.Format != D3DFMT_A8B8G8R8)
	{
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_UNSUPPORTED_PIXEL_FORMAT);
		goto cleanup;
	}

	if (FAILED(hRes = pSurface->lpVtbl->LockRect(pSurface, &lockRect, NULL, 0))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (lockRect.Pitch > 0 && (UINT)lockRect.Pitch / 4 > surfaceDesc.Width) {
		CopyPixelsFixWidth(surfaceDesc.Format, surfaceDesc.Width, surfaceDesc.Height,
			pSrcBits, surfaceDesc.Width, lockRect.pBits, lockRect.Pitch / 4);
	} else if (lockRect.Pitch <= 0 || (UINT)lockRect.Pitch / 4 < surfaceDesc.Width) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_UNEXPECTED_BEHAVIOR);
		goto cleanup;
	} else {
		CopyPixels(surfaceDesc.Format, surfaceDesc.Width * surfaceDesc.Height,
			pSrcBits, lockRect.pBits);
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	if (lockRect.pBits)
		pSurface->lpVtbl->UnlockRect(pSurface);

	return SUCCEEDED(hRes);
}

// The next 2 functions are currently unused.

PDIRECT3DSURFACE9 GDI3D_GetLockableBackbuffer(PDIRECT3DDEVICE9 pDevice, PBOOL pbIsCopy) {
	D3DLOCKED_RECT    lockRect = { 0 };
	PDIRECT3DSURFACE9 pSurface = NULL;
	HRESULT           hRes     = S_OK;

	if (FAILED(hRes = pDevice->lpVtbl->GetBackBuffer(
		pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (FAILED(hRes = pSurface->lpVtbl->LockRect(pSurface,
		&lockRect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE)))
	{
		D3DSURFACE_DESC desc = { (D3DFORMAT)0 };

		if (FAILED(hRes = pSurface->lpVtbl->GetDesc(pSurface, &desc))) {
			GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
			GDI3D_SetLastD3D9Result(hRes);
			goto cleanup;
		}

		pSurface->lpVtbl->Release(pSurface);

		if (FAILED(hRes = pDevice->lpVtbl->CreateOffscreenPlainSurface(pDevice,
			desc.Width, desc.Height, desc.Format, D3DPOOL_DEFAULT, &pSurface, NULL)))
		{
			GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
			GDI3D_SetLastD3D9Result(hRes);
			goto cleanup;
		}

		*pbIsCopy = TRUE;

		goto cleanup;
	}

	pSurface->lpVtbl->UnlockRect(pSurface);

	*pbIsCopy = FALSE;

	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	return pSurface;
}

BOOL GDI3D_UpdateBackbufferCopy(PDIRECT3DDEVICE9 pDevice, PDIRECT3DSURFACE9 pSurface) {
	PDIRECT3DSURFACE9 pBackSurface = NULL;
	HRESULT hRes = S_OK;

	if (FAILED(hRes = pDevice->lpVtbl->GetBackBuffer(
		pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackSurface)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (FAILED(hRes = pDevice->lpVtbl->StretchRect(pDevice,
		pBackSurface, NULL, pSurface, NULL, D3DTEXF_NONE)))
	{
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	return SUCCEEDED(hRes);
}

BOOL GDI3D_PrintString(
	HDC hDC, PCWSTR szText, DWORD dwColor, PPOINT pptText, PRECT prcCrop, UINT uFormat
) {
	BITMAPINFO    bmi      = { sizeof(BITMAPINFOHEADER) };
	RECT          rcText   = { 0 };
	BLENDFUNCTION blend    = { 0 };
	ARGBQUAD      color    = { 0 };
	POINT         ptSrc    = { 0 };
	BOOL          bSuccess = TRUE;
	PARGBQUAD     pPixels  = NULL;
	HDC           hdcTemp  = NULL;
	HBITMAP       hbmTemp  = NULL;
	ULONG_PTR     i;
	
	if (!DrawTextW(hDC, szText, -1, &rcText, uFormat | DT_CALCRECT)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	uFormat |= DT_NOCLIP;

	if (prcCrop) {
		if (rcText.right < prcCrop->right - prcCrop->left) {
			rcText.right = prcCrop->right - prcCrop->left;
			uFormat &= ~DT_NOCLIP;
		}
		if (rcText.bottom < prcCrop->bottom - prcCrop->top) {
			rcText.bottom = prcCrop->bottom - prcCrop->top;
			uFormat &= ~DT_NOCLIP;
		}
	}

	if (!(hdcTemp = CreateCompatibleDC(hDC))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes   = 1;
	bmi.bmiHeader.biWidth    = rcText.right;
	bmi.bmiHeader.biHeight   = rcText.bottom;

	if (!(hbmTemp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (PVOID*)&pPixels, NULL, 0))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	SelectObject(hdcTemp, hbmTemp);
	SelectObject(hdcTemp, (HFONT)GetCurrentObject(hDC, OBJ_FONT));
	
	SetBkMode(hdcTemp, OPAQUE);
	SetTextColor(hdcTemp, 0xFFFFFF);
	SetBkColor(hdcTemp, 0);

	GdiFlush();

	if (!DrawTextW(hdcTemp, szText, -1, &rcText, uFormat)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		goto cleanup;
	}

	color.argb = dwColor;

	for (i = 0; i < (ULONG_PTR)bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight; i++) {
		DWORD    dwAlpha;
		ARGBQUAD pixel;
		
		pixel   = pPixels[i];
		dwAlpha = ((DWORD)pixel.r + (DWORD)pixel.g + (DWORD)pixel.b) / 3;

		pixel.a = (BYTE)((DWORD)color.a * (dwAlpha + 1) >> 8);
		pixel.r = (BYTE)((DWORD)color.r * ((DWORD)pixel.r + 1) >> 8);
		pixel.g = (BYTE)((DWORD)color.g * ((DWORD)pixel.g + 1) >> 8);
		pixel.b = (BYTE)((DWORD)color.b * ((DWORD)pixel.b + 1) >> 8);

		pPixels[i] = pixel;
	}

	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 0xFF;
	blend.AlphaFormat = AC_SRC_ALPHA;

	if (pptText) {
		rcText.left = pptText->x;
		rcText.top  = pptText->y;
	}
	if (prcCrop) {
		ptSrc.x = prcCrop->left;
		ptSrc.y = prcCrop->top;
	}

	AlphaBlend(hDC, rcText.left, rcText.top, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight,
		hdcTemp, -ptSrc.x, -ptSrc.y,
		bmi.bmiHeader.biWidth + ptSrc.x, bmi.bmiHeader.biHeight + ptSrc.y,
		blend);

	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	if (hbmTemp)
		DeleteObject(hbmTemp);
	if (hdcTemp)
		DeleteDC(hdcTemp);

	return bSuccess;
}

BOOL GDI3D_GetSurfaceDC(PDIRECT3DSURFACE9 pSurface, HDC *pHdc, PVOID *ppBits) {
	BITMAPINFO      bmi         = { sizeof(BITMAPINFOHEADER) };
	D3DSURFACE_DESC surfaceDesc = { (D3DFORMAT)0 };
	HRESULT hRes = S_OK;
	HBITMAP hBmp = NULL;
	HDC     hDC  = NULL;

	if (FAILED(hRes = pSurface->lpVtbl->GetDesc(pSurface, &surfaceDesc))) {
		GDI3D_SetLastError(GDI3D_D3D9_FAILURE);
		GDI3D_SetLastD3D9Result(hRes);
		goto cleanup;
	}

	if (surfaceDesc.Format != D3DFMT_A8R8G8B8 && surfaceDesc.Format != D3DFMT_A8B8G8R8) {
		hRes = S_FALSE;
		GDI3D_SetLastError(GDI3D_UNSUPPORTED_PIXEL_FORMAT);
		goto cleanup;
	}

	if (!(hDC = CreateCompatibleDC(NULL))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		hRes = S_FALSE;
		goto cleanup;
	}

	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes   = 1;
	bmi.bmiHeader.biWidth    = surfaceDesc.Width;
	bmi.bmiHeader.biHeight   = -(LONG)surfaceDesc.Height;

	if (!(hBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, ppBits, NULL, 0))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		hRes = S_FALSE;
		goto cleanup;
	}

	SelectObject(hDC, hBmp);

	GdiFlush();

	if (!GDI3D_BlitSurface(pSurface, *ppBits)) {
		hRes = S_FALSE;
		goto cleanup;
	}

	GDI3D_SetLastError(GDI3D_SUCCESS);

cleanup:
	if (FAILED(hRes)) {
		if (hBmp)
			DeleteObject(hBmp);
		if (hDC)
			DeleteDC(hDC);

		return FALSE;
	} else {
		*pHdc = hDC;

		return TRUE;
	}
}

BOOL GDI3D_ReleaseSurfaceDC(PDIRECT3DSURFACE9 pSurface, HDC hDC, PVOID pBits) {
	BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER) };
	HBITMAP    hBmp;

	if (!(hBmp = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP))) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	if (!GetDIBits(hDC, hBmp, 0, 0, NULL, &bmi, DIB_RGB_COLORS)) {
		GDI3D_SetLastError(GDI3D_WIN32_FAILURE);
		return FALSE;
	}

	if (bmi.bmiHeader.biBitCount != 32) {
		GDI3D_SetLastError(GDI3D_UNSUPPORTED_PIXEL_FORMAT);
		return FALSE;
	}

	if (pSurface && !GDI3D_BlitBitmap(pBits, pSurface))
		return FALSE;

	GDI3D_SetLastError(GDI3D_SUCCESS);

	DeleteObject(hBmp);
	DeleteDC(hDC);

	return TRUE;
}