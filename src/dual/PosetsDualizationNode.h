#pragma once
#include "IPoset.h"
#include "PosetItem.h"
#include "bit_vector.h"

namespace PosetDualization {

/*!
    Decision tree node of dualization algorithm over posets
*/
struct PosetsDualizationNode {
    /// Types of decision tree nodes
    enum NodeType {
        Extra = -1,
        Inner = 0,
        Result = 1
    };

    /// Current item
    PosetItems CurrentPosetItems;

    /// Type of decision tree node
    NodeType Type;

    /// Stop enumeration flag
    //bool Stoped;

    /// Prun branch flag
    //bool Pruned;

    PosetsDualizationNode();

    void Clear();

    void Init(Posets const& posets);

    void StoreUsedItemsSizes();

    void RestoreUsedItemsSizes();

    void Use(int j, PosetItem const& a);

    void Set(int j);

    void Set(int j, PosetItem const& a);

    void Reset(int j);

    bool WasUsed(IPoset const& poset, int j, PosetItem const& a) const;

    bool IsSetted(int j) const;

private:
    /// ������ �������������� ���������
    std::vector<PosetItems> _usedItems;
    /// ���� �������� ������� �������������� ���������
    std::vector<int> _usedItemsSizes;
    /// ����� ��� ������� ������������� ���������
    bit_vector _settedItems;
    /// �������� ��������� �� �� ����������
    PosetItems _replacesItems;
};

}  // namespace PosetDualization