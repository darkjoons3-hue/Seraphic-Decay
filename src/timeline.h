#pragma once
#include <windows.h>

namespace Timeline {
    // Запускает перформанс (музыку и таймлайн)
    void Start();
    
    // Останавливает всё (например, по нажатию секретной кнопки)
    void Stop();
    
    // Проверяет, было ли это перезапуском после ребута
    // Возвращает true, если мы продолжаем пранк, и false, если это первый запуск
    bool CheckPostRebootState();
}
