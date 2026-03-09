//DEPEDENCIES
#include <windows.h>
#include <cmath>
#include <vector>
#include <string>
#include <stdio.h>
#include "d3dx9.h"

//INCLUDE GDI3D FILES
#include "core.h"
#include "graphics.h"
#include "def.h"
#include "io.h"
#include "window.h"

void Render3DText(HDC hdc, std::wstring text, float time) {
    GDI3D_Context ctx(hdc);

    HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS_PITCH,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Comic Sans MS");
    SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    Matrix4x4 matRot = Matrix4x4::RotationY(time) * Matrix4x4::RotationX(sinf(time) * 0.2f);

    int layers = 10;
    for (int i = layers; i >= 0; i--) {
        float zOffset = i * 0.05f;
        Vector3 pos3D = Vector3(0, 0, zOffset) * matRot;

        Point screenPos = ctx.Project(pos3D);

        COLORREF color;
        if (i == 0) {
            color = RGB(0, 180, 255);
        }
        else {
            int shade = 50 + (i * 10);
            color = RGB(0, 50, 150 - shade);
        }

        SetTextColor(hdc, color);
        TextOutW(hdc, screenPos.x, screenPos.y, text.c_str(), text.length());
    }

    DeleteObject(hFont);
}