
#include "BinaryClassifierAdaptor.h"




Estimate BinaryClassifierAdaptor::Apply( Object const& o ) const
{
    auto p = _classifier->Apply(o);
    assert(p.predictions.size() == 2 && p.predictions[0].label == 0 &&
        p.predictions[1].label == 1);

    return p.predictions[0].estimate - p.predictions[1].estimate;
}

BinaryClassifierAdaptor::BinaryClassifierAdaptor( IClassifierPtr classifier )
{
    assert(classifier);
    _classifier = classifier;
}

void BinaryClassifierAdaptor::GetECSet( ECSet& ecset )
{
    auto ec = std::dynamic_pointer_cast<IECVoting>(_classifier);
    if (ec)
        ec->GetECSet(ecset);
}