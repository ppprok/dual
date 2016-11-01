#pragma once
#include "WinHeap.h"
#include "bit_chunk.h"


//////////////////////////////////////////////////////////////////////////
// ������� ������ � ���������� ������ ��� �����
//////////////////////////////////////////////////////////////////////////
class bit_vector
    :public bit_chunk, private boost::noncopyable
{
private:
    
    int _capacity;

    inline void bit_vector::free()
    {
        if (_capacity > 0 && begin)
            WinHeap::Instance().Free(begin);
        
        _capacity = 0;        
        begin = end = 0;
    }

public:

    
    /*Block* begin()
    {
        return bit_chunk::begin;
    }
    
    Block const* begin() const
    {
        return bit_chunk::begin;
    }

    Block* end()
    {
        return bit_chunk::end;
    }

    Block const* end() const
    {
        return bit_chunk::end;
    }*/

    inline bit_vector& bit_vector::operator=( bit_vector&& v )
    {
        free();

        bit_chunk::begin = v.begin;
        bit_chunk::end = v.end;
        _capacity = v._capacity;

        v._capacity = 0;
        v.free();

        return *this;
    }

    inline bit_vector::bit_vector( bit_vector&& v ) :_capacity(0)
    {
        bit_chunk::begin = v.begin;
        bit_chunk::end = v.end;
        _capacity = v._capacity;

        v._capacity = 0;
        v.free();
    }

    inline bit_vector::bit_vector( int n )
    {
        if (n > 0)
        {
            _capacity = block_for_bits(n);
            bit_chunk::begin = bit_chunk::end = WinHeap::Instance().Alloc<Block>(_capacity);
        }
        else
        {
            _capacity = 0;
        }
    }

    inline bit_vector::bit_vector() :_capacity(0)
    {
    }

    /*inline bit_vector::bit_vector( bit_chunk bits ) :bit_chunk(bits), _capacity(0)
    {
    }*/
    
    inline bit_vector::~bit_vector()
    {
        free();
    }

    // �������� ������ �����
    void zero(int n)
    {
        if ( n > 0)
        {
            n = block_for_bits(n);
            if (_capacity < n)
            {
                if (_capacity > 0)
                    bit_chunk::begin = bit_chunk::end = WinHeap::Instance().ReAlloc<Block>(begin, n);
                else
                    bit_chunk::begin = bit_chunk::end = WinHeap::Instance().Alloc<Block>(n);
                _capacity = n;
            }
            else
            {
                bit_chunk::end = bit_chunk::begin;
            }
        }
        else
        {
            if (_capacity > 0)
                clear();
        }
    }
};
