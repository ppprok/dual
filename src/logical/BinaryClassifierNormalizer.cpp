
#include "BinaryClassifierNormalizer.h"




Estimate BinaryClassifierNormalizer::Apply( Object const& o ) const
{
    if (! _classifier)
        return 0;        
    auto e = _classifier->Apply(o);
    if (_shrink)
        e += _shrink;
    if (_scale != 1 && e)
        e *= _scale;
    return e;
}

void BinaryClassifierNormalizer::Normalize( ObjectPtrList const& objects, Estimate from /*= 0*/, Estimate to /*= 1*/ )
{
    Estimate min = std::numeric_limits<Estimate>::max();
    Estimate max = -min;

    for (auto& o : objects)
    {
        auto e = _classifier->Apply(*o);
        if (min > e)
            min = e;
        if (max < e)
            max = e;
    }

    if (max <= min || from >= to)
    {
        _shrink = 0;
        _scale = 1;
    }
    else
    {
        _scale = (to - from) / (max - min);
        _shrink = from / _scale - min;             
    }
}

BinaryClassifierNormalizer::BinaryClassifierNormalizer( IBinaryClassifierPtr cls)
{
    auto norm = std::dynamic_pointer_cast<BinaryClassifierNormalizer>(cls);
    if (norm)
    {
        _classifier = norm->_classifier;
        _shrink = norm->_shrink;
        _scale = norm->_scale;
    }
    else
    {
        _classifier = cls;
        _shrink = 0;
        _scale = 1;
    }       
}

void BinaryClassifierNormalizer::GetECSet( ECSet& ecset )
{
    auto ecv = std::dynamic_pointer_cast<IECVoting>(_classifier);
    if (ecv)
        ecv->GetECSet(ecset);
}