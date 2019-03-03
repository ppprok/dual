
#include "FeatureValue.h"
#include "Object.h"


bool FeatureValue::Apply( Object const& o ) const
{
    if (feature < 0)
        return false;

    if ((int)o.values.size() <= feature)
        return false;

    return o.values[feature] == value;
}