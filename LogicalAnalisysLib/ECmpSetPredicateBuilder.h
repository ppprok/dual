#pragma once
#include "Object.h"
#include "CmpMatrixBuilder.h"
#include "containers.h"
#include "Pruning.h"
#include "CoverCollectors.h"
#include "RUNC.h"
#include "ECmpSetPredicate.h"

// Поиск тупиковых корректных предикатов с наибольшей информативностью
class ECmpSetPredicateBuilder
{
    CmpMatrixBuilder _cmpMatrixBuilder;

    ECmpSetPredicates _predicates;
    
public:

    // Информация для обучения
    struct TrainInfo
    {
        // Объекты, для которых необходимо получить корректный результат
        ObjectPtrList correctObjects;
        // Объекты, на которых корректность может быть нарушена
        ObjectPtrList optionalObjects;
        // Объекты, на которых определяется качество по сумме их весов
        ObjectPtrList weightedObjects;

        void Clear()
        {
            correctObjects.clear();
            weightedObjects.clear();
            optionalObjects.clear();
        }
    };

    ECmpSetPredicates& GetPredicates();

    ECmpSetPredicates const& GetPredicates() const;

    // Инициализировать объектами и локальным базисом
    void Init(
        ObjectPtrList const& positive, 
        ObjectPtrList const& negative,
        ECmpSet localBasis,
        int maxOptional = 0);

    // Найти наилучшие предикаты не более count штук
    void Train(
        TrainInfo const& positive, 
        TrainInfo const& negative, 
        int count);

};