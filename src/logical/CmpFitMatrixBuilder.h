#pragma once
#include "CmpMatrix.h"
#include "ObjectsClass.h"
#include "CoverMatrix.h"
#include "Configurable.h"


struct CmpFitMatrixBuilderOptions
{
    // Максимальное число настроечных строк
    int fitHeightLimit;
    
    // Максимальное число строк матрицы сравнения
    int cmpHeightLimit;
    
    // Порог числа настроечных строк, при котором необходимо производить слияние
    int fitHeightThreshold;

    // Порог числа строк матрицы сравнения, при котором необходимо производить слияние
    int cmpHeightThreshold;

    // Ограничение на число лучших покрытий
    int coverLimit;

    // Порог числа покрытий, при которым надо отбирать лучшие
    int coverThreshold;

    // Максимальное число итераций дуализации
    int iterationLimit;

    // Ограничение на число настроечных объектов
    int fitObjectLimit;

    CmpFitMatrixBuilderOptions()
    {
        fitHeightLimit = 20;
        fitHeightThreshold = 50;
        fitObjectLimit = 200;
        
        cmpHeightLimit = 20;
        cmpHeightThreshold = 50;
        
        coverLimit = 5;
        coverThreshold = 500;
        iterationLimit = 100000;
    }
};

struct DualCallback;

// Совместное построение матрицы сравнения и настроечной матрицы
class CmpFitMatrixBuilder
    :public IConfigurable
{
    // Матрица сравнения
    CmpMatrix _cmpMatrix;

    // Настроечная матрица
    FitMatrix _fitMatrix;

    // Матрица с покрытиями
    CoverMatrix _coverMatrix;

public:

    // Опции построения матриц и поиска их покрытий
    CmpFitMatrixBuilderOptions Options;

    CoverMatrix& GetCoverMatrix()
    {
        return _coverMatrix;
    }

    // Установить сравниватель
    void SetComparer(IComparerPtr comparer);

    // Посроить матрицы сравнения и настройки
    void Build(ObjectsClassRange const& correct, ObjectsClassRange const& fitting);

    // Найти покрытия матрицы сравнения
    void FindCovers();
    
    // Установить опции
    virtual void SetOptions(::Options const& options );

    // Установить опции по умолчанию
    virtual void SetDefaultOptions();

private:

    void RestMostCovers();

    void RestMostCovers(int thres, int limit);
    
    void MergeFitMatrixRows(int thres, int limit);

    void MergeCmpMatrixRows(int thres, int limit);

    Weight GetCoverWeight(bit_chunk const& cover) const;

    friend DualCallback;
};