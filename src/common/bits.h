#pragma once

#include "stdint.h"

typedef uint32_t bits32;

typedef uint64_t bits64;

typedef unsigned char byte;

struct BitOpTables
{
    // “аблица с числом битов в каждом байте
    static const int bit_count[256];

    // “аблица с номером наименьшего бита в каждом байте
    static const int low_bit_number[256];

    // “аблица с результатами удалени€ наименьшего бита из байта
    static const byte reset_low_bit[256];

    // “аблица с номером наибольшего бита в каждом байте
    static const int high_bit_number[256];

    // “аблица с результатами удалени€ набольшего бита из байта
    static const byte reset_high_bit[256];

};


inline int bits_count(bits32 bits)
{
    return BitOpTables::bit_count[bits & 0xff] + 
        BitOpTables::bit_count[(bits >> 8) & 0xff] + 
        BitOpTables::bit_count[(bits >> 16) & 0xff] + 
        BitOpTables::bit_count[bits >> 24];
}

inline int bits_count(bits64 bits)
{
    return 
        BitOpTables::bit_count[bits & 0xff] + 
        BitOpTables::bit_count[(bits >> 8) & 0xff] + 
        BitOpTables::bit_count[(bits >> 16) & 0xff] + 
        BitOpTables::bit_count[(bits >> 24) & 0xff] +
        BitOpTables::bit_count[(bits >> 32) & 0xff] + 
        BitOpTables::bit_count[(bits >> 40) & 0xff] + 
        BitOpTables::bit_count[(bits >> 48) & 0xff] + 
        BitOpTables::bit_count[bits >> 56];
}

template <typename T>
struct low_bit
{
    int bit;
    T rest_bits;

    low_bit(T bits)
    {
        find_bit(bits, 0);
    }

    operator bool() const
    {
        return bit != -1;
    }

    operator int() const
    {
        return bit;
    }

    low_bit& operator ++()
    {
        find_bit(rest_bits, bit ^ (bit & 0x07));
        return *this;
    }

private:

    void find_bit(T bits, int from)
    {
        if (! bits)
        {
            bit = -1;
            rest_bits = 0;
            return;
        }

        bit = from;
        while(! (bits&0xff))
        {
            bits >>= 8;
            bit += 8;
        }

        auto low = bits&0xff;
        bit += BitOpTables::low_bit_number[low];
        rest_bits = bits ^ low;
        rest_bits |= BitOpTables::reset_low_bit[low];
    }
};


template <typename T>
low_bit<T> find_first(T const& bits)
{
    return bits;
}


//template <typename BitMatrix>
//void read_bit_matrix(std::string const& filename, BitMatrix& bitmatrix)
//{
//    CSVLoader csv;
//    csv.Load(filename);
//    //TODO:
//}

