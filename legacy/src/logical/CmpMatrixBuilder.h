#pragma once

#include <map>

#include "ECmp.h"
#include "bit_matrix.h"
#include <assert.h>
#include "containers.h"


typedef std::map<Object*, int> ObjectPositionMap;

// Информация о подматрице матрицы сравнения
struct CmpMatrixSubmatrix
{
    UTILITY_MOVE_DEFAULT_MEMBERS(CmpMatrixSubmatrix, (Rows)(Positive)(Negative)(OptionalPositive)(OptionalNegative));    
    
    CmpMatrixSubmatrix(){};

    // Строки подматрицы
    bit_chunks Rows;
    
    // Положительные объекты, содержащиеся в подматрице
    ObjectPtrList Positive;
    
    // Отрицательные объекты, содержащиеся в подматрице
    ObjectPtrList Negative;
    
    // Положительные объекты, для которых заведены столбцы исключения
    ObjectPtrList OptionalPositive;
    
    // Отрицательные объекты, для которых заведены столбцы исключения
    ObjectPtrList OptionalNegative;

    // Найти номер строки для пары объектов
    int GetRowNum(int pos, int neg)
    {
        assert(inside(Positive, pos));
        assert(inside(Negative, neg));
        return pos * Negative.size() + neg;
    }

    // Найти положительный объект по номеру строки
    int GetPosNum(int row)
    {
        assert(inside(Rows, row));
        return row / Negative.size();
    }

    // Найти отрицательный объект по номеру строки
    int GetNegNum(int row)
    {
        assert(inside(Rows, row));
        return row % Negative.size();
    }
};

/// Построение матрицы сравнения и её подматриц
class CmpMatrixBuilder
{
    
    // Построенная матрица сравнения
    bit_matrix _cmpMatrix;

    // Порядок положительных объектов
    ObjectPositionMap _positivePositions;
    
    // Порядок отрицательных объектов
    ObjectPositionMap _negativePositions;

    // Число зарезервированных столбцов
    int _reservedColumns;

    // Локальный базис из эл.кл. и отношений
    ECmpSet _localBasis;
    
    // Построить словарь позиций объектов в матрице
    int MakeObjectPositionMap(ObjectPtrList const& objects, ObjectPositionMap& posMap);

    // Построить словарь позиций столбцов опциональных объектов
    int MakeOptionalObjectPositionMap( ObjectPtrList const& optional, ObjectPtrList const& objects, ObjectPositionMap& posMap, ObjectPtrList& registered);

    // Определить положение объекта по словарю
    int GetObjectPosition(ObjectPositionMap const& posMap, ObjectPtr const& o);

    int GetOptionalObjectPosition(ObjectPtr const& o, 
        ObjectPtrList const& optional,
        ObjectPositionMap& registeredPositions, 
        ObjectPtrList& registered);
    
    // Определить положение объекта по словарю
    int GetObjectsPositions(
        ObjectPositionMap const& posMap, 
        ObjectPtrList const& objects,
        ObjectPtrList& present,
        Positions& positions);


public:

    CmpMatrixBuilder():
        _reservedColumns(0)
    {
    }

    ECmpSet const& GetLocalBasis() const
    {
        return _localBasis;
    }

    bit_matrix const& GetCmpMatrix() const
    {
        return _cmpMatrix;
    }

    ObjectPositionMap const& GetPositiveObjectPositions() const
    {
        return _positivePositions;
    }

    ObjectPositionMap const& GetNegativeObjectPositions() const
    {
        return _negativePositions;
    }

    // Получить строку матрицы для пары объектов
    bit_chunk GetRow(ObjectPtr const& pos, ObjectPtr const& neg);


    // Построить подматрицу из строк, относящимся к парам объектов из указанных наборов
    // положительных и отрицательных объектов.
    // Для каждого необязательного (optional) объекта из числа зарезервированных столбцов
    // выделяется один столбец, в котором стоят единицы в тех строках, которые соответствуют
    // необязательному столбцу.
    CmpMatrixSubmatrix GetSubmatrix(
        ObjectPtrList const& positive, 
        ObjectPtrList const& negative,
        ObjectPtrList const& optionalPositive,
        ObjectPtrList const& optionalNegative);


    // Построить матрицу сравнения объектов из двух наборов:
    // положительных объектов и отрицательных объектов
    // Для сравнения использовать локальный базис и зарезервировать
    // указанное число столбцов 
    void Build(
        ObjectPtrList const& positive, 
        ObjectPtrList const& negative,
        ECmpSet localBasis,
        int reserveColumns = 0);
    
    // Получить эл.кл. с указанными номерами
    ECmpSet GetECmpSubset(Positions const& positions);

    // Получить список исключенных объектов
    void GetExcludedOptionalObjects(
        CmpMatrixSubmatrix const& submatrix, 
        Positions const& cover, 
        ObjectPtrList& excludedPositive, 
        ObjectPtrList& excludedNegative);
};