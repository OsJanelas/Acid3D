#ifndef _D3DX9_H_
#define _D3DX9_H_

typedef struct D3DXVECTOR3 {
    float x, y, z;

    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    D3DXVECTOR3 operator * (float s) const { return D3DXVECTOR3(x * s, y * s, z * s); }
} D3DXVECTOR3, *LPD3DXVECTOR3;

typedef struct D3DXMATRIX {
    float m[4][4];
} D3DXMATRIX, *LPD3DXMATRIX;

inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2) {
    D3DXMATRIX out;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.m[i][j] = pM1->m[i][0] * pM2->m[0][j] + pM1->m[i][1] * pM2->m[1][j] +
                          pM1->m[i][2] * pM2->m[2][j] + pM1->m[i][3] * pM2->m[3][j];
        }
    }
    *pOut = out;
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX *pOut, float angle) {
    float s = sinf(angle);
    float c = cosf(angle);
    memset(pOut, 0, sizeof(D3DXMATRIX));
    pOut->m[0][0] = c;  pOut->m[0][2] = -s;
    pOut->m[1][1] = 1;
    pOut->m[2][0] = s;  pOut->m[2][2] = c;
    pOut->m[3][3] = 1;
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX *pOut, float angle) {
    float s = sinf(angle);
    float c = cosf(angle);
    memset(pOut, 0, sizeof(D3DXMATRIX));
    pOut->m[0][0] = 1;
    pOut->m[1][1] = c;  pOut->m[1][2] = s;
    pOut->m[2][1] = -s; pOut->m[2][2] = c;
    pOut->m[3][3] = 1;
    return pOut;
}

inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM) {
    float x = pV->x * pM->m[0][0] + pV->y * pM->m[1][0] + pV->z * pM->m[2][0] + pM->m[3][0];
    float y = pV->x * pM->m[0][1] + pV->y * pM->m[1][1] + pV->z * pM->m[2][1] + pM->m[3][1];
    float z = pV->x * pM->m[0][2] + pV->y * pM->m[1][2] + pV->z * pM->m[2][2] + pM->m[3][2];
    float w = pV->x * pM->m[0][3] + pV->y * pM->m[1][3] + pV->z * pM->m[2][3] + pM->m[3][3];
    
    pOut->x = x / w; pOut->y = y / w; pOut->z = z / w;
    return pOut;
}

#endif