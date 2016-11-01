#include "stdafx.h"

#include "logging_utils.h"

#include "MIGRAS.h"

namespace PosetDualization 
{
    namespace MIGRAS
    {

        int DualizationAlgorithm::SelectBaseRowByCoverItemCount(LocalState & state)
        {
            int i = -1;
            int minCount = 0;
            int c = 0;

            for (auto t : state.dependentItems)
            {
                int count = CountRowCoverItems(state, t);
                if (i == -1 || count < minCount)
                {
                    i = t;
                    minCount = count;
                }

                if (Options.ScanRowsLimit > 0)
                {
                    ++c;
                    if (c>Options.ScanRowsLimit)
                        break;
                }
            }

            return i;
        }


        void DualizationAlgorithm::SetDefaultOptions()
        {
            Options = DualizationAlgorithmOptions();
        }

        void DualizationAlgorithm::SetOptions(::Options const& options)
        {
            DualizationAlgorithmOptions defaultOptions;

            Options.BaseRowSelectionType = (DualizationAlgorithmOptions::BaseRowSelectionTypeT)
                options.get("BaseRowSelectionType", (int)defaultOptions.BaseRowSelectionType);
            Options.RandomizeBaseRowSelection = options.get("RandomizeBaseRowSelection", defaultOptions.RandomizeBaseRowSelection);
            Options.ScanRowsLimit = options.get("ScanRowsLimit", defaultOptions.ScanRowsLimit);
        }

        void DualizationAlgorithm::InitChildState(
            LocalState const& state, 
            LocalState& child, 
            int j, PosetItem const& a)
        {
            child.Clear();

            auto& poset = GetPoset(j);
            auto& column = GetColumn(j);

            for(auto i : state.dependentItems)
            {
                if (poset.Preceq(column[i], a))
                    child.dependentItems.push_back(i);
                else
                    child.supportMap[j].push_back(i);
            }

            for(int l = 0; l != _n; ++l)
            {
                if (l == j)
                    continue;

                for (auto i : state.supportMap[l])
                {
                    if (poset.Preceq(column[i], a))
                        child.supportMap[l].push_back(i);
                }
            }
        }



        bool DualizationAlgorithm::CheckSupport(
            LocalState const& state, 
            int changingColumn, 
            PosetItem const& newItem, 
            int checkedColumn)
        {
            _checkSupportBuffer.clear();

            auto& poset = GetPoset(changingColumn);
            auto& column = GetColumn(changingColumn);

            auto& poset2 = GetPoset(checkedColumn);
            auto& column2 = GetColumn(checkedColumn);

            for (auto t : state.supportMap[checkedColumn])
            {
                if (poset.Preceq(column[t], newItem))
                    _checkSupportBuffer.push_back(column2[t]);
            }

            return ! _checkSupportBuffer.empty() && 
                poset2.IsMaximalIndependent(_idealNode.CurrentPosetItems[checkedColumn], _checkSupportBuffer);
        }

        bool DualizationAlgorithm::CheckSupport(
            LocalState const& state, 
            int changingColumn, PosetItem const& newItem)
        {
            for (int l = 0; l != _n; ++l)
            {
                if (changingColumn == l)
                    continue;

                if (state.supportMap[l].empty())
                    continue;

                if (! _idealNode.IsSetted(l))
                    continue;

                if (GetPoset(l).IsMaximal(_idealNode.CurrentPosetItems[l]))
                    continue;

                if (! CheckSupport(state,  changingColumn, newItem, l))
                    return false;
            }

            return true;
        }

        void DualizationAlgorithm::DoComplete(
            LocalState& state, int j0 /*= 0*/)
        {
            if (DoCallback(PosetsDualizationNode::Inner) == CallbackResult::Prun)
                return;

            if (_stopped)
                return;

            int j = FindFirstUndefinedCoordinate(j0);
            if (j == -1)
            {
                auto& result = _idealNode.CurrentPosetItems;
                if (! _task->GetPosetsProduct().IsFilterIndependent(_task->GetFilterBase(), result))
                {
                    DoCallback(PosetsDualizationNode::Extra);
                    return;
                }

                assert(_task->GetPosetsProduct().IsMaximalIndependent(_task->GetIdealBase(), result));

                if (Options.CollectResultsInMemory)
                    IdealIndependent.AddRow(result);

                DoCallback(PosetsDualizationNode::Result);
                return;
            }


            state.itemsForCover[j].clear();
            GetPoset(j).GetMaximal(state.itemsForCover[j]);
            RemoveIfUsedOrUnsupported(state, j);

            if (state.itemsForCover[j].empty())
            {
                DoCallback(PosetsDualizationNode::Extra);
                return;
            }

            auto child = _localStatesPool.Get(state.dependentItems.size(), _n);
            _idealNode.Set(j);
            for (auto& a :state.itemsForCover[j])
            {
                InitChildState(state, child, j, a);
                _idealNode.Set(j, a);
                DoComplete(child, j+1);
                if (_stopped)
                    break;
            }
            _idealNode.Reset(j);
            _localStatesPool.Release(child);
        }

        void DualizationAlgorithm::DoComplete( LocalState& idealState, LocalState& filterState, int j0 /*= 0*/ )
        {
            if (DoCallback(PosetsDualizationNode::Inner) == CallbackResult::Prun)
                return;

            if (_stopped)
                return;


            int j = FindFirstUndefinedCoordinate(j0);
            if (j == -1)
            {
                ReverseTask(idealState, filterState);

                j = FindFirstUndefinedCoordinate(0);
                if (j != -1)
                    DoComplete(idealState, filterState, j);
                
                ReverseTask(idealState, filterState);
                
                if (j == -1)
                {
                    assert(_task->GetPosetsProduct().Preceq(_filterNode.CurrentPosetItems, _idealNode.CurrentPosetItems));
                    assert(_task->GetPosetsProduct().IsMaximalIndependent(_task->GetIdealBase(), _idealNode.CurrentPosetItems));
                    assert(_task->GetPosetsProduct().IsMinimalIndependent(_task->GetFilterBase(), _filterNode.CurrentPosetItems));

                    auto& lower = _reversed ? _idealNode.CurrentPosetItems : _filterNode.CurrentPosetItems;
                    auto& upper = !_reversed ? _idealNode.CurrentPosetItems : _filterNode.CurrentPosetItems;
                                    
                    if (Options.CollectResultsInMemory)
                    {
                        IdealIndependent.AddRow(upper);
                        FilterIndependent.AddRow(lower);
                    }

                    DoCallback(PosetsDualizationNode::Result);
                }
                
                return;
            }


            idealState.itemsForCover[j].clear();
            GetPoset(j).GetMaximal(idealState.itemsForCover[j]);
            RemoveIfUsedOrUnsupported(idealState, j);

            auto& poset = GetPoset(j);
            auto lb = _filterNode.CurrentPosetItems[j];
            remove_if(idealState.itemsForCover[j],
                [&poset, lb](PosetItem const& a)
            {
                return ! poset.Preceq(lb, a);
            });
            
            if (idealState.itemsForCover[j].empty())
            {
                DoCallback(PosetsDualizationNode::Extra);
                return;
            }

            auto child = _localStatesPool.Get(0, _n);
            _idealNode.Set(j);
            for (auto& a :idealState.itemsForCover[j])
            {
                InitChildState(idealState, child, j, a);
                _idealNode.Set(j, a);
                DoComplete(child, filterState, j+1);
                if (_stopped)
                    break;
            }
            _idealNode.Reset(j);
            _localStatesPool.Release(child);
        }
        

        void DualizationAlgorithm::CollectCandidatesForCover(LocalState &state)
        {
            for (int j = 0; j != _n; ++j)
            {
                CollectCandidatesForCover(j, state);
            }
        }

        

        void DualizationAlgorithm::CollectCandidatesForCover(int j, LocalState &state, PosetItems const& supportItems)
        {
            auto& poset = GetPoset(j);
            poset.GetMaximalIndependent(supportItems,  _maximalIndepBuffer);
            for (auto& b : _maximalIndepBuffer)
            {
                assert(! poset.InIdeal(b, supportItems));
                assert(poset.IsMaximalIndependent(b, supportItems));
                
                if (! Contains(poset, state.itemsForCover[j], b))
                    state.itemsForCover[j].push_back(b);                
            }
        }
        

            
        void DualizationAlgorithm::CollectCandidatesForCover(int j, LocalState &state)
        {
            state.itemsForCover[j].clear();
            if (_idealNode.IsSetted(j))
                return;

            auto& poset = GetPoset(j);            
            auto& column = GetColumn(j);

            _columnItemsBuffer.clear();
            for (auto t:state.dependentItems)
                _columnItemsBuffer.push_back(column[t]);
            poset.SelectUnique(_columnItemsBuffer);
            
            state.itemsForCover[j].assign(1, poset.GetUniversalMaximal());

            auto beg = _columnItemsBuffer.begin();
            auto end = _columnItemsBuffer.end();

            for (int k = 0; k != state.itemsForCover[j].size(); ++k)
            {
                auto mid = std::partition(beg, end, 
                    [&poset, &state, j, k](PosetItem const& x)
                {return poset.Preceq(x, state.itemsForCover[j][k]);});
                _supportItemsBuffer.assign(mid, end);
                
                
                if (! _supportItemsBuffer.empty())
                {
                    assert( poset.IsMaximalIndependent(state.itemsForCover[j][k], _supportItemsBuffer));
                    CollectCandidatesForCover(j, state, _supportItemsBuffer);
                }
                
                for (auto i = beg; i != mid; ++i)
                {
                    _supportItemsBuffer.push_back(*i);
                    CollectCandidatesForCover(j, state, _supportItemsBuffer);
                    _supportItemsBuffer.pop_back();
                }

            }

            remove_if(state.itemsForCover[j], 
                [&poset, j, &state, this]
            (PosetItem const& b)
            {
                return _idealNode.WasUsed(poset, j, b) || ! CheckSupport( state, j, b);
            });
            
        }

        int DualizationAlgorithm::CountRowCoverItems(LocalState const& state, int i)
        {
            int s = 0;
            for (int j = 0; j != _n; ++j)
            {
                auto& poset = GetPoset(j);
                auto& a = GetColumn(j)[i];
                                
                for (auto x : state.itemsForCover[j])
                {
                    if (! poset.Preceq(a, x))
                        ++s;
                }
            }

            return s;
        }

        int DualizationAlgorithm::SelectItemsForCover(LocalState &state)
        {
            int targetItemIndex = -1;
            switch (Options.BaseRowSelectionType)
            {
            case DualizationAlgorithmOptions::None:
                break;

            case DualizationAlgorithmOptions::MinCoverItemCount:
                if (Options.RandomizeBaseRowSelection)
                    random_shuffle(state.dependentItems);
                targetItemIndex = SelectBaseRowByCoverItemCount(state);
                break;


            case DualizationAlgorithmOptions::First:            
                if (Options.RandomizeBaseRowSelection)
                {
                    int m = state.dependentItems.size();
                    int r = rand() % m;
                    if (r < 0 || r >= m)
                        r = 0;
                    targetItemIndex = state.dependentItems[r];
                }
                else
                {
                    targetItemIndex = state.dependentItems.front();            
                }
                break;
            }       

            int r = 0;

            for (int j = 0; j != _n; ++j)
            {
                auto& poset = GetPoset(j);
                auto& forCover = state.itemsForCover[j];

                if (targetItemIndex != -1)
                {
                    auto a = GetColumn(j)[targetItemIndex];
                    remove_if(forCover,
                        [a, &poset](PosetItem const& x){return poset.Preceq(a, x); });
                }

                r += forCover.size();
            }

            return r;
        }



        void DualizationAlgorithm::DoDualize(LocalState& state)
        {
            if (state.dependentItems.empty())
            {
                DoComplete(state);
                return;
            }

            if (DoCallback(PosetsDualizationNode::Inner) == CallbackResult::Prun)
                return;

            if (_stopped)
                return;

            CollectCandidatesForCover(state);
            int r = SelectItemsForCover(state);

            if (r == 0)
            {
                DoCallback(PosetsDualizationNode::Extra);
                return;
            }

            _idealNode.StoreUsedItemsSizes();
            auto child = _localStatesPool.Get(state.dependentItems.size(), _n);

            for (int j = 0; j !=_n; ++j)
            {
                if (state.itemsForCover[j].empty())
                    continue;
                _idealNode.Set(j);
                for (auto& a : state.itemsForCover[j])
                {
                    _idealNode.Use(j, a);
                    InitChildState(state, child, j, a);
                    _idealNode.Set(j, a);
                    DoDualize(child);
                    if (_stopped)
                        break;
                }
                _idealNode.Reset(j);

                if (_stopped)
                    break;
            }
            
            _localStatesPool.Release(child);
            _idealNode.RestoreUsedItemsSizes();
        }

        void DualizationAlgorithm::DoDualize( LocalState& idealState, LocalState& filterState )
        {
            if (idealState.dependentItems.empty() && filterState.dependentItems.empty())
                return DoComplete(idealState, filterState);

            if (DoCallback(PosetsDualizationNode::Inner) == CallbackResult::Prun)
                return;

            if (_stopped)
                return;

            int r1 = 0;
            if (! idealState.dependentItems.empty())
            {
                CollectCandidatesForCover(idealState);
                r1 = SelectItemsForCover(idealState);            
                if (! r1)
                {
                    DoCallback(PosetsDualizationNode::Extra);
                    return;
                }
            }

            int r2 = 0;            
            if (! filterState.dependentItems.empty())
            {
                r2 = CountItemsForCover(filterState, _idealNode.CurrentPosetItems);
                if (! r2)
                {
                    DoCallback(PosetsDualizationNode::Extra);
                    return;
                }            
            }

            bool reversed = r1 == 0 || (r2 != 0 && r2 < r1);
            if (reversed)
                ReverseTask(idealState, filterState);

            _idealNode.StoreUsedItemsSizes();
            auto child = _localStatesPool.Get(idealState.dependentItems.size(), _n);

            for (int j = 0; j !=_n; ++j)
            {
                if (idealState.itemsForCover[j].empty())
                    continue;

                auto& poset = GetPoset(j);
                auto lb = _filterNode.CurrentPosetItems[j];

                _idealNode.Set(j);
                for (auto& a : idealState.itemsForCover[j])
                {
                    if (! poset.Preceq(lb, a))
                        continue;
                    
                    _idealNode.Use(j, a);
                    InitChildState(idealState, child, j, a);
                    _idealNode.Set(j, a);
                    DoDualize(child, filterState);
                    if (_stopped)
                        break;
                }
                _idealNode.Reset(j);
                if (_stopped)
                    break;
            }

            _localStatesPool.Release(child);
            _idealNode.RestoreUsedItemsSizes();

            if (reversed)
                ReverseTask(idealState, filterState);
        }
        
        
        CallbackResult DualizationAlgorithm::DoCallback(
            PosetsDualizationNode::NodeType type)
        {
            ICallback* callback = 0;
            CallbackResult r = CallbackResult::Continue;
            switch(type)
            {
            case PosetsDualizationNode::Result:
                Counters.IncResult();
                callback = ResultCallback.GetCallback();
                if (! callback && ResultCallback.Callback)
                {
                    r = CallbackResult::WasDelayed;
                }
                if (Options.MaxResults >= 0 && Counters.results >= Options.MaxResults)
                {
                    r = CallbackResult::Stop;
                    _stopped = true;
                }
                break;
            case PosetsDualizationNode::Extra:
                Counters.IncExtra();
            case PosetsDualizationNode::Inner:
                Counters.IncInners();
                callback = InnerCallback.GetCallback();
                if (! callback && InnerCallback.Callback)
                {
                    r = CallbackResult::WasDelayed;
                }
                if (Options.MaxDelay >=0 && Counters.currentDelay >= Options.MaxDelay &&
                     (Options.MinResults <= 0 || Counters.results >= Options.MinResults))
                {
                    r = CallbackResult::Stop;
                    _stopped = true;
                }
                break;
            }

            if (callback)
            {
                _idealNode.Type = type;
                auto r1 = callback->Call(*_task, _idealNode);
                if (r1 == CallbackResult::Stop)
                    _stopped = true;
                if (r != CallbackResult::Stop)
                    return r1;
            }

            return r;
        }

        void DualizationAlgorithm::Dualize(PosetsDualizationTask const& task)
        {
            _task = &task;

            Counters.Reset();
            
            _n = task.GetPosetsProduct().GetPosets().size();    
            int m = task.GetIdealBase().GetHeight();

            LOG_SEV(info) << "IdealBaseSize: " << m;
            LOG_SEV(info) << "Posets: " << _n;

            _localStatesPool.Reserve(m+_n+1);

            IdealIndependent.Resize(0, _n);
            FilterIndependent.Reserve(0, _n);

            _idealNode.Init(task.GetPosetsProduct().GetPosets());
            _filterNode.Init(task.GetPosetsProduct().GetReversedPosets());
            
            //_checkFilter = _task->GetFilterBaseSize() > 0;
            _reversed = false;
            _stopped = false;
                        
            auto root = CreateRootState(m);
            DoDualize(root);

            if (ResultCallback.Callback && ResultCallback.Tick > 0)
                ResultCallback.Callback->Call(*_task, _idealNode);

            _localStatesPool.Release(root);
            _task = 0;
        }


        void DualizationAlgorithm::DualizeIntervals( PosetsDualizationTask const& task )
        {
            _task = &task;
            _n = task.GetPosetsProduct().GetPosets().size();    
            int m1 = task.GetIdealBase().GetHeight();
            int m2 = task.GetFilterBase().GetHeight();

            LOG_SEV(info) << "Posets: " << _n;
            LOG_SEV(info) << "IdealBaseSize: " << m1;
            LOG_SEV(info) << "FilterBaseSize: " << m2;
            
            _localStatesPool.Reserve(m1 + _n*2 + m2);

            IdealIndependent.Resize(0, _n);
            FilterIndependent.Resize(0, _n);

            Counters.Reset();

            _idealNode.Init(task.GetPosetsProduct().GetPosets());
            _filterNode.Init(task.GetPosetsProduct().GetReversedPosets());

            _reversed = false;
            _stopped = false;

            auto idealRoot = CreateRootState(m1);
            auto filterRoot = CreateRootState(m2);

            CollectCandidatesForCover(idealRoot);
            SelectItemsForCover(idealRoot);

            ReverseTask(idealRoot, filterRoot);

            DoDualize(idealRoot, filterRoot);

            //TODO:
            //if (ResultCallback.Callback && ResultCallback.Tick > 0)
              //  ResultCallback.Callback->Call(*_task, _currentIdealIndep);

            ReverseTask();

            _localStatesPool.Release(idealRoot);
            _localStatesPool.Release(filterRoot);
            
            _task = 0;
        }

        DualizationAlgorithm::DualizationAlgorithm() 
            :_n(0), _task(0), _stopped(false), _reversed(false)
        {
        }

        
        void DualizationAlgorithm::RemoveIfUsedOrUnsupported(LocalState& state, int j )
        {
            auto& poset = GetPoset(j);
            remove_if(state.itemsForCover[j], 
                [&state, j, &poset, this](PosetItem const& a){
                    return _idealNode.WasUsed(poset, j, a) || ! CheckSupport(state, j, a);});
        }

        LocalState DualizationAlgorithm::CreateRootState( int m )
        {
            auto root = _localStatesPool.Get(m, _n);
            root.Clear();
            for (int i = 0; i != m; ++i)
                root.dependentItems.push_back(i);
            return root;
        }

        void DualizationAlgorithm::ReverseTask()
        {
            _task->ReverseTask();
            std::swap(_idealNode, _filterNode);
            //_checkFilter = _task->GetFilterBaseSize() != 0;
            _reversed = !_reversed;
        }

        void DualizationAlgorithm::ReverseTask( LocalState& idealState, LocalState& filterState )
        {
            std::swap(idealState, filterState);
            ReverseTask();
        }
        
        int DualizationAlgorithm::BoundItemsForCover( LocalState &state, PosetItems const& upperBounds )
        {
            int r = 0;
            for (int j = 0; j != _n; ++j)
            {
                auto& poset = GetPoset(j);
                auto ub = upperBounds[j];
                remove_if(state.itemsForCover[j], 
                    [ub, j, &poset](PosetItem const& a)
                {
                    return ! poset.Preceq(a, ub);
                });

                r += state.itemsForCover[j].size();
            }

            return r;
        }

        int DualizationAlgorithm::CountItemsForCover( LocalState &state, PosetItems const& upperBounds )
        {
            int r = 0;
            for (int j = 0; j != _n; ++j)
            {
                auto& poset = GetPoset(j);
                auto ub = upperBounds[j];
                for (auto& a : state.itemsForCover[j])
                    if (poset.Preceq(a, ub))
                        ++r;
            }

            return r;
        }

        void DualizationAlgorithm::DualizeIntervalsDirect( PosetsDualizationTask const& task )
        {
            auto ideal = task.GetIdealBase();
            auto filter = task.GetFilterBase();
            
            Options.CollectResultsInMemory = true;

            PosetsDualizationTask task1;
            task1.GetPosetsProduct().SetPosets(task.GetPosetsProduct().GetPosets(), task.GetPosetsProduct().GetBindings());
            task1.GetIdealBase() = std::move(ideal);
            Dualize(task1);
            auto upper = std::move(IdealIndependent);

            PosetsDualizationTask task2;
            task2.GetPosetsProduct().SetPosets(task.GetPosetsProduct().GetReversedPosets(), task.GetPosetsProduct().GetBindings());
            task2.GetIdealBase() = std::move(filter);
            Dualize(task2);
            auto lower = std::move(IdealIndependent);

            Counters.Reset();

            
            for (auto l = 0, m1 = lower.GetHeight(), m2 = upper.GetHeight(); l != m1; ++l)
            {
                auto lrow = lower[l];
                for (auto u = 0; u != m2; ++u)
                {
                    auto urow = upper[u];
                    if (task.GetPosetsProduct().Preceq(lrow, urow))
                    {
                        IdealIndependent.AddRow(urow);
                        FilterIndependent.AddRow(lrow);
                        Counters.IncResult();
                    }
                    else
                    {
                        Counters.IncExtra();
                    }
                }
            }
        }

        Posets const& DualizationAlgorithm::GetPosets() const
        {
            assert(_task);
            return _task->GetPosetsProduct().GetPosets();
        }

        IPoset const& DualizationAlgorithm::GetPoset( int j ) const
        {
            assert(_task);
            assert(inside(_task->GetPosetsProduct().GetPosets(), j));
            return *_task->GetPosetsProduct().GetPosets()[j];
        }

        PosetItems const& DualizationAlgorithm::GetColumn( int j ) const
        {
            assert(_task);
            //assert(inside(_task->GetIdealBase(), j));
            return _task->GetIdealBase().GetColumn(j);
        }

        int DualizationAlgorithm::FindFirstUndefinedCoordinate( int j0 )
        {
            for (int j = j0; j != _n; ++j)
            {
                auto& poset = GetPoset(j);
                if (! poset.IsItem(_idealNode.CurrentPosetItems[j]))
                    return j;
            }
            return -1;
        }

        void LocalState::Clear()
        {
            dependentItems.clear();

            for (auto& s : supportMap)
                s.clear();

            for (auto& c : itemsForCover)
                c.clear();
        }

        void LocalState::Reserve( int m, int n )
        {
            supportMap.resize(n);
            itemsForCover.resize(n);
            dependentItems.reserve(m);
        }

        LocalState::LocalState( int m, int n )
        {
            Reserve(m, n);
        }

        LocalState::LocalState()
        {
        }

        void LocalStatesPool::Release( LocalState state )
        {
            _states.push_back(state);
        }

        LocalState LocalStatesPool::Get( int m, int n )
        {
            if (_states.empty())
                return LocalState(m, n);

            auto s = std::move(_states.back());
            _states.pop_back();

            if (s.supportMap.size()!=n)
                s.Reserve(m, n);
            return s;
        }

        void LocalStatesPool::Reserve( int q )
        {
            _states.reserve(q);
        }
    } // namespace MIGRAS
} //namespace PosetsDualization 