#include "stdafx.h"

#include "FrequentInfrequentEnumerator.h"
#include "io_utils.h"




void PosetDualization::FrequentInfrequentEnumerator::Enumerate( FrequentInfrequentTask const& task )
{
    _task = &task;
	int n = task.GetPosetsProduct().GetPosets().size();
    int r = task.GetThresholds().size();
    Results.resize(r);

    for (int i = 0; i != r; ++i)
    {
        Results[i].Clear(n);
        Results[i].threshold = task.GetThresholds()[i];
		Results[i].complete = false;
    }

	sort(Results, 
		[](FrequentInfrequentDivision const& d1, 
		FrequentInfrequentDivision const& d2)
	{return d1.threshold < d2.threshold;});

	if (Results.empty())
		return;
    
	_dualizer.Options.MaxDelay = Options.maxDelay;
    _dualizer.Options.MinResults = Options.minIndepCount;
    _dualizer.Options.MaxResults = Options.maxIndepCount;
    _dualizer.Options.BaseRowSelectionType = MIGRAS::DualizationAlgorithmOptions::MinCoverItemCount;
    _dualizer.Options.CollectResultsInMemory = true;
    _dualTask.GetPosetsProduct() = _task->GetPosetsProduct();
		
    bool complete = false;

    while(! complete)
    {
        complete = true;
        for (int i = 0; i != r; ++i)
        {
            if (Results[i].complete)
                continue;
            DoEnumerate(Results[i]);
            complete &= Results[i].complete;
        }     
    }

    _task = 0;
}

void PosetDualization::FrequentInfrequentEnumerator::DoEnumerate( FrequentInfrequentDivision& division )
{
    _dualTask.GetIdealBase() = std::move(division.minimalInfrequent);
    _dualTask.GetFilterBase() = std::move(division.maximalFrequent);

    _dualizer.DualizeIntervals(_dualTask);

    division.minimalInfrequent = std::move(_dualTask.GetIdealBase());
    division.maximalFrequent = std::move(_dualTask.GetFilterBase());

    if (_dualizer.Counters.results == 0)
    {
        division.complete = true;
        return;
    }

    //_removeMask.zero((int)_dualizer.Counters.results);

    for (int i = 0; i != _dualizer.Counters.results; ++i)
    {
        _dualizer.IdealIndependent.AddRow(_dualizer.FilterIndependent[i]);
    }

    //_dualizer.IdealIndependent.RemoveRows(_removeMask);
    _dualizer.IdealIndependent.DeleteDuplicateRows();
    //_dualizer.FilterIndependent.DeleteDuplicateRows();

    //auto idealIndepStr = _task->GetPosetsProduct().ToString(_dualizer.IdealIndependent);
    //auto filterIndepStr = _task->GetPosetsProduct().ToString(_dualizer.FilterIndependent);

    //write_all("idealIndep.csv", idealIndepStr);
    //write_all("filterIndep.csv", filterIndepStr);


    AddResults(_dualizer.IdealIndependent);
    //AddResults(_dualizer.FilterIndependent);
}

void PosetDualization::FrequentInfrequentEnumerator::AddResults( PosetItemsMatrix const& items )
{
    auto& pp = _task->GetPosetsProduct();
    for (int i = 0, m = items.GetHeight(); i != m; ++i)
    {
        int t = pp.GetSupportLevel(_task->GetDatabase(), items[i], Results.back().threshold);        
		int p = find_if(Results, [t](FrequentInfrequentDivision const& div){return div.threshold <= t;});
		int r = Results.size();
		if (p == -1)
			p = r;


		//if (p != -1)
		//{
			items.CopyRow(i, _current);
			for (int u = p; u != r; ++u)
				AddMaximalFrequent(Results[u], _current);
		//}

		//if (p != 0)
		//{
			items.CopyRow(i, _current);
			for (int u = p-1; u != -1; --u)
				AddMinimalInfrequent(Results[u], _current);
		//}
    }
}

void PosetDualization::FrequentInfrequentEnumerator::AddMaximalFrequent( FrequentInfrequentDivision& division, PosetItems& items )
{
    auto& pp = _task->GetPosetsProduct();
    int m = division.maximalFrequent.GetHeight();
    
	for (int i = 0; i != m; ++i)
    {
        if (pp.Preceq(items, division.maximalFrequent[i]))
            return;
        assert(! pp.Preceq(division.maximalFrequent[i], items));
    }

    FindMaximalFrequent(division.threshold, items);    
    division.maximalFrequent.AddRow(items);    
}

void PosetDualization::FrequentInfrequentEnumerator::AddMinimalInfrequent( FrequentInfrequentDivision& division, PosetItems& items )
{
    auto& pp = _task->GetPosetsProduct();
    int m = division.minimalInfrequent.GetHeight();
    for (int i = 0; i != m; ++i)
    {
        if (pp.Preceq(division.minimalInfrequent[i], items))
            return;
        assert(! pp.Preceq(items, division.minimalInfrequent[i]));
    }

    FindMinimalInfrequent(division.threshold, items);
    division.minimalInfrequent.AddRow(items);

  
}

void PosetDualization::FrequentInfrequentEnumerator::FindMaximalFrequent( int threshold, PosetItems& current)
{
	if (_task->GetPosetsProduct().IsMaximal(current))
		return;
	
	_task->GetPosetsProduct().GetSupportItems(_task->GetDatabase(), current, _support);
		
	if ((int)_support.size() > threshold)
        DecreaseFrequency(threshold, current, _support);
    
    int n = _task->GetPosetsProduct().GetPosets().size();
    for (int j = 0; j !=n; ++j)
    {
        auto& poset = _task->GetPosetsProduct().GetPoset(j);
        auto& column = _task->GetDatabase().GetColumn(j);

        _forCover.clear();
        for (auto i : _support)
        {
            _forCover.push_back(column[i]);
        }

        poset.SelectUnique(_forCover);
        poset.GetMaxLowerBounds(_forCover, _bounds);

        for (auto b: _bounds)
        {
            if (poset.Preceq(current[j], b))
            {
                current[j] = b;
                break;
            }
        }
    }
}


void PosetDualization::FrequentInfrequentEnumerator::DecreaseFrequency( int threshold, PosetItems& current, std::vector<int>& support )
{
    int n = _task->GetPosetsProduct().GetPosets().size();

    for (int j = 0; j != n; ++j)
    {
        auto& poset = _task->GetPosetsProduct().GetPoset(j);
        auto& column = _task->GetDatabase().GetColumn(j);
        _enumerated1.clear();
        _enumerated2.clear();
        _enumerated3.clear();

        for (int i : support)
        {
            auto c = column[i];

			if (! poset.Preceq(current[j], c))
				continue;

			if (contains(_enumerated1, c))
                continue;

            _enumerated1.push_back(c);

            _forCover.assign(1, c);
            poset.GetMinimalIndependent(_forCover, _indep);
			            
            for (auto& a : _indep)
            {
                if ( contains(_enumerated2, a))
                    continue;

                _enumerated2.push_back(a);

				_forCover.assign(1, current[j]);
                _forCover.push_back(a);
                poset.GetMinUpperBounds(_forCover, _bounds);
				
				int t = 0;
                for (auto& b : _bounds)
                {
                    if ( contains(_enumerated3, b))
                        continue;

                    _enumerated3.push_back(b);
					
                    t = 0;
                    for (int k : support)
                    {
                        if (! poset.Preceq(b, column[k]))
                            continue;
                        ++t;
                        if (t == threshold)
                            break;
                    }

                    if (t >= threshold)
					{
						current[j] = b;
						break;
					}
                }

				if (t >= threshold)
					break;
            }
        }

		remove_if(support,
			[&poset, &current, &column, j](int r)
		{return ! poset.Preceq(current[j], column[r]);});

		if (support.size() == threshold)
			break;

    }

    
}

void PosetDualization::FrequentInfrequentEnumerator::IncreaseFrequency( int threshold, PosetItems& current)
{
    int n = _task->GetDatabase().GetWidth();
	int m = _task->GetDatabase().GetHeight();
        
    for (int j = 0; j != n; ++j)
    {
        auto& poset = _task->GetPosetsProduct().GetPoset(j);
        auto& column = _task->GetDatabase().GetColumn(j);

        _enumerated1.clear();
		_enumerated2.clear();
		bool changed = false;
        
        for (auto i = 0; i != m; ++i)
        {
            if (! _supportBitMap.get(i, j))
				continue;
			
			auto c = column[i];			
            
			if (changed && poset.Preceq(current[j], c))
				continue;

			if (contains(_enumerated1, c))
                continue;
            _enumerated1.push_back(c);
            
			_forCover.assign(1, current[j]);
			_forCover.push_back(c);

            poset.GetMaxLowerBounds(_forCover, _bounds);
            
            for (auto& b : _bounds)
            {
                if (contains(_enumerated2, b))
                    continue;
                _enumerated2.push_back(b);

				int t = 0;
				for (int k = 0; k != m; ++k)
				{
					auto& row = _supportBitMap.get_row(k);					
					if (row.none() || (row.get(j) && row.count()==1 && poset.Preceq(b, column[k])))
					{
						++t;
						if (t == threshold)
							break;	
					}
				}
                
				if (t < threshold)
				{
					current[j] = b;
					_supportBitMap.set(i,j,false);
					changed = true;

					if (t == threshold - 1)
						i = m;
					
					break;
				}                
            }
        
        }

        if (changed)
		{
			int t = UpdateSupportBitMap(j, current[j]);
			if (t == threshold - 1)
				return;
		}
        
    }

}



int PosetDualization::FrequentInfrequentEnumerator::BuildSupportBitMap(PosetItems const& items)
{
	int m = _task->GetDatabase().GetHeight();
	int n = _task->GetDatabase().GetWidth();

	_supportBitMap.zero(m, n);
	_supportMask.zero(n);

	for (int j = 0; j != n; ++j)
	{
		auto a = items[j];
		auto& poset = _task->GetPosetsProduct().GetPoset(j);
		if (poset.IsUniversalMinimal(a))
			continue;
		auto& column = _task->GetDatabase().GetColumn(j);

		for (int i = 0; i != m; ++i)
		{
			if (! poset.Preceq(a, column[i]))
				_supportBitMap.set(i, j);
		}
	}

	int t = 0;
	for(int i = 0; i != m; ++i)
	{
		int c = _supportBitMap.get_row(i).count();
		if (c == 0)
			++t;
		if (c == 1)
			_supportMask.assign_or(_supportMask, _supportBitMap.get_row(i));
	}
	return t;
}

void PosetDualization::FrequentInfrequentEnumerator::FindMinimalInfrequent(int threshold, PosetItems& current)
{
	if (_task->GetPosetsProduct().IsMinimal(current))
		return;
	
	int t = BuildSupportBitMap(current);

	if (t != threshold-1)
		IncreaseFrequency(threshold, current);

	SetUnsupportedToMinimal(current);

	SetSupportedToMinimalIndependent(current);	
}

int PosetDualization::FrequentInfrequentEnumerator::UpdateSupportBitMap(int j, PosetItem const& a)
{
	auto& column = _task->GetDatabase()[j];
	auto& poset = _task->GetPosetsProduct().GetPoset(j);
	int m = column.size();
	int n = _task->GetDatabase().GetWidth();
	int t = 0;
	_supportMask.zero(n);
	for (int i = 0; i != m; ++i)
	{
		if (_supportBitMap.get(i,j) && poset.Preceq(a, column[i]))
			_supportBitMap.set(i, j, false);
		int c = _supportBitMap.get_row(i).count();
		if (c==0)
			++t;
		else if (c==1)
			_supportMask.assign_or(_supportMask, _supportBitMap.get_row(i));
	}

	return t;
}

void PosetDualization::FrequentInfrequentEnumerator::SetUnsupportedToMinimal(PosetItems &current)
{
	int n = _task->GetDatabase().GetWidth();
	
	if (_supportMask.count() == n)
		return;

	for (int j = 0; j!=n; ++j)
	{
		if (_supportMask.get(j))
			continue;

		if (_task->GetPosetsProduct().GetPoset(j).IsMinimal(current[j]))
			continue;

		auto& poset = _task->GetPosetsProduct().GetPoset(j);
		poset.GetMinimal(_bounds);
		for (auto& b : _bounds)
		{
			if (poset.Preceq(b, current[j]))
			{
				current[j] = b;
				UpdateSupportBitMap(j, b);
				break;
			}
		}
	}
	
}

void PosetDualization::FrequentInfrequentEnumerator::SetSupportedToMinimalIndependent(PosetItems &current)
{
	int n = _task->GetDatabase().GetWidth();
	_nonsupportItems.resize(n);
	for (auto& u:_nonsupportItems)
		u.clear();
	int m = _task->GetDatabase().GetHeight();
	for (int i = 0; i != m; ++i)
	{
		auto& row = _supportBitMap.get_row(i);
		if (row.count()!=1)
			continue;

		int j = row.find_first();
		_nonsupportItems[j].push_back(_task->GetDatabase()[i][j]);
	}

	for (int j = 0; j !=n; ++j)
	{
		if (_nonsupportItems[j].empty())
			continue;

		auto& poset = _task->GetPosetsProduct().GetPoset(j);
		poset.SelectUnique(_nonsupportItems[j]);

		poset.GetMinimalIndependent(_nonsupportItems[j], _indep);

		for (auto& a : _indep)
		{
			if (poset.Preceq(a, current[j]))
			{
				current[j] = a;
				break;
			}
		}

	}
}
