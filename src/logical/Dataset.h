#pragma once
#include "Object.h"
#include "ObjectsClass.h"
#include "Predictions.h"

struct ILearnableClassifier;

typedef std::vector<int> Labels;

// Набор прецедентов
struct Dataset
{
    // Все объекты
	ObjectPtrList objects;

	// Классы объектов
    ObjectsClasses classes;

    // Значения признаков, встречающиеся у объектов
    FeatureValues featureValues;

    // Сохранённые веса объектов
    Weights weights;

    // Сохраненные метки объектов
    Labels labels;

    Dataset();

    //UTILITY_MOVE_DEFAULT_MEMBERS(Dataset, (objects)(classes)(featureValues));
    
    // Собрать информацию об объектах в классах
    ObjectsClasses const& CollectClasses();

    // Сформировать признаки--значения
    FeatureValues const& CollectFeatureValues();
    
    // Добавить признаки--значения объектов указанных классов
    FeatureValues const& AddFeatureValues(ObjectsClasses const& classes);

    // Добавить признаки--знаяения указанных объектов
    FeatureValues const& AddFeatureValues(ObjectPtrList const& objects);

	// Загрузить из файла
    bool Load(std::string const& filename);

	// Перетасовать объекты случайным образом
	void RandomShuffle();
    
	    
    // Добавить пары признак--значение для объекта
    void AddFeatureValues(Object const& o);

    // Найти класс по метке
    ObjectsClass const& GetClass(int label) const;

    // Упорядочить объекты по весу
    void OrderByWeight(ObjectPtrList& objects) const;

	// Упорядочить объекты по убыванию веса
	void OrderByWeightDescent(ObjectPtrList& objects) const;
    
    void Clear();
   
    static void TakeRange( ObjectPtrList const& objects, float from, float to, ObjectPtrList& output );
    
    static void TakeRange( ObjectsClasses const& classes, float from, float to, ObjectPtrList& output );
    
    void TakeRange( float from, float to, ObjectPtrList& output );
    
    std::pair<Dataset, Dataset> Split(float p1, float p2 ) const;

    Weights& StoreWeights();

    void RestoreWeights(Weights const& weights );

    void RestoreWeights();

    // Сохранить метки объектов
    Labels& StoreLabels();

    // Восстановить метки объектов
    void RestoreLabels(Labels const& labels );

    
    // Восстановить метки объектов
    void RestoreLabels();

    // Переопределить метки объектов с помощью отображения
    template <typename Map>
    void RemapLabels(Map map)
    {
        for (auto& o : objects)
            o->label = map(*o);
    }

    // Удалить объекты, у которых одинаковое описание, но разные метки
    bool RemoveUncorrect();


};



