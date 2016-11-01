#pragma once
#include <vector>

struct Object;

// Целочисленный признак и его значение
struct FeatureValue
{
    int feature;
    int value;

    FeatureValue(int f = -1, int v = -1)
    {
        feature = f;
        value = v;
    }

    bool operator <(FeatureValue const& fv) const
    {
        if (feature < fv.feature)
            return true;

        if (feature > fv.feature)
            return false;

        return value < fv.value;
    }

    bool operator ==(FeatureValue const& fv) const
    {
        return feature == fv.feature && value == fv.value;        
    }

    bool Apply(Object const& o) const;
};

typedef std::vector<FeatureValue> FeatureValues;
