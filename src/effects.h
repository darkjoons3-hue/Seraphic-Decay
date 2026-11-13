#pragma once
#include <windows.h>
#include <gdiplus.h>

// Пространство имен для наших эффектов
namespace Effects {
    // Инициализация GDI+ (вызывается один раз в main)
    void Init();
    void Shutdown();

    // Рисование 3D-куба (перенесли из main)
    void Draw3DCube(Gdiplus::Graphics& graphics, int cx, int cy, float angle);

    // ТУТ БУДУТ БУДУЩИЕ ЭФФЕКТЫ (пока пустые заготовки)
    void Glitch(int intensity);
    void Pixelate(int intensity);
    void InvertColors();
}
