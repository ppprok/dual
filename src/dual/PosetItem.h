#pragma once
#include <vector>
#include <assert.h>

#include "bits.h"


struct PosetItem
{
    typedef short IntervalBound;
    struct Interval
    {
        IntervalBound min;
        IntervalBound max;
    };

    typedef int ChainNumber;
    typedef int Vertex;
    typedef bits32 Subset;

    union
    {
        Interval interval;        
        ChainNumber dnumber;
        Vertex vertex;        
        Subset subset;    
    } value;

    PosetItem(){};

    PosetItem(Subset const& bits)
    {
        value.subset = bits;
    }

    PosetItem(Vertex i)
    {
        value.vertex = i;
    }

    PosetItem(IntervalBound a, IntervalBound b)
    {
        value.interval.min = a;
        value.interval.max = b;
    }

    bool operator ==(PosetItem const& i) const
    {
        return value.dnumber == i.value.dnumber;
    }

    bool operator <(PosetItem const& i) const
    {
        return value.dnumber < i.value.dnumber;
    }

    bool operator <=(PosetItem const& i) const
    {
        return value.dnumber <= i.value.dnumber;
    }
};

// type of items collection
typedef std::vector<PosetItem> PosetItems;

struct PosetInterval
{
    PosetItem lower;
    PosetItem upper;

    PosetInterval()
    {
    }

    PosetInterval(PosetItem const& l, PosetItem const& u)
        :upper(u), lower(l)
    {
    }

    bool operator==(PosetInterval const& that) const
    {
        return upper == that.upper && lower == that.lower;
    }
};

typedef std::vector<PosetInterval> PosetIntervals;


