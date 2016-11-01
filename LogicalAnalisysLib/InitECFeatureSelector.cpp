#include "stdafx.h"
#include "InitECFeatureSelector.h"
#include "ECVotingLearner.h"
#include "BinaryClassifierVoter.h"
#include "ECVoting.h"
//#include "Exceptions.h"
#include "Registrator.h"


InitECFeatureSelector::InitECFeatureSelector(void)
{
    SetupDefaultLearner();
}

void InitECFeatureSelector::Reset()
{
    if (_learner)
        _learner->Reset();
}

void InitECFeatureSelector::Init( Dataset const& ds )
{    
    _ecset.clear();
    
    auto c = std::dynamic_pointer_cast<IECVoting>(_learner->Train(ds));
    if (c)
    {
        c->GetECSet(_ecset);
        unique(_ecset);
    }

    _ecmpSet.clear();
    _ecmpSet.reserve(_ecset.size() * 2);
    for (auto& ec:_ecset)
    {
        _ecmpSet.emplace_back(ec, ECmp::LE);
        _ecmpSet.emplace_back(std::move(ec), ECmp::GE);
    }
        
    LOG_SEV(trace) << "localBasis.size()=" << _ecmpSet.size();
}


void InitECFeatureSelector::SetDefaultOptions()
{
    if (! _learner)
        SetupDefaultLearner();
    else
        TrySetDefaultOptions(_learner);
}

void InitECFeatureSelector::SetOptions( ::Options const& options )
{
    _learner = TryCreateChild<IClassifierLearnerPtr>(options, "learner");
    
    if (! _learner)
        SetupDefaultLearner();
}



/*IClassifierPtr ECVotingFeatureSelector::TrainByObjects( ObjectPtrList const& objects )
{
    _intermediateDataset.objects = objects;
    _intermediateDataset.CollectClasses();
        
    return TrainByDataset(_intermediateDataset);
}

IClassifierPtr ECVotingFeatureSelector::TrainByObjects( ObjectPtrList const& positive, ObjectPtrList const& negative )
{
    _intermediateDataset.objects = join(positive, negative);
    auto labels = StoreLabels(_intermediateDataset.objects);
    
    for (auto&p : positive)
        p->label = 0;

    for (auto&n : negative)
        n->label = 1;
        
    _intermediateDataset.CollectClasses();
    
    auto c = TrainByDataset(_intermediateDataset);

    RestoreLabels(_intermediateDataset.objects, labels);
    
    return c;
}


void ECVotingFeatureSelector::ExtractECmpSet( BinaryClassifierVoter* voter )
{
    _ecmpset.clear();

    if (! voter)
        return;

    int l = voter->GetClassifiers().size(); // число классов

    for (auto& kv: voter->GetClassifiers())
    {
        for (auto& cls : kv.second)
        {
            auto norm = std::dynamic_pointer_cast<BinaryClassifierNormalizer>(cls);
            if (! norm)
                continue;
            
            auto c = norm->GetInner<ECVoting>();
            if (! c)
                continue;

            for (auto& ec: c->GetECSet())
            {
                auto w = ec.weight * norm->GetScale();

                if (! w)
                    continue;
                
                /*if (l == 2)
                {
                    if (kv.first == 0 && w > 0 || kv.first == 1 && w < 0)
                    {
                        _ecmpset.emplace_back(ec, ECmp::GE, 1.0f);
                    }
                    else
                    {
                        _ecmpset.emplace_back(ec, ECmp::LE, 1.0f);
                    }                 
                }
                else
                {
                    _ecmpset.emplace_back(ec, ECmp::LE, 1.0f);
                    _ecmpset.emplace_back(ec, ECmp::GE, 1.0f);
               }
                
                
            }
            
        }
        unique(_ecmpset);
    }
}*/


void InitECFeatureSelector::Register()
{
    Registrator<IFeatureSelectorPtr>::Instance().
        RegisterFactory("InitECFeatureSelector", [](){return std::make_shared<InitECFeatureSelector>();});
}


void InitECFeatureSelector::SetupDefaultLearner()
{
    auto boosting = std::make_shared<BoostingLearner>();
    auto ec =  std::make_shared<ECVotingLearner>();
    ec->GetOptions().coverLimit = 1;
    ec->GetOptions().coverThreshold = 10;
    boosting->positiveLearner = ec;

    _learner = boosting;
}

void InitECFeatureSelector::Select( ObjectsClassRange const& cls )
{
}    


void InitECFeatureSelector::Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
{
}