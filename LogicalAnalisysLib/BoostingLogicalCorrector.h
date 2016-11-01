#pragma once

#include <map>

#include "IMLAlgorithm.h"
#include "ECmp.h"
#include "ECmpSetPredicate.h"
#include "SamplesWeighter.h"
#include "CmpMatrixBuilder.h"
#include "containers.h"
#include "ECmpSetPredicateBuilder.h"
#include "LocalBasisBuilder.h"
#include "Randomizer.h"
#include "math_utils.h"


// Обучение логического корректора методом бустинга
class BoostingLogicalCorrector
    :public MLAlgorithmBase
{

public:

    // Режим распознавания
    enum Mode
    {
        // Базовый режим
        Basis,
        // Аддитивный режим
        Additive,
    };

    // Информация о веса класса
    struct ClassWeightInfo
    {
        // Метка класса
        int label;
        // Использовать объекты не из класса
        bool complement;
        // Вес объектов класса
        Weight positiveWeight;
        // Вес объектов не из класса
        Weight negativeWeight;
        // Ненормированная сумма всех весов пар объект -- класс
        Weight Q;

        ClassWeightInfo(int l, bool c = false)
        {
            label = l;
            complement = c;
            positiveWeight = 0;
            negativeWeight = 0;
            Q = 0;
        }

		// Значение делта для класса, вычисленное по сумме весов положительных и отрицательных выделяемых объектов 
        Weight GetDelta(Weight p, Weight n, Weight eps) const
        {
            return (GetTotalWeight() / Q) * psi2(n, p, eps);
        }

		// Найти величину разницы между текущим значением делта и необходимым значением
		// Необходимо, чтобы этот показатель был больше нуля
		Weight GetDeltaDiff(Weight p, Weight n, Weight eps, Weight delta) const
		{
			return psi(p, n, eps) - sqrt(delta * Q / GetTotalWeight());
		}

		// Максимальное знаение делта у предикатов, корректных для класса 
		Weight GetMaxDelta(Weight eps) const
		{
			return GetDelta(GetNormalizedPositiveWeight(), 0, eps);
		}

		Weight GetTotalWeight() const
		{
			return positiveWeight + negativeWeight;
		}

		Weight GetNormalizedPositiveWeight() const
		{
			return GetTotalWeight() != 0 ? positiveWeight / GetTotalWeight() : 0;
		}

		Weight GetNormalizedNegativeWeight() const
		{
			return GetTotalWeight() != 0 ? negativeWeight / GetTotalWeight() : 0;
		}

		static Weight psi2(Weight p, Weight n, Weight eps)
		{
			return sqr(psi(p, n, eps));
		}

		static Weight psi(Weight p, Weight n, Weight eps)
		{
			return sqrt(p) - sqrt(n+eps);
		}
    };

    // Опции обучения и распознавания
    struct Options
    {
        // Число итераций
        int iterationsCount;

        // Локальный базис
        ECmpSet localBasis;

        // Число необязательных объектов
        int optionalObjectsCount;

        // Параметр вычисления веса предиката
        Weight epsilon;

        // Режим распознавания
        Mode mode;

		// Значение Q после которого следует остановиться
		Weight stopQ;

        Options()
        {
            mode = Basis;
            optionalObjectsCount = 0;
            iterationsCount = 0;
            epsilon = 0;
			stopQ = 0;
        }

        UTILITY_MOVE_DEFAULT_MEMBERS(Options, 
            (mode)(optionalObjectsCount)(iterationsCount)(localBasis)(epsilon)(stopQ));

        //Options(Dataset const& ds);
    };

private:

    typedef std::map<int, ECmpSetPredicates> PredicatesMap;

    // Предикаты, участвующие в голосовании
    PredicatesMap _predicates;

    // Текущие опции обучения и расознавания
    Options _currentOptions;

    Randomizer _randomizer;

public:

    // Рекоммендуемые опции обучения и распознавания
    Options RecommendedOptions;
    
    // Сбросить логический корректор
    virtual void Reset();

    // Обучиться по выборке
    virtual void Train(Dataset const& ds);


    // Применить классификатор к объекту
    virtual Prediction Apply(Object const& o) const;

private:

    // Определить наборы взвешенных положительных и отрицательных объектов, 
    // по которым определяется информативность предиката
 /*   void AssignWeightedObjects(Dataset const &ds, SelectedClassInfo &cls, 
        ECmpSetPredicateBuilder::TrainInfo &positive, 
        ECmpSetPredicateBuilder::TrainInfo &negative);

    // Выбрать случайные опциональны объекты
    void SelectOptionalPositiveObjects(
        SamplesWeighter &weighter, 
        SelectedClassInfo cls, int count,
        ECmpSetPredicateBuilder::TrainInfo &positive);

    // Вычислить веса объектов по отступам
    void CalculateWeightsByMargins(Dataset const &ds, Predictions &predictions);*/

    // Инициализировать семейства предикатов по классам
    void InitClassesPredicates(Dataset const &ds);
        
    // Скорректировать текущие опции по обучающей выборке
    Options CreateCurrentOptions(Dataset const& ds);

    
    // Собрать информацию о весах по классам
    std::vector<ClassWeightInfo> CollectClassWeightInfo(
        Classes const& classes,
        Predictions const& predictions, Weight eps);

    // Выбрать подходящий класс
    ClassWeightInfo SelectClass(Dataset const &ds, Predictions const& predictions, Weight delta, Weight eps);

    // Посчитать веса объектов для выбранного класса
    void WeightObjects(Dataset const& ds, Predictions const& predictions, ClassWeightInfo const& cls);
    
    // Выбрать объекты для обучения
    void SelectTrainObjects(Dataset const& ds, BoostingLogicalCorrector::ClassWeightInfo const& cls, ECmpSetPredicateBuilder::TrainInfo& positive, ECmpSetPredicateBuilder::TrainInfo& negative, Weight delta, Weight epsilon, int optionalObjectsCount);

	// Выбрать объекты для обучения
	void SelectTrainObjectsByIncrease(Dataset const& ds, BoostingLogicalCorrector::ClassWeightInfo const& cls, ECmpSetPredicateBuilder::TrainInfo& positive, ECmpSetPredicateBuilder::TrainInfo& negative, Weight delta, Weight epsilon, int optionalObjectsCount);

	int CollectOptionalObjects(int optionalObjectsCount, ECmpSetPredicateBuilder::TrainInfo &positive);

};