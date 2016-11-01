#pragma once

#include "Dataset.h"
#include "ECmp.h"
#include "Configurable.h"
#include "Registrator.h"
#include "Interfaces.h"
#include "FeatureSelectorBase.h"


typedef std::vector<ECmp::RelationType> ECmpRelationTypes;

struct ECmpByRankFeatureSelectorOptions
{
    // Используемые отношения
    ECmpRelationTypes relations;

    // Максимальный ранг эл.кл.
    int maxRank;

    ECmpByRankFeatureSelectorOptions()
        :maxRank(1)
    {
        relations.push_back(ECmp::LE);
    }
};

//////////////////////////////////////////////////////////////////////////
// Отбор признаков всех объектов, использующихся при инициализации
//////////////////////////////////////////////////////////////////////////
class ECmpByRankFeatureSelector: 
    public FeatureSelectorBase, public IConfigurable
{

public:

    ECmpByRankFeatureSelectorOptions Options;

    static void Register()
    {
        Registrator<IFeatureSelectorPtr>::Instance().
            RegisterFactory("ECmpByRank", [](){return std::make_shared<ECmpByRankFeatureSelector>();});
    }
        
    virtual void Init( Dataset const& ds);

    virtual void SetOptions( ::Options const& options );
        
    virtual void SetDefaultOptions();
        
    virtual void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting );

    virtual void Select( ObjectsClassRange const& cls );

private:

    void CollectECSet(Object const& o);

    ECmpRelationTypes GetRelationTypes(::Options const& options) const;

};