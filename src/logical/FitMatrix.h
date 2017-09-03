#pragma once
#include "Object.h"
#include "bit_matrix.h"
#include "Comparers.h"
#include "bit_vector.h"



typedef bit_matrix_<std::vector<weighted_bit_chunk<Weight> > > 
    weighted_bit_matrix;


// Настроечная матрица
class FitMatrix
{
    // Объект, осуществляющий сравнения объектов
    IComparerPtr _comparer;

    // Временная строка для сохранения результата сравнения объектов
    bit_vector _cmpRow;

    // Матрица сравнения для настройки
    weighted_bit_matrix _fitMatrix;

    // Число столбцов
    int _n;

    // Сумма весов строк, которые можно покрыть столбцом
    Weights _columnWeights;

public:

    FitMatrix();

    void SetComparer(IComparerPtr comparer);

    // Найти сумму весов покрытых строк
    Weight GetCoveredRowsWeight(bit_chunk cover) const;

    // Найти сумму весов общих столбцов
    Weight GetCommonColumnsWeight(bit_chunk const& c1, bit_chunk const& c2) const;

    // Найти сумму весов отличающихся столбцов
    Weight GetDiffColumnsWeight(bit_chunk const& c1, bit_chunk const& c2) const;

    // Получить доступ к построенной матрице
    weighted_bit_matrix& GetFitMatrix();

    // Получить доступ к построенной матрице
    weighted_bit_matrix const& GetFitMatrix() const;

    // Добавить объект, по которому оценивается качество
    void AddFittingObject(Object const& precedent, Object const& o, Weight w);

    // Добавить объекты, по которым оценивается качество
    void AddFittingObjects(Object const& precedent, ObjectsRange const& objects, Weight w);

    // Добавить объект, по которому оценивается качество по набору прецедентов
    void AddFittingObject(ObjectsRange const& precedents, Object const& o, Weight w);

    // Добавить объекты, по которым оценивается качество по набору прецедентов
    void AddFittingObjects(ObjectsRange const& precedents, ObjectsRange const& objects, Weight w);

    // Удалить строки, охватывающие указанную строку
    void RemoveSupRows(bit_chunk row);
    
    // Удалить строки, охватывающие хотя бы одну из указанных строк
    template<typename Rows>
    void RemoveSupRows(Rows const& rows)
    {
        for (auto const& row:rows)
        {
            if (_fitMatrix.height() == 0)
                break;
            RemoveSupRows(row);
        }
    }

    // Удалить строки, покрытые указанной строкой
    void RemoveCoveredRows(bit_chunk cover);

    // Удалить строки с нулевыми весами
    void RemoveZeroWeightRows();

    // Слить похожие строки, оставив не более restCount строк
    void MergeSimilarRows(int restCount);

private:
    
    // Изменить веса столбцов после изменения веса строки
    void UpdateColumnWeights(bit_chunk row, Weight deltaW);

};
