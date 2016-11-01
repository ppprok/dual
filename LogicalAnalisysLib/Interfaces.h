#pragma once

#include <boost/program_options.hpp>

#include "Predictions.h"
#include "Object.h"
#include "Dataset.h"
#include "ECmp.h"
#include "InformativityCondition.h"

//////////////////////////////////////////////////////////////////////////
// Бинарный классификатор объектов
//////////////////////////////////////////////////////////////////////////
struct IBinaryClassifier
{
    virtual Estimate Apply(Object const& o) const = 0;

    virtual ~IBinaryClassifier(){};
};

typedef std::shared_ptr<IBinaryClassifier> IBinaryClassifierPtr;


//////////////////////////////////////////////////////////////////////////
// Информация по одному из двух классов
// для обучения бинарного классификатора
//////////////////////////////////////////////////////////////////////////
/*struct ClassTrainInfo
{
    // Объекты, для которых необходимо получить корректный результат
    ObjectPtrList correctObjects;
    // Объекты, на которых корректность может быть нарушена
    ObjectPtrList optionalObjects;
    // Объекты, на которых определяется качество по сумме их весов
    ObjectPtrList fittingObjects;

    ClassTrainInfo(){};

    void Clear()
    {
        correctObjects.clear();
        fittingObjects.clear();
        optionalObjects.clear();
    }

    UTILITY_MOVE_DEFAULT_MEMBERS(ClassTrainInfo, (correctObjects)(fittingObjects)(optionalObjects));
};*/



//////////////////////////////////////////////////////////////////////////
// Методо обучения бинарного классификатора
//////////////////////////////////////////////////////////////////////////
struct IBinaryClassifierLearner
{
    
    virtual IBinaryClassifierPtr Train(
        ObjectsClassRange const& correct,
        ObjectsClassRange const& fitting) = 0;

    virtual void Reset() = 0;

    virtual ~IBinaryClassifierLearner(){};
};

typedef std::shared_ptr<IBinaryClassifierLearner> IBinaryClassifierLearnerPtr;


//////////////////////////////////////////////////////////////////////////
// Классификатор объектов на несколько классов
//////////////////////////////////////////////////////////////////////////
struct IClassifier
{
    virtual Prediction Apply(Object const& o) const = 0;

    virtual Predictions ApplyMany(ObjectsRange const& objects) const = 0;

    virtual ~IClassifier(){};
};

typedef std::shared_ptr<IClassifier> IClassifierPtr;

//////////////////////////////////////////////////////////////////////////
// Интерфейс обучаемого алгоритма классификации
//////////////////////////////////////////////////////////////////////////
struct IClassifierLearner
{
	virtual IClassifierPtr Train(Dataset const& ds) = 0;

    virtual void Reset() = 0;
    	
    virtual ~IClassifierLearner(){};
};

typedef std::shared_ptr<IClassifierLearner> IClassifierLearnerPtr;

struct IFeatureSelector;

typedef std::shared_ptr<IFeatureSelector> IFeatureSelectorPtr;

// Интерфейс наследуется алгоритмами, которые нуждаются в 
// выборе признаков и их значений в процессе обучения
struct INeedFeatureSelector
{
    // Установить селектор признаков
    virtual void SetFeatureSelector(IFeatureSelectorPtr selector) = 0;

    // Получить текущий селектор признаков
    virtual IFeatureSelectorPtr GetFeatureSelector() = 0;

    virtual ~INeedFeatureSelector(){};
};


typedef std::vector<int> Features;

//////////////////////////////////////////////////////////////////////////
// Выполнение отбора признаков, пар признак--значение, наборов эл.кл. и
// наборов эл.кл. с отношениями
//////////////////////////////////////////////////////////////////////////
struct IFeatureSelector
{
    virtual ~IFeatureSelector(){};

    // Сброс свей информации
    virtual void Reset() = 0;
        
    // Инициализация с использованием объектов выборки
    virtual void Init(Dataset const& ds) = 0;

    // Отобрать признаки по корректной и настроечной частям
    virtual void Select(
        ObjectsClassRange const& correct, 
        ObjectsClassRange const& fitting) = 0;

    // Отобрать признаки для различения объектов класса
    virtual void Select(ObjectsClassRange const& cls) = 0;

    // Отобрать признаки, подходящие для различения наборов указанных объектов
    virtual Features const& GetFeatures() const = 0;

    // Отобрать эл.кл. с отношениями, подходящие для различения наборов указанных объектов
    virtual ECmpSet const& GetECmpSet() const = 0;

};

typedef std::shared_ptr<IFeatureSelector> IFeatureSelectorPtr;


//////////////////////////////////////////////////////////////////////////
// Типовая реализация части методов обучаемого алгоритма классификации
//////////////////////////////////////////////////////////////////////////
class ClassifierBase
    :public IClassifier
{
public:

    virtual Predictions ApplyMany( ObjectsRange const& objects ) const
    {
        Predictions predictions;
        predictions.reserve(objects.size());

        for(auto& o:objects)
        {
            predictions.push_back(Apply(*o));
        }

        return std::move(predictions);

    }

};




inline bool CheckCorrectness(IBinaryClassifierPtr bincls, 
                            ObjectPtrList const& positive,
                            ObjectPtrList const& negative)
{
    for(auto po: positive)
        if (bincls->Apply(*po) <= 0)
            return false;
    for(auto no: negative)
        if (bincls->Apply(*no) > 0)
            return false;

    return true;
}


// Алгоритм, основанный на голосовании по эл.кл.
struct IECVoting
{
    // Сбор эл.кл., по которым осуществляется распознавание
    virtual void GetECSet(ECSet& ecset) = 0;

    virtual ~IECVoting(){};
};