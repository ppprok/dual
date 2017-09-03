#pragma once

#include <cmath>

typedef float Weight;

// Условия, которым должна удовлетворять информативность
struct InformativityCondition
{
    enum InformativityType
    {
        AdaBoost,
        EmpericalRisk,
        P,
        N
    };

    InformativityCondition()
        :type(AdaBoost), threshold(0), minP(0), minN(0)
    {
    }
    
    InformativityType type;
    Weight threshold;
    Weight minP;
    Weight minN;

    Weight Get(Weight p, Weight n) const
    {
        if (n < minN / 2)
            n = minN / 2;
        if (p < n)
            return 0;

        switch (type)
        {
        case InformativityCondition::AdaBoost:
            return sqrt(p) - sqrt(n);
        case InformativityCondition::EmpericalRisk:
            return p - n;
        case InformativityCondition::P:
            return p;
        case InformativityCondition::N:
            return -n;
        }
        
        return 0;
    }

    bool Sucess(Weight p, Weight n) const
    {
        return Get(p, n) > threshold;
    }

};