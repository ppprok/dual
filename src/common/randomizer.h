/// @file
/// @brief Random number and random choice helpers.

#pragma once

#include <random>

#include <dual/blob.h>

/// Random number generator (non-deterministic `std::random_device`).
typedef std::random_device rand_engine;

/// Uniform distribution on `[0, 1)`.
typedef std::uniform_real_distribution<> uni_dist;

/// Standard normal distribution.
typedef std::normal_distribution<> norm_dist;

/// @brief Convenience class for working with random numbers.
///
/// Values are drawn from a process-wide static distribution and engine, then mapped linearly:
/// `value * scale + shrink`.
/// @tparam Dist Distribution type.
/// @tparam Eng Random engine type.
template<typename Dist = uni_dist, typename Eng = rand_engine>
struct RandomSampler {

    /// @brief Get the shared distribution instance.
    static Dist& GetDistribution() {
        static Dist dist;
        return dist;
    }

    /// @brief Get the shared engine instance.
    static Eng& GetEngine() {
        static Eng eng;
        return eng;
    }

    /// @brief Append @p count random numbers `value * scale + shrink` to @p values.
    template<typename C, typename T>
    static void RandomValues(int count, C& values, T shrink = 0, T scale = 1) {
        if (count > 1)
            values.reserve(values.size() + count);
        for (int i = 0; i < count; ++i)
            values.push_back(RandomValue(shrink, scale));
    }

    /// @brief Fill `[begin, end)` with random numbers `value * scale + shrink`.
    template<typename I, typename T>
    static void RandomValues(I begin, I end, T shrink = 0, T scale = 1) {
        for (auto i = begin; i != end; ++i)
            *i = RandomValue(shrink, scale);
    }

    /// @brief One random number `value * scale + shrink`.
    template<typename T>
    static T RandomValue(T shrink = 0, T scale = 1) {
        return GetDistribution()(GetEngine()) * scale + shrink;
    }
};

/// @brief Random selection of objects from a population.
struct RandomChoicer {
    /// Sampler of uniform `float` values used for weighted choice.
    typedef RandomSampler<std::uniform_real_distribution<float>> RandomSamplerType;

    /// @brief Pick several random objects from a discrete uniform distribution (with repetition,
    ///        using `rand()`), appending them to @p output.
    template<typename C, typename C1>
    static void RandomChoice(C const& samples, C1& output, int count = 1) {
        int n = samples.size();

        if (n == 0 || count <= 0)
            return;

        if (count > 1)
            output.reserve(count + output.size());

        for (int i = 0; i < count; ++i)
            output.push_back(samples[rand() % n]);
    }

    /// @brief Compute the total weight of objects `0..n-1`; non-positive weights count as 0.
    template<typename GetWeight>
    static auto GetTotalWeight(int n, GetWeight getWeight) -> decltype(getWeight(0)) {

        if (n <= 0)
            return 0;

        auto totalWeight = getWeight(0);
        if (totalWeight < 0)
            totalWeight = 0;

        for (int i = 1; i < n; ++i) {
            auto w = getWeight(i);
            if (w <= 0)
                continue;
            totalWeight += w;
        }

        return totalWeight;
    }

    /// @brief Pick @p count random elements, with repetition, from a weighted population and append
    ///        them to @p output.
    /// @param samples Population.
    /// @param getWeight `getWeight(i)` is the weight of `samples[i]`; non-positive weights are never
    ///        chosen.
    /// @param[out] output Container the chosen elements are appended to.
    /// @param count Number of elements to pick.
    template<typename C, typename GetWeight, typename C1>
    static void RandomChoiceWeighted(C const& samples, GetWeight getWeight, C1& output, int count = 1) {
        if (count <= 0 || samples.empty())
            return;

        RandomChoiceWeighted(samples, getWeight, output, GetTotalWeight(samples.size(), getWeight), count);
    }

    /// @brief Pick several random elements from a weighted population, with a known total weight
    ///        (see the overload without @p totalWeight).
    /// @param samples Population.
    /// @param getWeight `getWeight(i)` is the weight of `samples[i]`.
    /// @param[out] output Container the chosen elements are appended to.
    /// @param totalWeight Total weight of the objects, as returned by `GetTotalWeight`.
    /// @param count Number of elements to pick.
    template<typename C, typename GetWeight, typename C1, typename T>
    static void RandomChoiceWeighted(C const& samples, GetWeight getWeight, C1& output, T totalWeight, int count = 1) {
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

        for (int i = 0; i < n; ++i) {
            auto w = getWeight(i);

            if (w <= 0)
                continue;

            current -= w;

            while (current <= random.back()) {
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
