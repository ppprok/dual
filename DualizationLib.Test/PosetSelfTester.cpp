#include "stdafx.h"
#include "PosetSelfTester.h"



void PosetSelfTester::EqualSelfTest(PosetItems const& items)
{
	for (auto& a : items)
	{
		// antireflection
		Assert::IsTrue(_poset->Equal(a, a));

		for (auto& b : items)
		{
			// antisimmetry
			if (_poset->Equal(a, b))
			{
				Assert::IsFalse(_poset->Prec(a, b));
				Assert::IsTrue(_poset->Preceq(a, b));
				Assert::IsTrue(_poset->Equal(b, a));
				Assert::IsFalse(_poset->Prec(b, a));
				Assert::IsTrue(_poset->Preceq(b, a));
			}

			for (auto& c : items)
			{
				// transitive

				if (_poset->Equal(a, b) && _poset->Equal(b, c))
					Assert::IsTrue(_poset->Equal(a, c));

			}
		}
	}
}

void PosetSelfTester::PrecSelfTest(PosetItems const& items)
{
	for (auto& a : items)
	{
		// antireflection
		Assert::IsTrue(! _poset->Prec(a, a));

		for (auto& b : items)
		{
			// antisimmetry
			if (_poset->Prec(a, b))
			{
				Assert::IsFalse(_poset->Equal(a, b));
				Assert::IsTrue(_poset->Preceq(a, b));

				Assert::IsFalse(_poset->Equal(b, a));
				Assert::IsFalse(_poset->Prec(b, a));
				Assert::IsFalse(_poset->Preceq(b, a));
			}

			if (_poset->Preceq(a, b))
			{
				Assert::IsFalse(_poset->Prec(b, a));
			}

			if (_poset->Equal(a, b))
			{
				Assert::IsFalse(_poset->Prec(a, b));
				Assert::IsFalse(_poset->Prec(b, a));
			}

			for (auto& c : items)
			{
				// transitive

				if (_poset->Prec(a, b) && _poset->Prec(b, c))
					Assert::IsTrue(_poset->Prec(a, c));

			}
		}
	}
}

void PosetSelfTester::PreceqSelfTest(PosetItems const& items)
{
	for (auto& a : items)
	{
		// reflection
		Assert::IsTrue(_poset->Preceq(a, a));

		for (auto& b : items)
		{
			// antisimmetry
			if (_poset->Preceq(a, b) && _poset->Preceq(b, a))
				Assert::IsTrue(_poset->Equal(a, b));

			for (auto& c : items)
			{
				// transitive

				if (_poset->Preceq(a, b) && _poset->Preceq(b, c))
					Assert::IsTrue(_poset->Preceq(a, c));

			}
		}
	}
}

PosetItems PosetSelfTester::SelectFilterSelfTest(PosetItems const& B, PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[&B](IPosetPtr poset, PosetItems& selected)
		{poset->SelectFilter(B, selected);},
		[&B](IPosetPtr poset, PosetItem a)
		{return poset->InFilter(a, B);}
		);
		
}

PosetSelfTester::PosetSelfTester(IPosetPtr poset) :_poset(poset)
{
	assert(poset);
}

int PosetSelfTester::IsMaximalFromSelfTest(PosetItems const& items)
{
		
	int count = 0;
	
	for (auto& a : items)
	{
		if (_poset->IsMaximal(a, items))
		{
			++count;

			for (auto& b : items)
			{
				Assert::IsFalse(_poset->Prec(a, b));
			}
		}
	}

	if (! items.empty())
		Assert::IsTrue(count > 0);

	return count;
}

int PosetSelfTester::IsMinimalFromSelfTest(PosetItems const& items)
{
	int count = 0;

	for (auto& a : items)
	{
		if (_poset->IsMinimal(a, items))
		{
			++count;

			for (auto& b : items)
			{
				Assert::IsFalse(_poset->Prec(b, a));
			}
		}
	}

	if (! items.empty())
		Assert::IsTrue(count > 0);

	return count;
}

void PosetSelfTester::IsItemSelfTest(PosetItems const& items, PosetItems const& notItems)
{
	for (auto& a: items)
	{
		Assert::IsTrue(_poset->IsItem(a));
	}

	for (auto& b: notItems)
	{
		Assert::IsFalse(_poset->IsItem(b));
	}
}

PosetItems PosetSelfTester::SelectIdealSelfTest(PosetItems const& B, PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[&B](IPosetPtr poset, PosetItems& selected)
		{poset->SelectIdeal(B, selected);},
		[&B](IPosetPtr poset, PosetItem a)
		{return poset->InIdeal(a, B);});
}

PosetItems PosetSelfTester::SelectFilterIndependentSelfTest(PosetItems const& B, PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[&B](IPosetPtr poset, PosetItems& selected)
	{poset->SelectFilterIndependent(B, selected);},
	[&B](IPosetPtr poset, PosetItem a)
	{return ! poset->InFilter(a, B);});
}

PosetItems PosetSelfTester::SelectIdealIndependentSelfTest(PosetItems const& B, PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[&B](IPosetPtr poset, PosetItems& selected)
	{poset->SelectIdealIndependent(B, selected);},
	[&B](IPosetPtr poset, PosetItem a)
	{return ! poset->InIdeal(a, B);});
}

bool PosetSelfTester::Equals(PosetItems const& A, PosetItems const& B)
{
	if (A.size() != B.size())
		return false;

	for (int i = 0, n = A.size(); i != n; ++i)
	{
		if (! _poset->Equal(A[i], B[i]))
			return false;
	}

	return true;
}

template<typename Selector, typename Predicate>
PosetItems PosetSelfTester::DoSelectMethodSelfTest(PosetItems const& items, Selector selector, Predicate predicate)
{
	PosetItems selected = items;
	selector(_poset, selected);

	for (auto& a:selected)
		Assert::IsTrue(predicate(_poset, a));

	for (auto& b:items)
		if (! contains(selected, b))
			Assert::IsFalse(predicate(_poset, b));

	return selected;
}

PosetItems PosetSelfTester::SelectMaximalSelfTest(PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[](IPosetPtr poset, PosetItems& selected)
	{poset->SelectMaximal(selected);},
	[&D](IPosetPtr poset, PosetItem a)
	{return poset->IsMaximal(a, D);});
}

PosetItems PosetSelfTester::SelectMinimalSelfTest(PosetItems const& D)
{
	return DoSelectMethodSelfTest(D, 
		[](IPosetPtr poset, PosetItems& selected)
	{poset->SelectMinimal(selected);},
	[&D](IPosetPtr poset, PosetItem a)
	{return poset->IsMinimal(a, D);});
}

PosetItems PosetSelfTester::SelectUniqueSelfTest(PosetItems const& A)
{
	auto selected = A;
	_poset->SelectUnique(selected);

	for(auto& a : selected)
	{
        Assert::IsTrue(find_if(A, 
            [this, a](PosetItem b){return _poset->Equal(a, b);})!=-1);
		for (auto& b : selected)
		{
			if (&a != &b)
				Assert::IsFalse(_poset->Equal(a, b));
		}
	}

	for (auto& a : A)
	{
        Assert::IsTrue(find_if(selected, 
            [this, a](PosetItem b){return _poset->Equal(a, b);})!=-1);
	}

	return selected;
}

void PosetSelfTester::ToStringConvertSelfTest(PosetItems const& items)
{
	for (auto& a: items)
	{
		auto str = _poset->ToString(a);
		auto b = _poset->FromString(str.c_str(), str.c_str() + str.length());
		Assert::IsTrue(_poset->Equal(a, b));
	}
}

void PosetSelfTester::FromStringConvertSelfTest(Strings const& strs)
{
	for (auto& str: strs)
	{
		auto b = _poset->FromString(str.c_str(), str.c_str() + str.length());
		auto str1 = _poset->ToString(b);
		Assert::AreEqual(str, str1);
	}
}

PosetItems PosetSelfTester::GetMinimalSelfTest(PosetItems const& testItems)
{
	PosetItems minimal;
	_poset->GetMinimal(minimal);

	Assert::IsFalse(minimal.empty());
	
	for (auto& m : minimal)
	{
		Assert::IsTrue(_poset->IsItem(m));
		Assert::IsTrue(_poset->IsMinimal(m));
		for (auto& a : testItems)
            if (_poset->IsItem(a))
                Assert::IsFalse(_poset->Prec(a, m));
	}

    return minimal;
}

PosetItems PosetSelfTester::GetMaximalSelfTest(PosetItems const& testItems)
{
    PosetItems maximal;
    _poset->GetMaximal(maximal);

    Assert::IsFalse(maximal.empty());

    for (auto& m : maximal)
    {
        Assert::IsTrue(_poset->IsItem(m));
        Assert::IsTrue(_poset->IsMaximal(m));
        for (auto& a : testItems)
            if (_poset->IsItem(a))
                Assert::IsFalse(_poset->Prec(m, a));
    }

    return maximal;
}

PosetItems PosetSelfTester::TopologicalSortSelfTest( PosetItems const& items )
{
    PosetItems A = items;
    _poset->TopologicalSort(A);
    
    for (auto& a: A)
        Assert::IsTrue(contains(items, a));

    for (auto& a: items)
        Assert::IsTrue(contains(A, a));

    Assert::AreEqual(A.size(), items.size());
    Assert::IsTrue(_poset->IsTopologicalOrdered(A));

    for (int i = 0, n = A.size(); i != n; ++i )
        for (int j = 0; j != n; ++j)
            if (_poset->Prec(A[i], A[j]))
                Assert::IsTrue(i > j);

    return A;
}

void PosetSelfTester::PrecImmediateSelfTest( PosetItems const& items )
{
    for (auto& a: items)
        for (auto& b: items)
        {
            if (_poset->PrecImmediate(a, b))
            {
                for (auto& c : items)
                    Assert::IsFalse(_poset->Prec(a, c) && _poset->Prec(c, b));                    
            }
            else if (_poset->Prec(a, b))
            {
                PosetItems successors;
                _poset->GetImmediateSucc(a, successors);
                
                int r = 0;
                for (auto& c : successors)
                {
                    if(_poset->Prec(a, c) && _poset->Prec(c, b))
                        ++r;
                }
                Assert::IsTrue(r>0);
            }


        }
}

void PosetSelfTester::GetImmediatePrecSelfTest( PosetItems const& items )
{
    for (auto& a : items)
    {
        PosetItems precessors;
        _poset->GetImmediatePrec(a, precessors);
        if (precessors.empty())
        {
            Assert::IsTrue(! _poset->IsItem(a) || _poset->IsMinimal(a));
        }
        else
        {
            for (auto& c : precessors)
            {
                auto r1 = contains(precessors, c);
                auto r2 = _poset->PrecImmediate(c, a);
                Assert::AreEqual(r1, r2);
            }
        }
    }
}

void PosetSelfTester::GetImmediateSuccSelfTest( PosetItems const& items )
{
    for (auto& a : items)
    {
        PosetItems successors;
        _poset->GetImmediateSucc(a, successors);
        if (successors.empty())
        {
            Assert::IsTrue(! _poset->IsItem(a) || _poset->IsMaximal(a));
        }
        else
        {
            for (auto& c : successors)
                Assert::AreEqual(contains(successors, c), _poset->PrecImmediate(a, c));
        }
    }
}

void PosetSelfTester::IsMaximalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates )
{
    for (auto& c:A)
    {
        Assert::AreEqual(
            _poset->IsMaximalIndependent(c, PosetItems()),
            _poset->IsMaximal(c));
    }

    for (auto& a:A)
    {
        Assert::IsFalse(_poset->IsMaximalIndependent(a, A));
    }

    for (auto& a : candidates)
    {
        if (_poset->IsMaximalIndependent(a, A))
        {
            Assert::IsFalse(_poset->InIdeal(a, A));

            for (auto& c : candidates)
            {
                if (_poset->Prec(a, c) && _poset->IsItem(c))
                    Assert::IsTrue(_poset->InIdeal(c, A));
            }

            PosetItems successors;
            _poset->GetImmediateSucc(a, successors);
            for (auto& c : successors)
            {
                 Assert::IsTrue(_poset->InIdeal(c, A));
            }
        }
        else
        {
            if (! _poset->InIdeal(a, A))
            {
                PosetItems successors;
                _poset->GetImmediateSucc(a, successors);
                bool r = false;
                for (auto& c : successors)
                    r |= ! _poset->InIdeal(c, A);                    
                Assert::IsTrue(r);
            }
        }
    }
}

void PosetSelfTester::IsMinimalIndependentSelfTest(
    PosetItems const& A, PosetItems const& candidates )
{
    for (auto& c:A)
    {
        Assert::AreEqual(
            _poset->IsMinimalIndependent(c, PosetItems()),
            _poset->IsMinimal(c));
    }

    for (auto& a:A)
    {
        Assert::IsFalse(_poset->IsMinimalIndependent(a, A));
    }

    for (auto& a : candidates)
    {
        if (_poset->IsMinimalIndependent(a, A))
        {
            Assert::IsFalse(_poset->InFilter(a, A));

            for (auto& c : candidates)
            {
                if (_poset->Prec(c, a) && _poset->IsItem(c))
                    Assert::IsTrue(_poset->InFilter(c, A));
            }

            PosetItems precessors;
            _poset->GetImmediatePrec(a, precessors);
            for (auto& c : precessors)
            {
                Assert::IsTrue(_poset->InFilter(c, A));
            }
        }
        else
        {
            if (! _poset->InFilter(a, A))
            {
                PosetItems precessors;
                _poset->GetImmediatePrec(a, precessors);
                bool r = false;
                for (auto& c : precessors)
                    r |= ! _poset->InFilter(c, A);                    
                Assert::IsTrue(r);
            }
        }
    }
}

PosetItems PosetSelfTester::GetMaximalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates )
{
    PosetItems indep;
    _poset->GetMaximalIndependent(A, indep);

    for (auto& a: indep)
        Assert::IsTrue(_poset->IsMaximalIndependent(a, A));

    PosetItems indep1;
    _poset->SelectIdealIndependent(A, indep1);
    _poset->SelectMaximal(indep1);

    for (auto& c: indep1)
    {
        if (_poset->IsMaximalIndependent(c, A))
            Assert::IsTrue(contains(indep, c));
    }

    return indep;
}

PosetItems PosetSelfTester::GetMinimalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates )
{
    PosetItems indep;
    _poset->GetMinimalIndependent(A, indep);

    
    PosetItems indep1 = candidates;
    _poset->SelectFilterIndependent(A, indep1);
    _poset->SelectMinimal(indep1);

    for (auto& a: indep)
    {
        auto r = _poset->IsMinimalIndependent(a, A);
        Assert::IsTrue(r);
    }

    for (auto& c: indep1)
    {
        if (_poset->IsMinimalIndependent(c, A))
            Assert::IsTrue(contains(indep, c));
    }

    return indep;
}

int PosetSelfTester::GetIndependentIntervalsSelfTest( 
    PosetItems const& A, PosetItems const& B, PosetItems const& candidates )
{
    PosetIntervals indep;
    _poset->GetIndependentIntervals(A, B, indep);

    for (auto& i :indep)
    {
        Assert::IsTrue(_poset->Preceq(i.lower, i.upper));
        Assert::IsTrue(_poset->IsMaximalIndependent(i.upper, A));
        Assert::IsTrue(_poset->IsMinimalIndependent(i.lower, B));
    }

    PosetItems lower = candidates;
    PosetItems upper = candidates;

    _poset->SelectIdealIndependent(A, upper);
    _poset->SelectFilterIndependent(B, lower);
    _poset->SelectMaximal(upper);
    _poset->SelectMinimal(lower);

    for (auto& l:lower)
    {
        for(auto& u : upper)
        {
            if (_poset->Preceq(l, u) &&
                _poset->IsMinimalIndependent(l, B) &&
                _poset->IsMaximalIndependent(u, A))
            {
                Assert::IsTrue(contains(indep, PosetInterval(l, u)));
            }
        }
    }

    return indep.size();
}