#pragma once

#include <boost/random.hpp>

#include "blob.h"

//////////////////////////////////////////////////////////////////////////
// Генераторы случайных чисел
//////////////////////////////////////////////////////////////////////////
typedef boost::mt11213b rand_engine;

typedef boost::uniform_01<> uni_dist;
//typedef boost::variate_generator<rand_engine, uni_dist> uni_generator;

typedef boost::normal_distribution<> norm_dist;
//typedef boost::variate_generator<rand_engine, norm_dist> norm_generator;

//////////////////////////////////////////////////////////////////////////
// Класс для удобства работы со случайными числами
//////////////////////////////////////////////////////////////////////////
template<typename Dist = uni_dist, 
    typename Eng = rand_engine >
struct RandomSampler
{
        
    // Получить экземпляр распределения
    static Dist& GetDistribution()
    {
        static Dist dist;
        return dist;
    }

    static Eng& GetEngine()
    {
        static Eng eng;
        return eng;
    }
               
    
    // Выбрать несколько случайных чисел
    template <typename C, typename T>
    static void RandomValues(int count, C& values, T shrink = 0, T scale = 1)
    {
        if (count > 1)
            values.reserve(values.size() + count);
        for (int i = 0; i < count; ++i)
            values.push_back(RandomValue(shrink, scale));
    }

    // Выбрать несколько случайных чисел
    template <typename I, typename T>
    static void RandomValues(I begin, I end, T shrink = 0, T scale = 1)
    {
        for (auto i = begin; i != end; ++i)
            *i = RandomValue(shrink, scale);
    }


    template <typename T>
    static T RandomValue(T shrink = 0, T scale = 1)
    {
        return GetDistribution()(GetEngine()) * scale + shrink;
    }
};

struct RandomChoicer
{
    typedef RandomSampler<boost::uniform_01<float> > RandomSamplerType;

    // Выбрать несколько случайных объектов из дискретного равномерного распределения
    template <typename C, typename C1>
    static void RandomChoice(
        C const& samples, 
        C1& output, 
        int count = 1)
    {
        int n = samples.size();

        if (n == 0 || count <= 0)
            return;

        if (count > 1)
            output.reserve(count + output.size());
        
        for (int i = 0; i < count; ++i)
            output.push_back(samples[rand() % n]);
    }

    // Посчитать веса объектов
    template <typename GetWeight>
    static auto GetTotalWeight(int n, GetWeight getWeight)
        -> decltype (getWeight(0)) 
    {
        
        if (n <= 0)
            return 0;
        
        auto totalWeight = getWeight(0);
        if (totalWeight < 0)
            totalWeight = 0;

        for (int i = 1; i < n; ++i)
        {
            auto w = getWeight(i);
            if (w <= 0)
                continue;
            totalWeight += w;
        }
        
        return totalWeight;
    }

    // Выбрать нескольку случайных элементов из взвешенной совокупности
    template <typename C, typename GetWeight, typename C1>
    static void RandomChoiceWeighted(
        C const& samples, 
        GetWeight getWeight, 
        C1& output, 
        int count = 1)
    {
        if (count <= 0 || samples.empty())
            return;

        RandomChoiceWeighted(samples, getWeight, output, 
            GetTotalWeight(samples.size(), getWeight), count);
    }
    
    // Выбрать нескольку случайных элементов из взвешенной совокупности
    // Необходимо указать суммарный вес объектов
    template <typename C, typename GetWeight, typename C1, typename T>
    static void RandomChoiceWeighted(
        C const& samples, 
        GetWeight getWeight, 
        C1& output, 
        T totalWeight, 
        int count = 1)
    {
        int n = samples.size();

        if (count <= 0 || totalWeight <= 0 || n == 0)
            return;
        
        int s = output.size();
        
        if (count > 1)
            output.reserve(count + s);
        
        blob<float> random(count);
        RandomSamplerType::RandomValues(random.begin(), random.end(), 0.0f, (float) totalWeight);
        sort(random);
        
        auto current = totalWeight;
                
        for (int i = 0; i < n; ++i)
        {
            auto w = getWeight(i);

            if (w <= 0)
                continue;
            
            current -= w;

            while (current <= random.back())
            {
                output.push_back(samples[i]);
                random.pop_back();
                if (random.empty())
                    break;
            }

            if (random.empty())
                break;
        }

        if (count > 1)
            std::random_shuffle(output.begin() + s, output.end());
    }

};