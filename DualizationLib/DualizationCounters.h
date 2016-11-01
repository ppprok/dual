#pragma once
#include <string>

// Счётчики результатов выполнения дуализации
struct DualizationCounters
{
    typedef __int64 Int;

    Int results;    ///< Число найденных решений
    Int inners;     ///< Число внутренних состояний
    Int extras;     ///< Число лишних шагов
    Int currentDelay;   ///< Текущая задержка
    Int maxDelay;       ///< Максимальная задержка

    // Сбросить счётчики
    void Reset();

    // Конструктор, обнуляющий счётчики
    DualizationCounters();

    // Преобразовать счётчики в текстовый вид
    std::string ToString() const;

    void IncResult()
    {
        ++results;
        if (results == 1)
            maxDelay = currentDelay;
        else
            if (maxDelay < currentDelay)
                maxDelay = currentDelay;
        currentDelay = 0;
    }

    void IncExtra()
    {
        ++extras;
        ++currentDelay;
    }

    void IncInners()
    {
        ++inners;
        ++currentDelay;
    }
};
