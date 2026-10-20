#pragma once

#include <assert.h>
#include <memory>
#include <vector>

#include "Callbacks.h"
#include "IPoset.h"
#include "PosetsDualizationTask.h"
//#include "ISolver.h"
#include "DualizationCounters.h"
#include "Registrator.h"
#include "bit_vector.h"

namespace PosetDualization {
namespace MIGRAS {

typedef std::vector<int> SupportItems;

typedef std::vector<int> DependentItems;

typedef std::vector<SupportItems> SupportMap;

typedef std::pair<int, PosetItem> ItemOfPoset;

typedef std::vector<ItemOfPoset> ItemsOfPosets;

//////////////////////////////////////////////////////////////////////////
// Local state of algorithm of dualization over posets MIGRAS
//////////////////////////////////////////////////////////////////////////
struct LocalState {
    // Lists of support items numbers for each poset
    SupportMap supportMap;

    // Items selected for cover dependent rows for each poset
    std::vector<PosetItems> itemsForCover;

    // Current dependent items
    DependentItems dependentItems;

    LocalState();

    LocalState(int m, int n);

    void Reserve(int m, int n);

    void Clear();
};

typedef std::vector<LocalState> LocalStates;

struct LocalStatesPool {
    LocalStates _states;

public:
    void Reserve(int q);

    LocalState Get(int m, int n);

    void Release(LocalState state);
};

/**
    Options for MIGRAS algorithm
    */
struct DualizationAlgorithmOptions {

    enum BaseRowSelectionTypeT {
        None = 0,
        First = 1,
        MinCoverItemCount = 2
    };

    BaseRowSelectionTypeT BaseRowSelectionType;

    int ScanRowsLimit;

    bool RandomizeBaseRowSelection;

    bool CollectResultsInMemory;

    std::string Name;

    DualizationAlgorithmOptions() :
            RandomizeBaseRowSelection(false),
            BaseRowSelectionType(None),
            ScanRowsLimit(-1),
            CollectResultsInMemory(false),
            MaxResults(-1),
            MaxDelay(-1),
            MinResults(1) {}

    int MaxResults;  ///< Максимальное число решений

    int MaxDelay;  ///< Максимальная задержка при поиске очередного решения

    int MinResults;  ///< Минимальное число решений, которое должно быть найдено до того, как выполнить остановку по превышению предела MaxDelay
};

struct CallbackParams {
    ICallback* Callback;
    int Delay;
    int Tick;

    CallbackParams(ICallback* callback = 0, int delay = 100) : Tick(0), Delay(delay), Callback(callback) {}

    ICallback* GetCallback() {
        if (!Callback)
            return 0;

        if (Tick >= Delay) {
            Tick = 0;
            return Callback;
        }

        ++Tick;
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////
// Algorithm of dualization over posets MIGRAS
//////////////////////////////////////////////////////////////////////////
class DualizationAlgorithm : public IConfigurable {
    LocalStatesPool _localStatesPool;  ///< Пул локальных состояний

    int _n;  ///< Number of posets

    PosetsDualizationNode _idealNode;  ///< Текущий узел дерева решений поиска
            ///< максимального независимого от идеала элемента

    PosetsDualizationNode _filterNode;  ///< Текущий узел дерева решений поиска
            ///< минимального независимого от фильтра элемента

    PosetsDualizationTask const* _task;  ///< Решаемая задача

    bool _reversed;  ///< Флаг того, что задача была заменена на обратную

    PosetItems _checkSupportBuffer;  ///< Буффер для проверки совместимости элементов

    PosetItems _columnItemsBuffer;  ///< Буффер для отбора непокрытых элементов в столбцах

    PosetItems _supportItemsBuffer;  ///< Буффер для отбора элементов опорных строк

    PosetItems
            _maximalIndepBuffer;  ///< Буффер для отбора максимальных независимых элементов, используемых для покрытия

    bool _stopped;

public:
    DualizationCounters Counters;  ///< Счётчики процесса дуализации

    CallbackParams ResultCallback;  ///< Параметры обратного вызова для фиксации результатов

    CallbackParams InnerCallback;  ///< Параметры обратного вызова для внутренних состояний

    DualizationAlgorithmOptions Options;  ///< Опции работы алгоритма

    PosetItemsMatrix IdealIndependent;  ///< Независимые от идеала результаты

    PosetItemsMatrix FilterIndependent;  ///< Независимые от фильтра результаты

public:
    DualizationAlgorithm();

    void Dualize(PosetsDualizationTask const& task);

    void DualizeIntervals(PosetsDualizationTask const& task);

    void DualizeIntervalsDirect(PosetsDualizationTask const& task);

private:
    void ReverseTask();

    void ReverseTask(LocalState& idealState, LocalState& filterState);

    LocalState CreateRootState(int m);

    Posets const& GetPosets() const;

    IPoset const& GetPoset(int j) const;

    PosetItems const& GetColumn(int j) const;

    CallbackResult DoCallback(PosetsDualizationNode::NodeType type);

    void DoDualize(LocalState& state);

    void DoDualize(LocalState& idealState, LocalState& filterState);

    int SelectItemsForCover(LocalState& state);

    // Удалить элементы, отобранные для покрытия, если они не сравнимы с верхними границами
    int BoundItemsForCover(LocalState& state, PosetItems const& upperBounds);

    // Посчитать элементы, отобранные для покрытия и сравнимые с верхними границами
    int CountItemsForCover(LocalState& state, PosetItems const& upperBounds);

    int CountRowCoverItems(LocalState const& state, int i);

    void CollectCandidatesForCover(LocalState& state);

    void CollectCandidatesForCover(int j, LocalState& state);

    void CollectCandidatesForCover(int j, LocalState& state, PosetItems const& supportItems);

    void RemoveIfUsedOrUnsupported(LocalState& state, int j);

    void DoComplete(LocalState& state, int j0 = 0);

    void DoComplete(LocalState& idealState, LocalState& filterState, int j0 = 0);

    bool CheckSupport(LocalState const& state, int j, PosetItem const& a);

    bool CheckSupport(LocalState const& state, int j, PosetItem const& a, int l);

    void InitChildState(LocalState const& state, LocalState& child, int j, PosetItem const& a);

    int SelectBaseRowByCoverItemCount(LocalState& state);

    /*!
            Найти неопределённую координату в текущем независимом от идеала наборе
            \param j0 --- координата, с которой начинать поиск
        */
    int FindFirstUndefinedCoordinate(int j0);

public:
    /*!
            Установить опции работы алгоритма
        */
    virtual void SetOptions(::Options const& options);

    /*!
            Применить опции по умолчанию
        */
    virtual void SetDefaultOptions();
};

}  // namespace MIGRAS
}  // namespace PosetDualization