
#include "FTVoting.h"


Estimate FTVoting::Apply( 
    Object const& object, 
    Object const& precedent, 
    Features const& featureTest ) const
{
    int n1 = object.values.size();
    int n2 = object.values.size();

    for (int j : featureTest)
    {
        if (j < 0 || j >= n1 || j >= n2 || precedent.values[j] != object.values[j])    
            return 0;
    }

    return 1;
}

Estimate FTVoting::Apply( Object const& object ) const
{
    Estimate e = 0.0f, tn = 0;

    for (auto& p : precedents)
        for (auto& t: tests)
        {
            e += Apply(object, *p, t);
            ++tn;
        }

    return e / tn;
}

FTVoting::FTVoting( FeatureTests tests_, ObjectPtrList precedents_ )
{
    precedents = std::move(precedents_);
    tests = std::move(tests_);
}

void FTVoting::GetECSet( ECSet& ecset )
{
    ecset.reserve(ecset.size() + precedents.size() * tests.size());
    for (auto& o:precedents)
        for(auto& ft:tests)
            ecset.emplace_back(ft, o->values, 1.0f);
}