#include "PosetsDualizationNode.h"


namespace PosetDualization
{
    
    bool PosetsDualizationNode::IsSetted(int j) const
    {
        return _settedItems.get(j);
    }

    bool PosetsDualizationNode::WasUsed(IPoset const& poset, int j, PosetItem const& a) const
    {
        return !_usedItems[j].empty() && find_if(_usedItems[j], 
            [&poset, j, &a](PosetItem const&b)
        {
            return poset.Equal(a, b);
        }) != -1;
    }

    void PosetsDualizationNode::Reset(int j)
    {
        CurrentPosetItems[j] = _replacesItems[j];
        _settedItems.reset(j);
    }

    void PosetsDualizationNode::Set(int j, PosetItem const& a)
    {
        CurrentPosetItems[j] = a;
    }

    void PosetsDualizationNode::Set(int j)
    {
        assert(inside(CurrentPosetItems, j));
        _settedItems.set(j);
        _replacesItems[j] = CurrentPosetItems[j];
    }

    void PosetsDualizationNode::Use(int j, PosetItem const& a)
    {
        _usedItems[j].push_back(a);
    }

    void PosetsDualizationNode::RestoreUsedItemsSizes()
    {
        for (auto& u : _usedItems)
        {
            assert(! _usedItemsSizes.empty());
            u.resize(_usedItemsSizes.back());
            _usedItemsSizes.pop_back();
        }
    }

    void PosetsDualizationNode::StoreUsedItemsSizes()
    {
        for (auto rbeg = _usedItems.rbegin(), rend = _usedItems.rend(); rbeg != rend; ++rbeg)
            _usedItemsSizes.push_back(rbeg->size());
    }

    void PosetsDualizationNode::Init(Posets const& posets)
    {
        Clear();

        auto n = posets.size();
        CurrentPosetItems.reserve(n);
        _replacesItems.reserve(n);

        for (auto& p : posets)
        {
            CurrentPosetItems.push_back(p->GetUniversalMaximal());
            _replacesItems.push_back(p->GetUniversalMaximal());
        }

        _usedItems.resize(n);

        for (int j = 0; j != n; ++j)
            _usedItems[j].assign(1, posets[j]->GetUniversalMaximal());

        _settedItems.zero(n);
    }

    void PosetsDualizationNode::Clear()
    {
        Type = Inner;
        //Stoped = false;
        //Pruned = false;
        CurrentPosetItems.clear();
        _replacesItems.clear();
        _usedItemsSizes.clear();
        for (auto& u : _usedItems)
            u.clear();
        _settedItems.zero(0);

        //CurrentFilterIndep.clear();
    }

    PosetsDualizationNode::PosetsDualizationNode()
    {
        Clear();
    }


}