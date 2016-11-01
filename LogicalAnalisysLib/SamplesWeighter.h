#pragma once
#include <algorithm>
#include <boost\random\uniform_real.hpp>
#include <boost\random.hpp>

#include "Object.h"
#include "containers.h"
#include <map>

// Информация о выбранном классе или его дополнении
struct SelectedClassInfo
{
    int label;  // метка класса
    Weight weight;  // суммарный вес объекта класса
    bool negative; // сам класс или его дополнение

    SelectedClassInfo(int l, Weight w, bool neg)
    {
        label = l;
        weight = w;
        negative = neg;
    }
};


// Инструменты для работы с весами объектов
class SamplesWeighter
{

    // Словарь, связывающий класс с весом его объектов
    typedef std::map<int, Weight> ClassWeightMap;
    
    // Объекты с весами
    ObjectPtrList _objects;

    // Текущая сумма весов
    Weight _sumWeight;

    //////////////////////////////////////////////////////////////////////////
    // Генераторы случайных чисел
    //////////////////////////////////////////////////////////////////////////
    typedef boost::minstd_rand stdrand;
    typedef boost::uniform_real<Weight> uni_dist;
    typedef boost::variate_generator<stdrand, uni_dist> uni_generator;

    // Генератор случайных чисел, равномерный из [0,1]
    mutable uni_generator _rand;

    // Веса классов
    ClassWeightMap _classWeights;

private:

    void OnUpdateWeight(ObjectPtr const& o);

    void OnUpdateWeights(ObjectPtrList const& objects);

    void OnUpdateWeights();


public:

    SamplesWeighter():_rand(stdrand(), uni_dist()){};

    // Получить веса классов
    ClassWeightMap const& GetClassWeights() const;

    // Установить объекты
    void SetObjects(ObjectPtrList objects);

    // Обновить суммарный вес
    void UpdateWeightSum();

    // Сбросить значение суммы весов
    void ResetWeightSum()
    {
        _sumWeight = 0;
    }

    // Получить объекты
    ObjectPtrList const& GetObjects() const;
    
    // Установить одинаковый вес
    void SetWeight(Weight w);

    // Получить сумму весов
    Weight GetWeightSum() const;

    // Нормировать веса, чтобы их сумма была единичной
    void NormalizeWeights();

    // Перемешать в случайном порядке
    void RandomShuffle();

    // Выбрать n случайных объектов
    ObjectPtrList GetRandomSamples(int n) const;

    // Выбрать n случайных объектов выбранного класса
    ObjectPtrList GetRandomSamples(int n, SelectedClassInfo const& cls) const;

    // Вернуть случайно выбранный объект, используя веса как распределение вероятностей
    ObjectPtr GetRandomSample() const;

    // Упорядочить выборку по возрастанию весов
    void OrderSamples();

    // Упорядочить выборку по убыванию весов
    void OrderSamplesDescend();

    // Удалить нулевые указатели
    void RemoveNull();
    
    // Выбрать классы с суммарным весом больше delta
    void UpdateClassWeights();


    // Выбрать случайный класс или его отрицание, у которого вес больше delta
    SelectedClassInfo SelectRandomClass(Weight delta);

    // Найти объекты класса и его дополнения
    void PartitionByClass(int label, ObjectPtrList& positive, ObjectPtrList& negative);

    // Выбрать оптимальные наборы положительных и отрицательных прецедентов,
    // которые необходимо научиться различать
    SelectedClassInfo SelectPositiveNegativeSamples(
        Weight epsilon, 
        Weight delta, 
        ObjectPtrList& positive, 
        ObjectPtrList& negative);

    private:

        // Функционал, вычисляющий 
        Weight Delta(Weight posw, Weight negw, Weight epsilon);
};