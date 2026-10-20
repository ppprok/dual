#pragma once
#include "Configurable.h"
#include "IPoset.h"
#include "PosetItem.h"
#include "PosetItemsMatrix.h"
#include "string_utils.h"

/*!
    Произведение частичных порядков
*/
class PosetsProduct : public IConfigurable {

    Strings _bindings;  ///< Column names for poset binding

    Posets _posets;  ///< Used posets

    Posets _reversedPosets;  ///< ReversedItems

    mutable std::vector<PosetItems> _supportMap;  ///< Temp arrays for support checking

    //std::stringstream _toStringStream;

public:
    /*!
		Номера элементов в наборе
	*/
    typedef std::vector<int> Indecies;

    PosetsProduct() {}

    //    UTILITY_MOVE_DEFAULT_MEMBERS(
    //        PosetsProduct, (_bindings)(_posets)(_reversedPosets));

    Strings const& GetBindings() const {
        return _bindings;
    }

    /// Set posets and bindings to data columns
    void SetPosets(Posets posets, Strings bindings = Strings());

    /// Get posets respect to columns
    Posets const& GetPosets() const;

    /// Get reversed posets
    Posets const& GetReversedPosets() const;

    IPoset const& GetPoset(int i) const {
        return *_posets[i];
    }

    IPoset& GetPoset(int i) {
        return *_posets[i];
    }

    int GetPosetIndex(std::string const& name) const;

    IPoset const& GetPoset(std::string const& name) const {
        return GetPoset(GetPosetIndex(name));
    }

    IPoset& GetPoset(std::string const& name) {
        return GetPoset(GetPosetIndex(name));
    }

    // Configure task
    virtual void SetOptions(Options const& options);

    // Reset all posets
    virtual void SetDefaultOptions();

    // Check relation <= over product of posets from task
    template<typename I1, typename I2>
    bool Preceq(I1 const& left, I2 const& right) const {
        int n = _posets.size();
        if (left.size() != n || right.size() != n)
            return false;

        auto i1 = left.begin();
        auto i2 = right.begin();

        for (auto p = _posets.begin(), e = _posets.end(); p != e; ++i1, ++i2, ++p)
            if (!(**p).Preceq(*i1, *i2))
                return false;

        return true;
    }

    // Check relation <= over product of posets from task
    //bool Preceq(PosetItemsMatrix::CRowRef const& items, int rowId, PosetItems const& right) const;

    // Check relation <= over product of posets from task
    //bool Preceq(PosetItems const& left, PosetItemsMatrix const& items, int rowId) const;

    // Check independency for ideal
    bool IsIdealIndependent(PosetItemsMatrix const& idealBase, PosetItems const& x) const;

    // Check independency for filter
    bool IsFilterIndependent(PosetItemsMatrix const& filterBase, PosetItems const& x) const;

    // Check maximal independency for ideal
    bool IsMaximalIndependent(PosetItemsMatrix const& idealBase, PosetItems const& x) const;

    // Check minimal independency for filter
    bool IsMinimalIndependent(PosetItemsMatrix const& filterBase, PosetItems const& x) const;

    // Check independency of interval
    bool IsIndependent(PosetItemsMatrix const& idealBase,
                       PosetItemsMatrix const& filterBase,
                       PosetItems const& lower,
                       PosetItems const& upper) const;

    // Check independency of interval and it maximality
    bool IsMaxMinIndependent(PosetItemsMatrix const& idealBase,
                             PosetItemsMatrix const& filterBase,
                             PosetItems const& lower,
                             PosetItems const& upper) const;

    bool IsMinimal(PosetItems const& items) const {
        for (int i = 0, n = _posets.size(); i != n; ++i)
            if (!_posets[i]->IsMinimal(items[i]))
                return false;
        return true;
    }

    bool IsMaximal(PosetItems const& items) const {
        for (int i = 0, n = _posets.size(); i != n; ++i)
            if (!_posets[i]->IsMaximal(items[i]))
                return false;
        return true;
    }

    /*! 
		Найти число элементов в dataset, следующих за item
    
	template <typename Item>
    int GetSupportLevel( PosetItemsMatrix const& dataset, Item const& item) const
    {
        int s = 0;
        for (int i = 0, m = dataset.GetHeight(); i != m; ++i)
            if (Preceq(item, dataset[i]))
                ++s;
        return s;
    }*/

    /*!
		Найти число элементов в dataset, следующих за item
    */
    template<typename Item>
    int GetSupportLevel(PosetItemsMatrix const& dataset, Item const& item, int threshold) const {
        int s = 0;
        for (int i = 0, m = dataset.GetHeight(); i != m; ++i)
            if (Preceq(item, dataset[i])) {
                ++s;
                if (s == threshold)
                    break;
            }
        return s;
    }

    template<typename Item>
    int UpdateSupportLevel(PosetItemsMatrix const& dataset,
                           Item const& item,
                           Indecies const& support,
                           int threshold) const {
        int s = 0;
        for (int i : support) {
            if (Preceq(item, dataset[i])) {
                ++s;
                if (s == threshold)
                    break;
            }
        }
        return s;
    }

    template<typename Item>
    int UpdateUnsupportLevel(PosetItemsMatrix const& dataset,
                             Item const& item,
                             Indecies const& unsupport,
                             int threshold) const {
        int s = 0;
        for (int i : unsupport) {
            if (!Preceq(item, dataset[i])) {
                ++s;
                if (s == threshold)
                    break;
            }
        }
        return s;
    }

    /*!
		Найти число элементов в dataset, не следующих за item
    */
    template<typename Item>
    int GetUnsupportLevel(PosetItemsMatrix const& dataset, Item const& item) const {
        int s = 0;
        for (int i = 0, m = dataset.GetHeight(); i != m; ++i)
            if (!Preceq(item, dataset[i]))
                ++s;
        return s;
    }

    /// Найти номера элементов в dataset, следующих за items
    void GetSupportItems(PosetItemsMatrix const& dataset, PosetItems const& items, Indecies& support) const;

    /// Обновить номера элементов в dataset, следующих за items после увеличения items
    void UpdateSupportItems(PosetItemsMatrix const& dataset, PosetItems const& items, Indecies& support) const;

    /// Найти номера элементов в dataset, не следующих за items
    void GetUnsupportItems(PosetItemsMatrix const& dataset, PosetItems const& items, Indecies& unsupport) const;

    /// Обновить номера элементов в dataset, не следующих за items после уменьшения items
    void UpdateUnsupportItems(PosetItemsMatrix const& dataset, PosetItems const& items, Indecies& unsupport) const;

    /// Load items from CSV file
    PosetItemsMatrix LoadItems(std::string const& filename, Options const& options);

    /// Reverse posets
    void Reverse();

    /*!
        Преобразовать элемент в строку, разделяя координаты указанным сепаратором
    */
    template<typename Items>
    std::string ToString(Items const& items, std::string const& separator = ",") const {
        std::ostringstream oss;

        for (int i = 0, n = items.size(); i != n; ++i) {
            if (i)
                oss << separator;
            oss << _posets[i]->ToString(items[i]);
        }

        return oss.str();
    }

    std::string ToString(PosetItemsMatrix const& matrix,
                         std::string const& separator = ",",
                         const std::string& rowSep = "\n") const;

private:
    // Is i-th item support for x item
    int CheckSupport(PosetItemsMatrix::CRowRef const& idealBaseRow, PosetItems const& x) const;
};
