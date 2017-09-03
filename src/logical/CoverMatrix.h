#pragma once
#include "FitMatrix.h"

// Матрица для накопления взвешенных покрытий матрицы сравнения
class CoverMatrix    
{
    // Число столбцов
    int _n;

    // Найденные покрытия
    weighted_bit_matrix _covers;
        
    typedef std::pair<Weight, int> WeightOrder;

    typedef std::vector<WeightOrder> WeightOrders;

    // Временный массив для сортировки по весам
    WeightOrders _weightOrders;

public:

    CoverMatrix();

    // Установить число столбцов
    void SetWidth(int n);

    // Получить доступ к найденным покрытиям
    weighted_bit_matrix& GetCovers();
    
    // Получить доступ к найденным покрытиям
    weighted_bit_matrix const& GetCovers() const;

    // Добавить покрытие с весом
    void AddCover(bit_chunk cover, Weight w);

    // Оставить count покрытий с наибольшими весами
    void RestMost(int count);
    
};

