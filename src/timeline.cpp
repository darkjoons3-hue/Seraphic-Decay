#include "timeline.h"
#include "effects.h"
#include <mmsystem.h>
#include <string>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <ctime>

#pragma comment(lib, "winmm.lib")

namespace Timeline {
    std::atomic<bool> g_running{false};
    std::atomic<bool> g_isPaused{false};
    DWORD g_startTime = 0;

    // --- РАБОТА С РЕЕСТРОМ ---
    void SetRegDword(HKEY root, const wchar_t* sub, const wchar_t* val, DWORD data) {
        HKEY h; if (RegCreateKeyExW(root, sub, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &h, nullptr) == ERROR_SUCCESS) {
            RegSetValueExW(h, val, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
            RegCloseKey(h);
        }
    }
    DWORD GetRegDword(HKEY root, const wchar_t* sub, const wchar_t* val) {
        HKEY h; DWORD data = 0, sz = sizeof(data);
        if (RegOpenKeyExW(root, sub, 0, KEY_READ, &h) == ERROR_SUCCESS) {
            RegQueryValueExW(h, val, nullptr, nullptr, (LPBYTE)&data, &sz);
            RegCloseKey(h);
        }
        return data;
    }

    // --- МУЗЫКА ---
    void PlayMusic() {
        mciSendStringW(L"open \"angel.mp3\" type mpegvideo alias angel", NULL, 0, NULL);
        mciSendStringW(L"play angel from 0", NULL, 0, NULL);
    }

    void StopMusic() {
        mciSendStringW(L"close angel", NULL, 0, NULL);
    }

    // --- ЛОГИКА ПЕРЕЗАПУСКА И ШНУРА ---
    bool CheckPostRebootState() {
        // Проверяем, был ли запущен пранк до перезагрузки
        DWORD isPlaying = GetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"IsPlaying");
        DWORD savedStartTime = GetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"StartTime");

        if (isPlaying == 1) {
            // Считаем, сколько времени прошло с момента старта (в секундах)
            DWORD currentTime = (DWORD)time(NULL);
            DWORD elapsedSeconds = currentTime - savedStartTime;

            // Если прошло больше 2 минут (120 сек) ДО перезагрузки, значит юзер выдернул шнур или тянул время
            if (elapsedSeconds > 120) {
                // ЗДЕСЬ БУДЕТ ТОТ САМЫЙ ЭКРАН "БРО, Я ЖЕ СКАЗАЛ..."
                MessageBoxW(NULL, 
                    L"Бро, я же сказал не надо перезагружаться...\n\nТеперь тебе придется ждать 5 часов. Не скучай.", 
                    L"Seraphic Decay", 
                    MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SYSTEMMODAL);
                
                // Тут мы потом запустим черный экран с таймером на 5 часов.
                // А пока просто выходим.
                SetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"IsPlaying", 0);
                ExitProcess(0);
            } else {
                // Если прошло мало времени, значит это просто ребут. Продолжаем.
                g_startTime = GetTickCount() - (elapsedSeconds * 1000); // Корректируем время
                return true; 
            }
        }
        return false;
    }

    // --- ГЛАВНЫЙ ЦИКЛ ТАЙМЛАЙНА ---
    void TimelineThread() {
        g_running = true;
        PlayMusic();

        // Если это первый запуск, записываем время в реестр
        if (GetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"IsPlaying") == 0) {
            g_startTime = GetTickCount();
            SetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"StartTime", (DWORD)time(NULL));
            SetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"IsPlaying", 1);
        }

        while (g_running) {
            if (!g_isPaused) {
                DWORD elapsedMs = GetTickCount() - g_startTime;
                DWORD elapsedSec = elapsedMs / 1000;

                // ========================================================
                // ТАЙМЛАЙН (ТЕСТОВЫЙ РЕЖИМ - 1 СЕК = 1 СЕКУНДА)
                // В реал. версии мы умножим секунды на нужный коэффициент
                // ========================================================

                // 0:00 - 0:10 (Первые 10 секунд - тишина, экран черный)
                // Тут мы могли бы скрыть рабочий стол, но пока оставим.

                // 0:10 (10 секунд) - Первый глитч
                if (elapsedSec >= 10 && elapsedSec < 11) {
                    Effects::InvertColors(); 
                }

                // 0:20 (20 секунд) - Тестовая проверка (выведем сообщение)
                if (elapsedSec == 20) {
                    MessageBoxW(NULL, L"Прошло 20 секунд трека. Система начинает сходить с ума.", L"Test", MB_OK);
                }

                // 50 МИНУТ = 3000 СЕКУНД (В реале). Для теста поставим 40 секунд.
                if (elapsedSec == 40) {
                    MessageBoxW(NULL, L"Форматирование диска Z: начато... (Симуляция)", L"Seraphic Decay", MB_ICONWARNING);
                }

                // 55:30 = 3330 СЕКУНД (В реале). Конец трека.
                if (elapsedSec >= 60) { // Для теста на 60-й секунде
                    g_running = false;
                    break;
                }
            }
            Sleep(100); // Проверяем каждые 100 мс
        }

        // КОНЕЦ ПЕРФОРМАНСА - ВОССТАНОВЛЕНИЕ
        StopMusic();
        SetRegDword(HKEY_CURRENT_USER, L"Software\\SeraphicDecay", L"IsPlaying", 0);
        
        MessageBoxW(NULL, L"Арт-перформанс завершен. Спасибо за участие.", L"Seraphic Decay", MB_OK | MB_ICONINFORMATION);
        ExitProcess(0); // Закрываем программу
    }

    void Start() {
        std::thread(TimelineThread).detach();
    }

    void Stop() {
        g_running = false;
    }
}
