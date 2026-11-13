#include "effects.h"

using namespace Gdiplus;

namespace Effects {

    void Init() {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }

    void Shutdown() {
        // Заглушка, GDI+ выключается в main
    }

    void Draw3DCube(Graphics& graphics, int cx, int cy, float angle) {
        // Вершины куба (x, y, z)
        float vertices[8][3] = {
            {-50, -50, -50}, { 50, -50, -50}, { 50,  50, -50}, {-50,  50, -50},
            {-50, -50,  50}, { 50, -50,  50}, { 50,  50,  50}, {-50,  50,  50}
        };

        float radX = angle * 0.5f;
        float radY = angle;

        PointF projected[8];
        for (int i = 0; i < 8; ++i) {
            float x1 = vertices[i][0] * cos(radY) - vertices[i][2] * sin(radY);
            float z1 = vertices[i][0] * sin(radY) + vertices[i][2] * cos(radY);
            float y2 = vertices[i][1] * cos(radX) - z1 * sin(radX);
            float z2 = vertices[i][1] * sin(radX) + z1 * cos(radX);

            float distance = 200.0f;
            float fov = distance / (distance + z2);
            projected[i].X = cx + x1 * fov;
            projected[i].Y = cy + y2 * fov;
        }

        Pen pen(Color(255, 80, 255, 80), 2.0f);
        int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };
        for (int i = 0; i < 12; ++i) {
            graphics.DrawLine(&pen, projected[edges[i][0]], projected[edges[i][1]]);
        }
    }

    // --- ЗАГОТОВКИ ДЛЯ БУДУЩЕГО ---
    void Glitch(int intensity) {
        // Здесь будет код инверсии экрана и сдвига пикселей
    }
    void Pixelate(int intensity) {
        // Здесь будет код пикселизации
    }
    void InvertColors() {
        // Инверсия
    }
}
