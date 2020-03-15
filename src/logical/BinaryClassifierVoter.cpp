
#include "BinaryClassifierVoter.h"

BinaryClassifierNormalizerPtr BinaryClassifierVoter::AddClassifier( int label, IBinaryClassifierPtr cls, Weight w )
{
    auto norm = std::make_shared<BinaryClassifierNormalizer>(cls);
    norm->Scale(w);
    _classifiers[label].push_back(norm);

    return norm;
}

Prediction BinaryClassifierVoter::Apply( Object const& o ) const
{
    Prediction p(o.label);
    p.predictions.reserve(_classifiers.size());

    for (auto& cls:_classifiers)
    {
        ClassPrediction cp(cls.first);
        for (auto& c:cls.second)
            cp.estimate += c->Apply(o);

        p.predictions.push_back(cp);
    }

    return std::move(p);
}

void BinaryClassifierVoter::Init( Dataset const& ds, int reserve /*= 0*/ )
{
    for (auto cls:ds.classes)
    {
        _classifiers[cls.label].reserve(reserve);
    }
}

BinaryClassifierNormalizerPtr BinaryClassifierVoter::AddClassifierAndUpdateEstimates( int label, IBinaryClassifierPtr cls, ObjectPtrList const &objects, Predictions& predictions, Weight w /*= 1*/ )
{
    auto norm = AddClassifier(label, cls, w);
        
    int m = objects.size();
    assert(predictions.size() == m);

    if (m)
    {
        int k = predictions.front().PosByLabel(label);
        for (int i = 0; i < m; ++i)
        {
            predictions[i].predictions[k].estimate += norm->Apply(*objects[i]);
        }
    }
    return norm;    
}

void BinaryClassifierVoter::GetECSet( ECSet& ecset )
{
    for (auto& cls:_classifiers)
    {
        for (auto& c: cls.second)
        {
            auto ecv = std::dynamic_pointer_cast<IECVoting>(c);
            if (ecv)
                ecv->GetECSet(ecset);
        }
    }
}