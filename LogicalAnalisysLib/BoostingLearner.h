#pragma once


#include "Interfaces.h"
#include "ECmp.h"
#include "CmpMatrixBuilder.h"
#include "containers.h"
#include "ECmpVotingLearner.h"
//#include "LocalBasisBuilder.h"
#include "Randomizer.h"
#include "math_utils.h"


// Опции обучения бустингом
struct BoostingOptions
{
    // Число итераций
    int iterationsCount;
    
    // Значение Q после которого следует остановиться
    Weight stopQ;

    // Число положительных объектов
    int positiveCount;
    
    // Часть положительных объектов
    Weight positivePart;

    // Минимальный вес классификатора
    Weight minWeight;

    BoostingOptions();
};


// Информация о весах класса, использующаяся бустингом
struct BoostingClassInfo
{
    // Метка класса
    int label;

    // Использовать объекты не из класса
    bool complement;

    // Вес объектов класса
    Weight positiveWeight;

    // Вес объектов не из класса    
    Weight negativeWeight;

    // Сумма всех весов пар объект--класс
    Weight Q;

    // Параметры вычисления информативности 
    InformativityCondition informativity;

    Weight GetNormalizedNegativeWeight() const;

    Weight GetNormalizedPositiveWeight() const;

    Weight GetTotalWeight() const;

    // Максимальное значение информативности, которое можно достигнуть на классе
    Weight GetClassWeight() const;

    // Максимальное значение информативности, которое можно достигнуть на классе
    Weight GetClassWeight(Weight threshold) const;

    // Получить описание дополнения класса
    BoostingClassInfo GetComplement() const;

    BoostingClassInfo(int l = -1, bool c = false);


    Weight NormalizeWeight(Weight w) const;


    // Посчитать вес предиката по весам положительных выделяемых и отрицательных выделяемых объектов
    Weight GetWeakClassifierWeight(Weight p, Weight n);


    void AddPositiveWeight(Weight w);


    void AddNegativeWeight(Weight w);

};





// Обучение логического корректора методом бустинга
class BoostingLearner
    :public IClassifierLearner,
    public IConfigurable,
    private boost::noncopyable
{
    
    // Информация о весах классов и их дополнениях
    std::vector<BoostingClassInfo> _weightedClasses;

    // Временный массив, в который выбирается класс
    std::vector<BoostingClassInfo> _selectedClasses;

    // Часть объектов, на которых нельзя ошибаться
    ObjectsClassRange _correctPart;
    
    // Часть объектов, на распознавание которых желательно настроиться
    ObjectsClassRange _fittingPart;

public:

    // Метод обучения бинарного алгоритма для выделения объектов класса
    IBinaryClassifierLearnerPtr positiveLearner;

    // Метод обучения бинарного алгоритма для выделения объектов не из класса
    IBinaryClassifierLearnerPtr negativeLearner;

    // Рекомендуемые опции обучения и распознавания
    BoostingOptions CurrentOptions;
    
    // Сбросить логический корректор
    virtual void Reset();

    // Обучиться по выборке
    virtual IClassifierPtr Train(Dataset const& ds);

    // Установить опции по умолчанию
    virtual void SetDefaultOptions();
        
    // Установить опции
    virtual void SetOptions( Options const& options );

    // Зарегистрировать тип в реестре для инстанцирования по имени
    static void Register();

    // Пустой конструктор
    BoostingLearner();
    
private:

    // Вычислить рекомендуемое число итераций, достаточное для построения корректного алгоритма
    int CalculateIterationCount(Dataset const &ds);

    // Инициализировать семейства предикатов по классам
    void InitClassesPredicates(Dataset const &ds);
        
    // Собрать информацию о весах по классам
    void CollectClassWeightInfo( ObjectsClasses const& classes, Predictions const& predictions, Weight delta);

    // Выбрать подходящий класс
    BoostingClassInfo SelectClass( Dataset const &ds, Predictions const& predictions, Weight delta);
    
    // Посчитать веса объектов для выбранного класса
    void WeightObjects(Dataset const& ds, Predictions const& predictions, BoostingClassInfo const& cls);
    
    // Вычислить вес бинарного классификатора по качеству распознавания положительных и отрицательных объектов
    Weight WeightBinaryClassifier( BoostingClassInfo &cls, IBinaryClassifierPtr bincls, ObjectPtrList const& positive, ObjectPtrList const& negative ) const;
    
    // Инициализировать выбор признаков в бинарных алгоритмах распознавания
    void InitFeatureSelector( Dataset const& ds );

    // Получить число необходимых положительных объектов для обучения
    int GetCorrectPositiveObjectsCount(int mk) const;

    // Сформировать наборы корректных объектов для обучения бинарного классификатора
    void CollectCorrectObjects(ObjectsClass& clsref, BoostingClassInfo& clsinfo);
    
    void SetupDefaultLearners();
};