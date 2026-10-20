#pragma once

#include <assert.h>
#include <CppUnitTestAssert.h>

#include "IPoset.h"
#include "string_utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


class PosetSelfTester
{

	// Tested poset instance
	IPosetPtr _poset;

	
public:
	
	PosetSelfTester(IPosetPtr poset);
	
	// Check method SelectFilter
	PosetItems SelectFilterSelfTest(PosetItems const& B, PosetItems const& D);

	// Check method SelectFilterIndependent
	PosetItems SelectFilterIndependentSelfTest(PosetItems const& B, PosetItems const& D);

	// Check method SelectIdeal
	PosetItems SelectIdealSelfTest(PosetItems const& B, PosetItems const& D);
	
	// Check method SelectIdealIndependent
	PosetItems SelectIdealIndependentSelfTest(PosetItems const& B, PosetItems const& D);

	// Check properties of relation a<=b
	void PreceqSelfTest(PosetItems const& items);

	// Check properties of relation a<b
	void PrecSelfTest(PosetItems const& items);

	// Check properties of relation a=b
	void EqualSelfTest(PosetItems const& items);

	// Check method IsMaximal(a, A)
	int IsMaximalFromSelfTest(PosetItems const& items);

	// Check method IsMinimal(a, A)
	int IsMinimalFromSelfTest(PosetItems const& items);

	// Check IsItem method
	void IsItemSelfTest(PosetItems const& items, PosetItems const& notItems);

	IPosetPtr GetPoset()
	{
		return _poset;
	}

	bool Equals(PosetItems const& A, PosetItems const& B);


	// Check method SelectMaximal
	PosetItems SelectMaximalSelfTest(PosetItems const& D);

	// Check method SelectMinimal
	PosetItems SelectMinimalSelfTest(PosetItems const& D);

	// Check method Select unique
	PosetItems SelectUniqueSelfTest(PosetItems const& A);

	// Check ToString method
	void ToStringConvertSelfTest(PosetItems const& items);

	// Check FromString method
	void FromStringConvertSelfTest(Strings const& strs);

	// Check GetMinimal method
	PosetItems GetMinimalSelfTest(PosetItems const& testItems);

	// Check GetMaximal method
	PosetItems GetMaximalSelfTest(PosetItems const& testItems);

    // Check topological sort of poset items
    PosetItems TopologicalSortSelfTest(PosetItems const& items);

    // Check PrecImmediate method on same items
    void PrecImmediateSelfTest(PosetItems const& items);

    void GetImmediatePrecSelfTest(PosetItems const& items);
    
    void GetImmediateSuccSelfTest(PosetItems const& items);

    void IsMaximalIndependentSelfTest(
        PosetItems const& A, 
        PosetItems const& candidates);

    void IsMinimalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates );

    PosetItems GetMaximalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates );

    PosetItems GetMinimalIndependentSelfTest( PosetItems const& A, PosetItems const& candidates );

    int GetIndependentIntervalsSelfTest(PosetItems const& A, PosetItems const& B, PosetItems const& candidates);

    PosetSelfTester& WriteLog(std::string const& message)
    {
        Logger::WriteMessage(message.c_str());
        return *this;
    }

    PosetSelfTester& WriteLog(PosetItems const& items)
    {
        WriteLog(ToString(_poset, items));
        return *this;
    }

private:

	template<typename Selector, typename Predicate>
	PosetItems DoSelectMethodSelfTest(
		PosetItems const& items, 
		Selector selector, Predicate predicate);

};

