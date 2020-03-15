
#include "ECmpByRankFeatureSelector.h"

#include "Configurable.h"
#include "containers.h"
#include "logging_utils.h"

void ECmpByRankFeatureSelector::SetOptions( ::Options const& options )
{
    ECmpByRankFeatureSelectorOptions def;
    
    Options.maxRank = options.get("maxRank", def.maxRank);
    Options.relations = GetRelationTypes(options);
    if (Options.relations.empty())
        Options.relations.push_back(ECmp::LE);    
}


void ECmpByRankFeatureSelector::Init( Dataset const& ds )
{
    _ecmpSet.clear();

    if (Options.maxRank < 1 || Options.relations.empty())
        return;

    auto s = _ecmpSet.size();
    
    for (auto& o : ds.objects)
    {
        CollectECSet(*o);

        if (s + 100 < _ecmpSet.size())
        {
            unique(_ecmpSet);
            s = _ecmpSet.size();
        }        
    }

    unique(_ecmpSet);
    
    LOG_SEV(trace) << "localBasis.size()=" << _ecmpSet.size();
}



// Получить первый по порядку представительный набор объекта с указанным рангом
void FirstEC(Object const& o, int r, FeatureValues& fv)
{
    int n = o.values.size();
    fv.clear();
    for (int j = 0; j < r && j < n; ++j)
        fv.emplace_back(j, o.values[j]);
}

// Получить следующий по порядку представительный набор объекта
void NextEC(Object const& o, FeatureValues& fv)
{
    int r = fv.size();
    if (! r)
        return;

    int n = o.values.size();
    assert(n >= r);
    ++fv.back().feature;

    int k = r-2;
    for (; k >= 0 && fv[k+1].feature >= n; --k)
    {
        fv[k].feature++;
    }

    if (k <= 0)
        k=1;
    
    for (; k < r; ++k)
    {
        if (fv[k].feature >= n)
            fv[k].feature = fv[k-1].feature + 1;
    }

    if (fv.back().feature >= n)
    {
        fv.clear();
        return;
    }

    for (auto& ff:fv)
        ff.value = o.values[ff.feature];
}

void ECmpByRankFeatureSelector::CollectECSet(Object const& o)
{
    
    int n = o.values.size();
    FeatureValues fv;
    fv.reserve(Options.maxRank);
    
    for (auto rel : Options.relations)
        for (int r = 1; r <= Options.maxRank && r <= n; ++r)
            for (FirstEC(o, r, fv); ! fv.empty(); NextEC(o, fv))
                _ecmpSet.emplace_back(fv, rel);
}


typedef std::vector<std::string> Strings;


ECmpRelationTypes ECmpByRankFeatureSelector::GetRelationTypes( ::Options const& options ) const
{
    ECmpRelationTypes relations;

    for (auto& c :  options.get_child("relation", ::Options()))
    {
        auto s =  c.first;
        if (s == "LE")
            relations.push_back(ECmp::LE);
        else if (s == "GE")
            relations.push_back(ECmp::GE);
        else if (s == "NOTAND")
            relations.push_back(ECmp::NotAnd);
        else if (s == "OR")
            relations.push_back(ECmp::Or);
        else
            LOG_SEV(warning) << "Unknown relation type " << s;
    }

    return std::move(relations);
}

void ECmpByRankFeatureSelector::SetDefaultOptions()
{
    Options = ECmpByRankFeatureSelectorOptions();
}

void ECmpByRankFeatureSelector::Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
{
}

void ECmpByRankFeatureSelector::Select( ObjectsClassRange const& cls )
{
}