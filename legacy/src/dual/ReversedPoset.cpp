
#include "ReversedPoset.h"
#include "NumberChain.h"
#include "Registrator.h"

ReversedPoset::ReversedPoset(IPosetPtr const& poset) : _inner(poset) {
    assert(_inner);
}

ReversedPoset::ReversedPoset() : _inner(std::make_shared<NumberChain>()) {}
bool ReversedPoset::Preceq(PosetItem const& left, PosetItem const& right) const {
    return _inner->Preceq(right, left);
}

bool ReversedPoset::Prec(PosetItem const& left, PosetItem const& right) const {
    return _inner->Prec(right, left);
}

bool ReversedPoset::Equal(PosetItem const& left, PosetItem const& right) const {
    return _inner->Equal(right, left);
}

bool ReversedPoset::IsMaximal(PosetItem const& a, PosetItems const& A) const {
    return _inner->IsMinimal(a, A);
}

bool ReversedPoset::IsMaximal(PosetItem const& a) const {
    return _inner->IsMinimal(a);
}

bool ReversedPoset::IsMinimal(PosetItem const& a, PosetItems const& A) const {
    return _inner->IsMaximal(a, A);
}

bool ReversedPoset::IsMinimal(PosetItem const& a) const {
    return _inner->IsMaximal(a);
}

bool ReversedPoset::IsItem(PosetItem const& a) const {
    return _inner->IsItem(a);
}

bool ReversedPoset::IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const {
    return _inner->IsMinimalIndependent(a, A);
}

bool ReversedPoset::IsMinimalIndependent(PosetItem const& a, PosetItems const& B) const {
    return _inner->IsMaximalIndependent(a, B);
}

bool ReversedPoset::IsIndependent(PosetItem const& a, PosetItems const& A, PosetItems const& B) const {
    return _inner->IsIndependent(a, B, A);
}

bool ReversedPoset::InIdeal(PosetItem const& a, PosetItems const& A) const {
    return _inner->InFilter(a, A);
}

bool ReversedPoset::InFilter(PosetItem const& a, PosetItems const& A) const {
    return _inner->InIdeal(a, A);
}

void ReversedPoset::SelectMaximal(PosetItems& A) const {
    return _inner->SelectMinimal(A);
}

void ReversedPoset::SelectMinimal(PosetItems& A) const {
    return _inner->SelectMaximal(A);
}

void ReversedPoset::SelectIdealIndependent(PosetItems const& A, PosetItems& B) const {
    return _inner->SelectFilterIndependent(A, B);
}

void ReversedPoset::SelectFilterIndependent(PosetItems const& A, PosetItems& B) const {
    return _inner->SelectIdealIndependent(A, B);
}

void ReversedPoset::SelectIndependent(PosetItems const& A, PosetItems const& B, PosetItems& X) const {
    return _inner->SelectIndependent(B, A, X);
}

void ReversedPoset::SelectIdeal(PosetItems const& A, PosetItems& B) const {
    return _inner->SelectFilter(A, B);
}

void ReversedPoset::SelectFilter(PosetItems const& A, PosetItems& B) const {
    return _inner->SelectIdeal(A, B);
}

void ReversedPoset::SelectUnique(PosetItems& items) const {
    return _inner->SelectUnique(items);
}

void ReversedPoset::GetAllItems(PosetItems& items) const {
    return _inner->GetAllItems(items);
}

void ReversedPoset::GetMaximal(PosetItems& maximal) const {
    return _inner->GetMinimal(maximal);
}

void ReversedPoset::GetMinimal(PosetItems& minimal) const {
    return _inner->GetMaximal(minimal);
}

PosetItem ReversedPoset::GetUniversalMaximal() const {
    return _inner->GetUniversalMinimal();
}

PosetItem ReversedPoset::GetUniversalMinimal() const {
    return _inner->GetUniversalMaximal();
}

void ReversedPoset::GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const {
    return _inner->GetMinimalIndependent(A, maximal);
}

void ReversedPoset::GetMinimalIndependent(PosetItems const& A, PosetItems& minimal) const {
    return _inner->GetMaximalIndependent(A, minimal);
}

void ReversedPoset::GetIndependentIntervals(PosetItems const& A,
                                            PosetItems const& B,
                                            PosetIntervals& independent) const {
    _inner->GetIndependentIntervals(B, A, independent);
    for (auto& i : independent) {
        std::swap(i.lower, i.upper);
    }
}

std::string ReversedPoset::ToString(PosetItem const& item) const {
    return _inner->ToString(item);
}

PosetItem ReversedPoset::FromString(char const* begin, char const* end) const {
    return _inner->FromString(begin, end);
}

void ReversedPoset::TopologicalSort(PosetItems& items) const {
    _inner->TopologicalSort(items);
    std::reverse(items.begin(), items.end());
}

bool ReversedPoset::IsTopologicalOrdered(PosetItems const& items) const {
    PosetItems reversed(items.size());
    std::reverse_copy(items.begin(), items.end(), reversed.begin());
    return _inner->IsTopologicalOrdered(reversed);
}

bool ReversedPoset::PrecImmediate(PosetItem const& a, PosetItem const& b) const {
    return _inner->PrecImmediate(b, a);
}

void ReversedPoset::GetImmediatePrec(PosetItem const& item, PosetItems& items) const {
    return _inner->GetImmediateSucc(item, items);
}

void ReversedPoset::GetImmediateSucc(PosetItem const& item, PosetItems& items) const {
    return _inner->GetImmediatePrec(item, items);
}

bool ReversedPoset::IsUniversalMaximal(PosetItem const& item) const {
    return _inner->IsUniversalMinimal(item);
}

bool ReversedPoset::IsUniversalMinimal(PosetItem const& item) const {
    return _inner->IsUniversalMaximal(item);
}

void ReversedPoset::SetOptions(Options const& options) {
    _inner = Registrator<IPosetPtr>::Instance().Create(options.get("type", "NumberChain"));
    assert(_inner);
    TrySetOptions(_inner, options);
}

void ReversedPoset::SetDefaultOptions() {
    _inner = std::make_shared<NumberChain>();
}

void ReversedPoset::Register() {
    Registrator<IPosetPtr>::Instance().RegisterFactory("ReversePoset",
                                                       []() { return std::make_shared<ReversedPoset>(); });
}

void ReversedPoset::GetMinUpperBounds(PosetItems const& items, PosetItems& upperBounds) const {
    _inner->GetMaxLowerBounds(items, upperBounds);
}

void ReversedPoset::GetMaxLowerBounds(PosetItems const& items, PosetItems& lowerBounds) const {
    _inner->GetMinUpperBounds(items, lowerBounds);
}

IPosetPtr Reverse(IPosetPtr const& poset) {
    auto reversed = std::dynamic_pointer_cast<ReversedPoset>(poset);
    if (reversed)
        return reversed->GetInnerPoset();
    return std::make_shared<ReversedPoset>(poset);
}
