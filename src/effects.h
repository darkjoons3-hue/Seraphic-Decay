#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

namespace Effects {
    void Init();
    void Shutdown(); // <-- Добавили функцию для правильного завершения
    void Draw3DCube(Gdiplus::Graphics& graphics, int cx, int cy, float angle);
    void Glitch(int intensity);
    void Pixelate(int intensity);
    void InvertColors();
}
