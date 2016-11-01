#pragma once

#include "Object.h"
#include "Comparers.h"
#include "FitMatrix.h"

// Построение матрицы сравнения
class CmpMatrix    
{
    // Матрица сравнения
    bit_matrix _cmpMatrix;

    // Временная строка для сохранения результата сравнения объектов
    bit_vector _cmpRow;

    // Число столбцов
    int _n;

    // Сравниватель объектов
    IComparerPtr _comparer;
        
public:

    void SetComparer(IComparerPtr comparer);
    
    CmpMatrix();

    // Текущая матрица сравнения
    bit_matrix& GetCmpMatrix();    
    
    // Добавить сравниваемые объекты
    void AddComparableObjects(Object const& po, Object const& no);
        
    // Добавить сравниваемые объекты
    void AddComparableObjects(Object const& po, ObjectsRange const& negative);

    // Добавить пары сравниваемых объектов
    void AddComparableObjects(ObjectsRange const& positive, 
        ObjectsRange const& negative);

    // Слить похожие строки
    void MergeSimilarRows(int restCount, FitMatrix const& fitMatrix);

    // Добавить новую строку
    void AddRow(bit_chunk const& row);
};

