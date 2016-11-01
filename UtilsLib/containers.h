#pragma once
#include "bit_chunk.h"

// Склеить два контейнера
template <typename C>
inline C join(C const& c1, C const& c2)
{
    C c3;
    c3.reserve(c1.size() + c2.size());
    c3.insert(c3.end(), c1.begin(), c1.end());
    c3.insert(c3.end(), c2.begin(), c2.end());
    return std::move(c3);
}

// Склеить три контейнера
template <typename C>
inline C join(C const& c1, C const& c2, C const& c0)
{
    C c3;
    c3.reserve(c1.size() + c2.size() + c0.size());
    c3.insert(c3.end(), c1.begin(), c1.end());
    c3.insert(c3.end(), c2.begin(), c2.end());
    c3.insert(c3.end(), c0.begin(), c0.end());
    return std::move(c3);
}

/*template <typename C1, typename C2>
C1& append_with_move(C1& c1, C2& c2)
{
    c1.reserve(c1.size() + c2.size());
    for (auto& v:c2)
    {
        c1.push_back(std::move(v));
    }
    c2.clear();

    return c1;
};*/

// Проверить, что индекс указывает на элемент контейнера
template <typename C>
inline bool inside(C const& c, int pos)
{
    return pos >=0 && pos < (int) c.size();
}


// Убрать из коллекции элементы, удовлетворяющие условию
template <typename C, typename F>
inline void pop_if(C& c, F cond)
{
    int n = c.size();
    for (int i = 0; i < n;)
    {
        if (! cond(c[i]))
        {
            ++i;
        }
        else
        {
            c[i] = std::move(c.back());
            c.pop_back();
            --n;
        }        
    }
}

// Убрать из коллекции элементы, удовлетворяющие условию
template <typename C, typename F>
inline void remove_if(C& c, F cond)
{
    auto end = std::end(c);
    c.erase(std::remove_if(std::begin(c), end, cond), end);
}

template <typename C, typename F>
inline int find_if(C& c, F cond)
{
    auto end = std::end(c), beg = std::begin(c);
    auto pos = std::find_if(beg, end, cond);
    if (pos == end)
        return -1;
    return pos - beg;
}

// Убрать из коллекции элементы, возвращающие false
template <typename C>
inline void pop_false(C& c)
{
    int n = (int)c.size();
    for (int i = 0; i < n; )
    {
        if (c[i])
        {
            ++i
        }
        else
        {
            c[i] = std::move(c.back());
            c.pop_back();
            --n;
        }
    }
}

// Проверить, содержит ли коллекция элемент
template <typename C, typename V>
inline bool contains(C const& c, V const& v)
{
    auto e = c.end();
    return std::find(c.begin(), e, v) != e;
}

/*
template <typename C, typename C1>
inline void exclude(C& c, C1 const& excl)
{
    remove(c, [&excl](typename C::value_type const& v)
                    {return contains(excl, v);});
}*/


template <typename C>
inline void sort(C& c)
{
    std::sort(c.begin(), c.end());
}

template <typename C, typename Cmp>
inline void sort(C& c, Cmp cmp)
{
    std::sort(c.begin(), c.end(), cmp);
}


template <typename C>
inline void unique(C& c, bool ordered = false)
{
    if (! ordered)
        sort(c);
    
   c.resize(std::unique(c.begin(), c.end()) - c.begin());
}



// Перемешать элементы коллекции
template <typename C>
inline void random_shuffle(C& c)
{
    std::random_shuffle(std::begin(c), std::end(c));
}

template <typename C>
inline void random_sort(C& c)
{
    random_shuffle(c);
    sort(c);
}


template <typename C, typename Cmp>
inline void random_sort(C& c, Cmp cmp)
{
    random_shuffle(c);
    sort(c, cmp);
}


// Сравнение элементов коллекций
template <typename C1, typename C2>
inline bool equal(C1 const& c1, C2 const& c2)
{
    return c1.size() == c2.size() && std::equal(std::begin(c1), std::end(c1), std::begin(c2));
}

// Сравнение в лексикографическом порядке
template <typename C1, typename C2>
inline bool less(C1 const& c1, C2 const& c2)
{
    return  std::lexicographical_compare(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2));
}

// Разместить элементы двух коллекций в первой коллекции
template <typename C>
inline void merge(C& c1, C& c2)
{
    if (c1.size() < c2.size())
        c1.swap(c2);
    while (! c2.empty())
    {
        c1.emplace_back(std::move(c2.back()));
        c2.pop_back();
    }
}


// Отобрать необходимое число <<лучших>> элементов
template <typename Objects, typename Cmp>
inline void most(
    Objects const& objects, int count, Cmp cmp, Objects& most, bool order = true, bool random = true)
{
    most.clear();

    if (count <= 0)
        return;

    if (count >= (int)objects.size())
    {
        most.assign(objects.begin(), objects.end());
        if (order)
        {
            if (random)
                random_shuffle(most);
            sort(most, cmp);
        }
    }
    else
    {
        most.resize(count);    
        //if (random)
          //  random_shuffle(objects);
        std::partial_sort_copy(objects.begin(), objects.end(), most.begin(), most.end(), cmp);
    }
}

// Оставить необходимое число <<лучших>> элементов
template <typename Objects, typename Cmp>
inline void most(
    Objects& objects, int count, Cmp cmp, bool order = true, bool random = true)
{
    if (count <= 0)
    {
        objects.clear();
        return;
    }
       
    if (count >= (int)objects.size())
    {
        if (order)
        {
            if (random)
                random_shuffle(objects);

            sort(objects, cmp);
        }
    }
    else
    {
        if (random)
            random_shuffle(objects);

        std::partial_sort(objects.begin(), objects.begin() + count, objects.end(), cmp);
        objects.resize(count);
    }

}



