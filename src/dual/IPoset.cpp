#include "IPoset.h"




std::string ToString( IPosetPtr poset, PosetItems const& items, std::string const& separator /*= " "*/ )
{
    std::string str;
    for (auto i = items.begin(), end = items.end(); i != end; ++i )
    {
        str += poset->ToString(*i);
        if (i+1 != end)
            str += separator;
    }
    return str;
}

bool AllPreceq( IPoset const& poset, PosetItems const& left, PosetItem const& right )
{
    for (auto& l :left)
        if (! poset.Preceq(l ,right))
            return false;
    return true;
}

bool Contains( IPoset const& poset, PosetItems const& collection, PosetItem const& item )
{
    return std::any_of(collection.begin(), collection.end(),
        [&poset, item](PosetItem const& a)
    {
        return poset.Equal(a, item);
    });
}