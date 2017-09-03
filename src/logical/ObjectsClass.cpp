#include "stdafx.h"

#include "ObjectsClass.h"
#include "InformativityCondition.h"
#include "logging_utils.h"
#include "math_utils.h"

#include "Interfaces.h"




void ObjectsClass::MakeComplement()
{
    positive.swap(negative);
}

ObjectsClass::ObjectsClass( int l /*= -1*/ ) :label(l)
{

}

ObjectsClass::ObjectsClass( ObjectsClassRange const& range, int l /*= -1*/ )
    :label(l), 
    positive(range.positive.begin(), range.positive.end()),
    negative(range.negative.begin(), range.negative.end())
{
}


void ObjectsClass::SplitOptimal( 
    InformativityCondition const& informativity, 
    ObjectsClassRange& correct, 
    ObjectsClassRange& fitting,
    int posLimit)
{
    auto r = DoSplitOptimal(informativity, posLimit);
    
    correct.positive = ObjectsRange(positive, 0, r.first);
    correct.negative = ObjectsRange(negative, 0, r.second);
    
    fitting.positive = ObjectsRange(positive, r.first, positive.size());
    fitting.negative = ObjectsRange(negative, r.second, negative.size());
}


void ObjectsClass::SortObjectsByWeights()
{
    ::SortObjectsByWeights(positive);
    ::SortObjectsByWeights(negative);
}

void ObjectsClass::Clear()
{
    positive.clear();
    negative.clear();
}

std::pair<int, int> ObjectsClass::DoSplitOptimal( InformativityCondition const& informativity, int posLimit )
{
    if (positive.empty() || negative.empty())
        return std::make_pair(0,0);

    //SortObjectsByWeights();

    Weight p = 0;
    Weight n = 0;

    int posSize = positive.size();
    int negSize = negative.size();

    if (posLimit <=0  || posLimit > posSize)
        posLimit = posSize;

    int posCount = 0;
    int negCount = negSize;

    int bestPosCount = posSize;
    int bestNegCount = negSize;

    for(int changes = 1; changes; )
    {
        changes = 0;
        for(;posCount < posSize;)
        {
            /*if (posCount >= posLimit)
                break;*/

            p += positive[posCount++]->weight;
            ++changes;
            if (informativity.Sucess(p, n))
                break;

        }

        for (;negCount > 1; --negCount)
        {
            Weight dw = negative[negCount-1]->weight;
            if (! informativity.Sucess(p, n + dw))
                break;
            n += dw;
            ++changes;
        }


        if (changes && 
            posCount + negCount < 
            bestPosCount + bestNegCount)
        {
            bestPosCount = posCount;
            bestNegCount = negCount;
        }

        if (posCount >= posLimit)
            break;
    }

    /*LOG_SEV(trace) << 
        "posSize=" << posSize << 
        "; bestPosCount=" << bestPosCount << 
        "; negSize=" << negSize << 
        "; bestNegCount=" << bestNegCount;*/

    return std::make_pair(bestPosCount, bestNegCount);
}

void ObjectsClass::Randomize()
{
    random_shuffle(positive);
    random_shuffle(negative);
}