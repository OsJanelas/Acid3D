void RenderPolygonBackground(HDC hdc, int width, int height) {
    int gridX = 10;
    int gridY = 8;
    int cellW = width / gridX;
    int cellH = height / gridY;

    for (int x = 0; x < gridX; x++) {
        for (int y = 0; y < gridY; y++) {
            POINT pts[3];
            pts[0] = { x * cellW, y * cellH };
            pts[1] = { (x + 1) * cellW, y * cellH };
            pts[2] = { x * cellW, (y + 1) * cellH };

            HBRUSH hBrush = CreateSolidBrush(RGB(x * 20, y * 30, 150)); 
            SelectObject(hdc, hBrush);
            
            Polygon(hdc, pts, 3);
            
            DeleteObject(hBrush);
        }
    }
}