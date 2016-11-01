#pragma once
#include "FeatureValue.h"
#include <memory>
#include "move_default.h"
#include "IteratorRange.h"
#include "containers.h"

typedef std::vector<int> Features;

typedef std::vector<int> Values;

typedef std::vector<int> Positions;

struct EC;

typedef std::vector<EC> ECSet;

typedef float Weight;

typedef std::vector<Weight> Weights;

// Описание объекта
struct Object
{
    int label;
    Values values;
    Weight weight;
    //Weight mark;

    UTILITY_MOVE_DEFAULT_MEMBERS(Object, (label)(values)(weight));

    Object(Values v, int l = -1, float w = 1)
        :values(std::move(v)), label(l), weight(w)
    {
    }

    Object(int l = -1, float w = 1)
        :label(l), weight(w)
    {
    }

    // Проверить, удовлетворяет ли объект эл.кл.
    /*bool Apply(FeatureValues const& ec) const;

    // Проверить, удовлетворяет ли объект эл.кл.
    bool Apply(EC const& ec) const;

    // Применение эл.кл. набора к объекту
    void Apply(ECSet const& ecset, Positions& positions) const;*/

    // Расстояние между объектами, равное число различающихся признаков
    int HammingDistance(Object const& o) const;

    // Число совпадающих признаков
    int Similarity(Object const& o) const;

    bool HasFeature(int j) const
    {
        return inside(values, j);
    }

};

typedef std::vector<Object> Objects;

typedef std::shared_ptr<Object> ObjectPtr;

typedef std::vector<ObjectPtr> ObjectPtrList;

typedef IteratorRange<ObjectPtrList::const_iterator> ObjectsRange;

// Отсортировать объекты по убыванию весов
template <typename Objects>
inline void SortObjectsByWeights( Objects& objects )
{
    sort(objects,
        [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        return o1->weight > o2->weight;
    });
}

// Отобрать объекты с наибольшими весами
template <typename Objects>
inline void MostObjectsByWeights(Objects& objects, int count )
{
    most(objects, count, [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        return o1->weight > o2->weight;
    });
}

// Отобрать необходимое число объектов с наибольшим весом
template <typename Objects>
inline void MostObjectsByWeights(
    Objects const& objects, int count, Objects& mostObjects)
{
    most(objects, count, 
        [](ObjectPtr const& o1, ObjectPtr const& o2)
        {
            return o1->weight > o2->weight;
        }, mostObjects);
}
