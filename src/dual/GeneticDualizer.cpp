#include "GeneticDualizer.h"
#include "containers.h"
#include "BoolMatrixPreparation.h"
#include "IteratorRange.h"




// Нормировать последовательность генов, чтобы она лежала в пределах от minw до maxw
void GeneticDualizer::Normalize( Genes& weights, Gene minw, Gene maxw )
{
    Gene dw = 1 / (maxw - minw);
    
    for (int i = 0, m = weights.size(); i < m; ++i)
    {
        weights[i] = (weights[i] - minw) * dw;
    }
}

// Осуществить мутацию генов с указанным параметром разброса
void GeneticDualizer::Mutate( Genes& weights, Gene sigma /*= 1*/ )
{
    Gene minw = std::numeric_limits<Gene>::max();
    Gene maxw = -minw;

    for (int i = 0; i < (int)weights.size(); ++i)
    {
        auto w = weights[i] + RandomSampler<boost::uniform_01<Gene>>::RandomValue(-sigma,  2*sigma);
        if (minw > w)
            minw = w;
        if (maxw < w)
            maxw = w;

        weights[i] = w;
    }

    auto d = maxw - minw;
    if (d > 3 || d < 0.3)
        Normalize(weights, minw, maxw);
}


// Осуществить мутацию генов особи
void GeneticDualizer::Mutate( Individual& ind, Gene sigma /*= 1*/ )
{
    Mutate(ind.colsOrder, sigma);
    Mutate(ind.rowsOrder, sigma);
}

void GeneticDualizer::Mutate(int eliteCount, Gene sigma)
{
    for (int i = eliteCount; i < _populationSize; ++i)
    {
        Mutate(_population[i], sigma);
        UpdateIndividual(_population[i]);
    }    
}

// Выполнить скрещивание генов
void GeneticDualizer::Crossover( 
    Genes const& w1, 
    Genes const& w2, 
    Gene c1 /*= 1*/, 
    Gene c2 /*= 1*/, 
    Genes& w )
{
    int n1 = w1.size();
    int n2 = w2.size();
    
    assert(n1 == n2);

    Gene minw = std::numeric_limits<Gene>::max();
    Gene maxw = -minw;

    for (int i = 0; i < n1; ++i)
    {
        Gene w0 = 0;
        if (i < n1)
            w0 += c1 * w1[i];

        if (i < n2)
            w0 += c2 * w2[i];
        
        w[i] = w0;
        
        if (minw > w0)
            minw = w0;
        if (maxw < w0)
            maxw = w0;
    }

    if (maxw - minw > 3)
        Normalize(w, minw, maxw);

}

void GeneticDualizer::Crossover( 
    Individual const& i1, 
    Individual const& i2, 
    Gene c1 /*= 1*/, 
    Gene c2 /*= 1*/,
    Individual& child)
{
    Crossover(i1.colsOrder, i2.colsOrder, c1, c2, child.colsOrder);
    Crossover(i1.rowsOrder, i2.rowsOrder, c1, c2, child.rowsOrder);
    
    auto r = RandomSampler<boost::uniform_01<Gene, Gene> >::RandomValue(0.0f, c1 + c2);

    if (r < c1)
        child.fixed.assign(i1.fixed);
    else
        child.fixed.assign(i2.fixed);
}

void GeneticDualizer::Crossover( Gene scale /*= 1*/ )
{
    int n = CurrentOptions.eliteCount;
    if (n < 2)
        n = 2;
    if (n > _populationSize)
        n = _populationSize;

    int n0 = _populationSize;
    _population.reserve(n0 + n * (n-1) / 2);

    for (int i = 0; i < n - 1; ++i)
        for (int j = i+1; j < n; ++j)
        {
            auto& child = GetIndividual(_populationSize++);

            Crossover(_population[i], _population[j], 
                GetCrossoverCost((Gene)_population[i].cost, (Gene)_population[j].cost, scale),
                GetCrossoverCost((Gene)_population[j].cost, (Gene)_population[i].cost, scale), 
                child);

            UpdateIndividualCover(child);
        }

    _populationSize = RemoveDuplicateCovers(_population.begin() + n0, _population.begin() + _populationSize) + n0;
        
    for (int i = n0; i < _populationSize; ++i)
    {
        UpdateIndividualCost(_population[i]);
    }

    _populationSize = RemoveDuplicateCovers(_population.begin(), _population.begin() + _populationSize);
}


void GeneticDualizer::RandomIndividual(Individual&ind)
{
    RandomSampler<boost::uniform_01<Gene> >::RandomValues(
        ind.rowsOrder.begin(), ind.rowsOrder.end(), 0.0f, 1.0f);
    RandomSampler<boost::uniform_01<Gene> >::RandomValues(
        ind.colsOrder.begin(), ind.colsOrder.end(), 0.0f, 1.0f);
}

void GeneticDualizer::UpdateIndividual( Individual& ind )
{
    UpdateIndividualCover(ind);
    UpdateIndividualCost(ind);
}

void GeneticDualizer::UpdateIndividualCost( Individual& ind ) const
{
    if (_targetFunc)
    {
        ind.cost = _targetFunc->GetCoveredRowsWeight(ind.cover);
        return;
    }

    /*if (! _targetRows.empty())
    {
        ind.cost = 0;
        for (auto& r: _targetRows)
            if (ind.cover.intersects(r.bits))
                ind.cost += r.weight;
        return;
    }*/
    
    ind.cost = static_cast<Cost>(ind.cover.count());
}

void GeneticDualizer::UpdateIndividualCover( Individual& ind )
{
    int m = _uncoveredRows.size();
    _orderedRows.resize(m);

    for (int i = 0; i < m; ++i)
    {
        _orderedRows[i].bits = _uncoveredRows[i].bits;
        _orderedRows[i].weight = ind.GetRowWeight(i, m);
    }
    
    sort(_orderedRows);

    ind.cover.assign(ind.fixed);

    for (int t = 0; t < m; ++t)
    {
        auto row = _orderedRows[t].bits;
        if (row.intersects(ind.cover))
            continue;

        int j = FindMinWeightColumn(row, ind);

        if (j < 0)
        {
            ind.cover.clear();
            return;
        }

        ind.cover.set(j);        
    }
}

void GeneticDualizer::Select( int count )
{
    if (count <= 0)
    {
        _populationSize = 0;
        return;
    }

    if (count > _populationSize)
        count = _populationSize;

    SelectElite(count);    
    _populationSize = count;
}

Individuals const& GeneticDualizer::GetPopulation() const
{
    return _population;
}

/*
void GeneticDualizer::SetRows(int n, bit_chunks rows)
{
    assert(n >= 0);
    _n = n;
    _rows = std::move(rows);   
    int m = _rows.size();

    _rowsOrder.resize(m);
    
    _populationSize = 0;

    if (! _population.empty() && 
        (_population.front().rowsOrder.size() != m ||
        _population.front().colsOrder.size() != n))
    {
        _population.clear();
    }

}*/

GeneticDualizer::GeneticDualizer() 
    :_n(0), _targetFunc(0), _populationSize(0), _rowsSource(0)
{       
}

// Удалить особей, относящихся к одному и тому же покрытию
int GeneticDualizer::RemoveDuplicateCovers( 
    Individuals::iterator begin, 
    Individuals::iterator end)
{
    if (begin == end)
        return 0;
    
    std::sort(begin, end,
        [](Individual const& i1, Individual const& i2)
    {
        return i1.cover < i2.cover;
    });

    for (auto i = begin, j = begin+1; j != end; ++i, ++j)
    {
        if (i->cover == j->cover)
            i->cover.clear();
    }

    return 
        std::partition(begin, end,
                [](Individual const& ind) 
                    { 
                        return ! ind.cover.empty(); 
                    }) - begin;
}

void GeneticDualizer::SelectElite()
{
    SelectElite(CurrentOptions.eliteCount);
}

void GeneticDualizer::SelectElite( int count )
{
    if (count <= 0)
        return;

    std::random_shuffle(_population.begin(), _population.begin() + _populationSize);
    
    if  (count > _populationSize)
        count = _populationSize;
        
    std::partial_sort(
        _population.begin(), 
        _population.begin() + count, 
        _population.begin() + _populationSize,
        [](Individual const& i1, Individual const& i2)
    {
        return i1.cost < i2.cost;
    });
}


void GeneticDualizer::MakePopulation(int t)
{
    //LoadUncoveredRows();
    GrowPopulation(CurrentOptions.populationSize);
    SelectElite();
    Mutate(CurrentOptions.eliteCount, GetMutationScale(t));
    Crossover(GetCrossoverScale(t));
    Select(CurrentOptions.populationSize);
}

void GeneticDualizer::Reset()
{
    _population.clear();
    _populationSize = 0;
    _uncoveredRows.clear();
    _orderedRows.clear();
    _commonFixed.clear();
    _n = 0;

    if (_targetFunc)
        _targetFunc->Reset();
    if (_rowsSource)
        _rowsSource->Reset();
}

void GeneticDualizer::MakePopulations(int from, int to)
{
    for (int t = from; t < to; ++t)
        MakePopulation(t);
}


int GeneticDualizer::FindMinWeightColumn(bit_chunk row, Individual const& ind)
{
    int j = row.find_first();
    if (j < 0)
        return j;

    Gene minw = ind.GetColWeight(j, _n);
    
    for (int k = row.find_next(j); k >=0; k = row.find_next(k))
    {
        auto w = ind.GetColWeight(k, _n);
        if (minw > w)
        {
            j = k;
            minw = w;
        }
    }

    return j;
}

Gene GeneticDualizer::GetCrossoverCost( Gene c1, Gene c2, Gene scale /*= 1*/ )
{
    return 1 / (1 + exp(scale*(c1 - c2)));
}

Individual& GeneticDualizer::GetIndividual( int i )
{
    int gm = _uncoveredRows.size();
    int gn = _n;

    if (CurrentOptions.genesLimit > 0)
    {
        if (CurrentOptions.genesLimit < gm)
            gm = CurrentOptions.genesLimit;
        if (CurrentOptions.genesLimit < gn)
            gn = CurrentOptions.genesLimit;
    }
    
    for (int d = i - (int)_population.size(); d >=0; --d)
    {
        _population.emplace_back(gm, gn, _n);
    }
    
    return _population[i];
}

void GeneticDualizer::GrowPopulation(int count)
{
    _population.reserve(count);

    if (_populationSize <= 0)
    {
        for (; _populationSize < count; ++_populationSize)
        {
            auto& ind = GetIndividual(_populationSize);
            ind.fixed.assign(_commonFixed);
            RandomIndividual(ind);
            UpdateIndividual(ind);
        }

        return;
    }

    //int ccount = count / _populationSize;
    //while (_populationSize * ccount < count)
      //  ++ccount;

    for (int k = 0; k < _populationSize && _populationSize < count; ++k)
    {
        auto& ind = GetIndividual(_populationSize);
        ind.fixed.assign(_population[k].fixed);
        RandomIndividual(ind);
        UpdateIndividual(ind);
    }
}

Gene GeneticDualizer::GetCrossoverScale(int t) const
{
    return CurrentOptions.crossoverScale0 * exp(-CurrentOptions.crossoverScaleScale * t);
}

Gene GeneticDualizer::GetMutationScale(int t) const
{
    return CurrentOptions.mutationScale0 * exp(-CurrentOptions.mutationScaleScale * t);
}


void GeneticDualizer::SetOptions( Options const& options )
{
    GAOptions def;    
    CurrentOptions.coverRowsLimit =      options.get<int>("coverRowsLimit", def.coverRowsLimit);
    CurrentOptions.targetRowsLimit =     options.get<int>("targetRowsLimit", def.targetRowsLimit);
    CurrentOptions.genesLimit =          options.get<int>("genesLimit", def.genesLimit);
    CurrentOptions.iterationCount =      options.get<int>("iterationCount", def.iterationCount);
    CurrentOptions.populationSize =      options.get<int>("populationSize", def.populationSize);
    CurrentOptions.eliteCount =          options.get<int>("eliteCount", def.eliteCount);
    CurrentOptions.crossoverScale0 =     options.get<Gene>("crossoverScale0", def.crossoverScale0);
    CurrentOptions.crossoverScaleScale = options.get<Gene>("crossoverScaleScale", def.crossoverScaleScale);
    CurrentOptions.mutationScale0 =      options.get<Gene>("mutationScale0", def.mutationScale0);
    CurrentOptions.mutationScaleScale =  options.get<Gene>("mutationScaleScale", def.mutationScaleScale);
}

void GeneticDualizer::SetDefaultOptions()
{
    CurrentOptions = GAOptions();
}

void GeneticDualizer::SetTargetFunc( ITargetFunc* target )
{
    _targetFunc = target;
}

void GeneticDualizer::SetRowsSource( IRowsSource* rowsSource )
{
    _rowsSource = rowsSource;
    _uncoveredRows.clear();
    //_targetRows.clear();
    
    if (_rowsSource)
    {
        _n = _rowsSource->GetWidth();
        _commonFixed = bit_vector(_n);
        //_rowsSource->Reset();
        _uncoveredRows.reserve(CurrentOptions.coverRowsLimit);
        _rowsSource->LoadUncoveredRows(_commonFixed, CurrentOptions.coverRowsLimit, _uncoveredRows);
    }
    else
    {
        _n = 0;
        _commonFixed = bit_vector(0);
    }
}

int GeneticDualizer::GetPopulationSize() const
{
    return _populationSize;
}

/*void GeneticDualizer::LoadUncoveredRows()
{
    if (! _rowsSource)
        return;
    
    int m0 = _uncoveredRows.size();
    for (int m = m0; m < CurrentOptions.coverRowsLimit; )
    {
        auto row = _rowsSource->GetNextUncoverRow();

        if (row.bits.empty())
            break;

        if (_commonFixed.intersects(row.bits))
        {
            //_rowsSource->CoverUncoveredRowCallback(row);
            continue;
        }

        int j = 0;
        while (j < m)
        {
            int r = bit_chunk::subset_cmp(_uncoveredRows[j].bits, row.bits);
            if (r < 0)
            {
                //_rowsSource->CoverUncoveredRowCallback(row);
                break;
            }
            
            if (r > 0)
            {
                //_rowsSource->CoverUncoveredRowCallback(_uncoveredRows[j]);
                _uncoveredRows[j] = _uncoveredRows.back();
                _uncoveredRows.pop_back();
                --m;
            }
            else
            {
                ++j;
            }
        }

        if (j == m)
        {
            _uncoveredRows.push_back(row);
            ++m;
        }
    }

    _orderedRows.resize(_uncoveredRows.size());

    /*RemoveDependentTargetRows(
        WeightedRowsRange(_uncoveredRows.begin() + m0, _uncoveredRows.end()));    */

    //if (_uncoveredRows.empty())
        //return;

    /*for (int i = 0; i < _populationSize; ++i)
        UpdateIndividualCover(_population[i]);

    _populationSize = RemoveDuplicateCovers(_population.begin(), _population.begin() + _populationSize);
}*/

/*
void GeneticDualizer::LoadTargetRows()
{
    if (! _rowsSource)
        return;

    int m = _uncoveredRows.size();

    for (int i = _targetRows.size(); i < CurrentOptions.targetRowsLimit; )
    {
        auto wr = _rowsSource->GetNextTargetRow();
        auto row = wr.bits;

        if (row.empty())
            break;

        if (_commonFixed.intersects(row))
        {
            _rowsSource->CoverTargetRowCallback(row);
            continue;
        }
        
        if (IsTargetRowDependentOfUncovered(wr, _uncoveredRows))
        {
            _rowsSource->CoverTargetRowCallback(row);
            continue;
        }

        int j = 0;
        for (; j < i; ++j)
        {
            if (_targetRows[j].bits == row)
            {
                _targetRows[j].weight += wr.weight;
                _rowsSource->CoverTargetRowCallback(row);
                break;
            }
        }
        
        if (j == i)
        {
            _targetRows.push_back(wr);
            ++i;
        }
    }

    /*for (int i = 0; i < _populationSize; ++i)
        UpdateIndividualCost(_population[i]);
}*/

void GeneticDualizer::FixCovers()
{
    if (_populationSize > 0)
        _commonFixed.assign(_population[0].cover);
    else
        _commonFixed.clear();
    
    for (int i = 0; i < _populationSize; ++i)
    {
        if (! _commonFixed.empty())
            _commonFixed.assign_and(_commonFixed, _population[i].cover);

        _population[i].fixed.assign(_population[i].cover);
    }

    //RemoveFixedCoveredTargetRows();
    
/*    for (auto& ur:_uncoveredRows)
        _rowsSource->CoverUncoveredRowCallback(ur);*/
    
    
    

}

/*bool GeneticDualizer::IsTargetRowDependentOfUncovered( 
    WeightedRow const& tr, 
    WeightedRowsRange const& uncoveredRows ) const
{
    for (auto& ur: uncoveredRows)
    {
        if (bit_chunk::subset_cmp(ur.bits, tr.bits) < 0)
            return true;
    }

    return false;
}

void GeneticDualizer::RemoveDependentTargetRows(
    WeightedRowsRange const& uncoveredRows)
{
    if (uncoveredRows.empty())
        return;
    
    auto end = _targetRows.end();
    auto mid = std::partition(_targetRows.begin(), end,
        [this, &uncoveredRows](WeightedRow const& wr)
    {
        return ! IsTargetRowDependentOfUncovered(wr, uncoveredRows);
    });

    if (mid == end)
        return;

    if (_rowsSource != 0)
    {
        for (auto i = mid ;i != end; ++i)
            _rowsSource->CoverTargetRowCallback(*i);
    }
    
    _targetRows.erase(mid, end);
}

void GeneticDualizer::RemoveFixedCoveredTargetRows()
{
    if (_commonFixed.empty())
        return;
    
    auto end = _targetRows.end();
    auto mid = std::partition(_targetRows.begin(), end,
        [this](WeightedRow const& wr)
    {
        return ! _commonFixed.intersects(wr.bits);
    });

    if (mid == end)
        return;

    if (_rowsSource != 0)
    {
        for (auto i = mid ;i != end; ++i)
            _rowsSource->CoverTargetRowCallback(*i);
    }

    _targetRows.erase(mid, end);
}*/

void GeneticDualizer::RandomizeGenesAfterFixCovers()
{
    
    for (int i = 0; i < _populationSize; ++i)
    {
        RandomIndividual(_population[i]);
        UpdateIndividualCover(_population[i]);
    }

    _populationSize = RemoveDuplicateCovers(_population.begin(), _population.begin() + _populationSize);

    for (int i = 0; i < _populationSize; ++i)
        UpdateIndividualCost(_population[i]);
}

/*void GeneticDualizer::FinallyLoadTargetRows()
{
    if (! _rowsSource)
        return;

    if (_targetFunc)
        return;
    
    bool c = false;

    for (;;)
    {
        auto tr = _rowsSource->GetNextTargetRow();
        if (tr.bits.empty())
            break;

        if (! tr.weight)
            continue;

        for (int i = 0; i < _populationSize; ++i)
        {
            if (_population[i].cover.intersects(tr.bits))
            {
                _population[i].cost += tr.weight;
                c = true;
            }
        }
    }

    if (c)
        SelectElite();
}*/

void GeneticDualizer::Search()
{
    for(;;)
    {
        MakePopulations(0, CurrentOptions.iterationCount);
        FixCovers();

        _uncoveredRows.clear();

        if (_rowsSource)
            _rowsSource->LoadUncoveredRows(_commonFixed, CurrentOptions.coverRowsLimit, _uncoveredRows);

        if (_uncoveredRows.empty())
            break;
        
        RandomizeGenesAfterFixCovers();
        GrowPopulation(CurrentOptions.populationSize * 2);
        Select(CurrentOptions.populationSize);
    }

    //FinallyLoadTargetRows();
}
/*
Cost CoveredRowsWeightTargetFunc::GetCost( bit_chunk cover ) const
{
    Weight s = _w0;
    
    for (int i = 0, m = _rows.size(); i < m; ++i)
    {
        if (cover.intersects(_rows[i]))
            s += _rowsWeights[i];
    }

    return s;
}


void CoveredRowsWeightTargetFunc::ClearZeroRows()
{
    int c = 0;
    
    for (int i = 0, m = _rows.size(); i < m; ++i)
    {
        if (_rowsWeights[i] == 0)
        {
            _rows[i].clear();
            ++c;
        }
        else if (_rows[i].empty())
        {
            _rowsWeights[i] = 0;
            ++c;
        }
    }

    if (c == 0)
        return;

    _rowsWeights.erase(remove(_rowsWeights.begin(), _rowsWeights.end(), 0), _rowsWeights.end());
    _rows.erase(remove_if(_rows.begin(), _rows.end(), 
        [](bit_chunk r)   { return r.empty(); }), _rows.end());

}

void CoveredRowsWeightTargetFunc::RemoveDuplicateRows()
{
    for (int i = 0, m = _rows.size(); i < m-1; ++i)
    {
        if (_rows[i].empty())
            continue;

        for (int j = i+1; j < m; ++j)
        {
            if (_rows[j].empty())
                continue;
            
            if (_rows[i] == _rows[j])
            {
                _rowsWeights[i] += _rowsWeights[j];
                _rows[j].clear();             
            }
        }
    }

    ClearZeroRows();
}

void CoveredRowsWeightTargetFunc::RemoveDependentRows(bit_chunks const& masterRows)
{
    int c = 0;
    
    for (auto r1 : masterRows)
    {
        for (int i = 0, m = _rows.size(); i < m; ++i)
        {
            if (_rows[i].empty())
                continue;

            if (bit_chunk::subset_cmp(r1, _rows[i]) == -1)
            {
                _rows[i].clear();
                _w0 += _rowsWeights[i];
                ++c;
            }            
        }

        if (c > 10)
        {
            ClearZeroRows();
            c = 0;
        }
    }

    if (c > 0)
        ClearZeroRows();
}
*/

Gene Individual::GetWeight( int i, int m, Genes const& genes ) const
{
    int m0 = genes.size();
    if (m0 == 0)
        return 0;

    if (m0 == 1 || i <= 0)
        return genes.front();

    if (i >= m-1)
        return genes.back();

    if (m <= m0)
        return genes[i];

    Gene p = (Gene)i * m0 / m;        
    // Определение номеров ближайших генов
    int p0 = (int)floor(p);
    int p1 = p0 + 1;
    if (p1 >= m0)
    {
        p1 = m0 - 1;
        p0 = m0 - 2;
    }

    Gene y0 = genes[p0];
    Gene y1 = genes[p1];
    // Линейная интерполяция i-го веса
    return (p - p0)*(y1-y0) + y0;
}