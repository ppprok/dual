#pragma once

#include <array>
#include <vector>
#include "bit_chunk.h"

//////////////////////////////////////////////////////////////////////////
// Ѕинарна€ булева операци€, заданна€ таблицей
//////////////////////////////////////////////////////////////////////////
struct BoolBinaryOperation
{
	std::array<bool, 4> table;		
	
	BoolBinaryOperation(bool constant = false);

	template<typename T>
	BoolBinaryOperation(T func)
	{
		Set(true, true, func(true, true));
		Set(true, false, func(true, false));
		Set(false, true, func(false, true));
		Set(false, false, func(false, false));
	}

	void Set(bool o1, bool o2, bool r);

	int GetTableIndex(bool o1, bool o2) const;

	bool Apply(bool o1, bool o2) const;

	std::vector<BoolBinaryOperation> Split();

	BoolBinaryOperation& operator &=(BoolBinaryOperation const& o);

	BoolBinaryOperation& operator |=(BoolBinaryOperation const& o);

	BoolBinaryOperation& operator ^=(BoolBinaryOperation const& o);

	bool operator ==(BoolBinaryOperation& op) const;

    BoolBinaryOperation operator !() const;

    bit_chunk Apply(bit_chunk o1, bit_chunk o2, bit_chunk temp, bit_chunk& r, int n);
};

