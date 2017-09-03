#include "Interfaces.h"

//  лассификатор, огрубл€ющий оценки исходного классификатора до 0 или 1
class BinaryClassifierBoolAdaptor
    :public IBinaryClassifier
{
    IBinaryClassifierPtr _classifier;

    Estimate _threshold;

public:

    BinaryClassifierBoolAdaptor(IBinaryClassifierPtr classifier, Estimate th = 0)
    {
        _classifier = classifier;
        _threshold = th;
    }

    virtual Estimate Apply( Object const& o ) const
    {
        if (! _classifier)
            return 0;        
        auto e = _classifier->Apply(o);
        if (e > _threshold)
            return 1;        
        return 0;
    }
};

