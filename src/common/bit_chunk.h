#pragma once

#include <assert.h>
#include <vector>

#include "chunk.h"
#include "raw_vector.h"
#include "bits.h"

template <int value>
struct log2
{
    static const int value = log2<value/2>::value+1;
};

template <>
struct log2<1>
{
    static const int value = 0;
};





// Блок с доступом к битам
//template <typename BaseChunk>
struct bit_chunk : chunk<unsigned int>
{
    typedef unsigned char byte;
    
    typedef value_type Block;

    //typedef raw_vector<Block> Blocks;

    static const int bits_per_block = sizeof(Block) * 8;
    static const int low_index_bit_count = log2<bits_per_block>::value;
    static const int low_index_bit_mask = bits_per_block - 1;

    // Информация о номере блока и маске получаемого бита
    struct get_bit_iterator
    {
        int blockNum;
        Block bitMask;
        
        inline get_bit_iterator( int i )
        {
            if (i < 0)
            {
                bitMask = 0;
                blockNum = 0;
            }
            else
            {
                blockNum = i >> low_index_bit_count;
                bitMask = Block(1) << (i & low_index_bit_mask);
            }
        }
    };
        
    static inline int block_for_bits(int n)
    {
        int b = n >> low_index_bit_count;
        if (n & low_index_bit_mask)
            return b+1;
        return b;
    }

        
    // Урезать до последней единицы
    inline void trim()
    {
        assert(valid());
        for (auto d = end-1; begin != end && ! *d; --d, --end){};
    }

    // Если число блоков меньше s, то добавить блоки, заполнив их bit-тами
    inline void pad_blocks(int s, bool bit = false)
    {
        assert(valid());
        
        auto end1 = begin + s;
        if (end1 <= end)
            return;
        if (bit)
            do_pad_ones(end1);
        else
            do_pad_zeros(end1);
    }

private:

    inline void do_pad_zeros(Block* end1)
    {
        assert(end < end1);
        for (;end != end1; ++end)
            *end = 0;
    }

    inline void do_pad_ones(Block* end1)
    {
        assert(end < end1);
        for (;end != end1; ++end)
            *end = -1;
    }

public:
    
    
    inline bool none() const
    {
        assert(valid());
        for (auto i = end; i != begin; --i)
            if (*(i-1))
                return false;
        return true;
    }

    inline void assign_and( bit_chunk const& r1, bit_chunk const& r2 )
    {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());
        
        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;
        
        for(auto d = begin; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2)
        {
            if (*d = *e1 & *e2)
                endd = d+1;
        }

        end = endd;
    }

    inline void assign_or( bit_chunk const& r1, bit_chunk const& r2 )
    {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto d = begin;
        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;

        for(; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2)
        {
            if (*d = *e1 | *e2)
                endd = d+1;
        }

        for(; e1!= r1.end; ++e1, ++d)
        {
            if (*d = *e1)
                endd = d+1;
        }

        for(; e2 != r2.end; ++e2, ++d)
        {
            if (*d = *e2)
                endd = d+1;
        }
        end = endd;
    }

    inline void assign_sub( bit_chunk const& r1, bit_chunk const& r2 )
    {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto d = begin;
        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;

        for(; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2)
        {
            if (*d = *e1 & ~*e2)
                endd = d+1;
        }

        for(; e1 != r1.end; ++e1, ++d)
        {
            if (*d = *e1)
                endd = d + 1;
        }
        
        end = endd;
    }

    inline void assign( bit_chunk const& r )
    {
        assert(valid());
        assert(r.valid());
        
        auto endd = begin;
        for (auto d = begin, e = r.begin; e != r.end; ++e, ++d)
        {
            if (*d = *e)
                endd = d+1;
        }
        end = endd;
    }

    inline void flip()
    {
        assert(valid());

        auto cend = end;
        for (auto i = end = begin; i != cend; ++i)
        {
            if (*i = ~*i)
                end = i+1;
        }
    }

    inline bool get( int i ) const
    {
        assert(valid());
        
        if (i < 0)
            return false;
        int h = i >> low_index_bit_count;
        auto d = begin + h;
        if (d >= end)
            return false;

        return (*d & (Block(1) << (i & low_index_bit_mask))) != 0;
    }

    inline bool get( get_bit_iterator it ) const
    {
        assert(valid());
        
        auto d = begin + it.blockNum;
        if (d >= end)
            return false;
        return (*d & it.bitMask) != 0;
    }

    inline void set( int i)
    {
        assert(i>=0 && valid());

        int h = i >> low_index_bit_count;

        auto d = begin + h;

        if (d >= end)
            do_pad_zeros(d+1);

        *d |= get_bit_mask(i);
    }

    inline void reset( int i)
    {
        assert(valid());

        if (i < 0)
            return;

        int h = i >> low_index_bit_count;

        auto d = begin + h;

        if (d >= end)
            return;
            
        *d &= get_bit_inv_mask(i);
    }


    inline void set( int i, bool value)
    {
        if (value)
            set(i);
        else
            reset(i);
    }

protected:

    static inline int get_first(Block b)
    {
        auto b1 = b & 0xff;
        if (b1)
            return BitOpTables::low_bit_number[b1];

        auto b2 = b & 0xff00;
        if (b2)
            return BitOpTables::low_bit_number[b2 >> 8] + 8;

        auto b3 = b & 0xff0000;
        if (b3)
            return BitOpTables::low_bit_number[b3 >> 16] + 16;

        return BitOpTables::low_bit_number[b >> 24] + 24;
    }

    static inline  int get_last(Block b)
    {
        auto b1 = b & 0xff000000;
        if (b1)
            return BitOpTables::low_bit_number[b1 >> 24] + 24;
        
        auto b2 = b & 0xff0000;
        if (b2)
            return BitOpTables::low_bit_number[b2 >> 16] + 16;

        auto b3 = b & 0xff00;
        if (b3)
            return BitOpTables::low_bit_number[b3 >> 8] + 8;

        return BitOpTables::low_bit_number[b];
    }
    
public:

    inline int find_first() const
    {
        assert(valid());
        int i = 0;
        for (auto d = begin; d != end; ++d, i+=bits_per_block)
        {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }
        return -1;
    }


    inline int find_next( int k ) const
    {
        assert(valid());
        assert(k>=-1);

        int h = (k+1) >> low_index_bit_count;
        auto d = begin + h;
        assert(d <= end);
        if (d == end)
            return -1;

        int i = h << low_index_bit_count;
        int r = (k+1) & low_index_bit_mask;
        auto b = *d >> r;
        if (b)
            return i + r + get_first(b);
        i+=bits_per_block;
        ++d;
        
        for (; d != end; ++d, i += bits_per_block)
        {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }

        return -1;
    }

    
    inline int find_last() const
    {
        assert(valid());

        for (auto d = end; d != begin; )
        {
            --d;
            auto b = *d;
            if (b)
                return get_last(b) + ((d - begin) << low_index_bit_count);            
        }

        return -1;
    }


        // Установить несколько битов
    template <typename Iter>
    inline void set_bits(Iter b, Iter e)
    {
        for(; b!= e; ++b)
            set(*b);
    }

    // Установить несколько битов
    template <typename C>
    inline void set_bits(C const& items)
    {
        set_bits(std::begin(items), std::end(items));
    }

    // Пометить битами объекты, удовлетворяющие предикату
    template <typename C, typename Pred>
    void set_if(C const& items, Pred pred)
    {
        int j = 0;
        for(auto const& item : items)
        {
            if (pred(item))
                set(j);
            ++j;
        }
    }


        
    // Получить номера установленных битов
    template <typename C>
    inline void get_bits(C& bits) const
    {
        for (int k = find_first(); k != -1; k = find_next(k))
            bits.push_back(k);
    }



    // Выбрать объекты, помеченные битами
    template<typename C1, typename C2>
    void get_selected(C1 const& items, C2& selected)
    {
        for(int j = find_first(),
            n = items.size(); 
            j != -1 && j < n; j = find_next(j))
        {
            selected.push_back(items[j]);
        }
    }



    inline bool operator<( bit_chunk const& r ) const
    {
        assert(valid() && r.valid());
        
        auto q = begin;
        auto e = r.begin;
        for(; q != end && e != r.end; ++q, ++e)
        {
            if (*q > *e)
                return false;
            if (*q < *e)
                return true;
        }

        for (;q != end; ++q)
            if (*q)
                return false;

        for (;e != r.end; ++e)
            if (*e)
                return true;

        return false;
    }
    
    bool operator==( bit_chunk const& r ) const
    {
        assert(valid() && r.valid());

        auto q = begin;
        auto e = r.begin;
        for(; q != end && e != r.end; ++q, ++e)
        {
            if (*q != *e)
                return false;
        }

        for (;q != end; ++q)
            if (*q)
                return false;

        for (;e != r.end; ++e)
            if (*e)
                return false;

        return true;
    }

    inline bool is_subset_of( bit_chunk const& b ) const
    {
        assert(valid());
        assert(b.valid());

        auto b1 = begin;
        auto b2 = b.begin;
        
        for(; b1 != end && b2 != b.end; ++b1, ++b2)
            if ((*b1 & *b2) != *b1)
                return false;

        for (;b1 != end; ++b1)
            if (*b1)
                return false;

        return true;
    }

    static inline int subset_cmp( bit_chunk const& b1, bit_chunk const& b2 )
    {
        assert(b1.valid());
        assert(b2.valid());

        auto d1 = b1.begin, d2 = b2.begin;
        int r = 0;

        for(; d1 != b1.end && d2 != b2.end; ++d1, ++d2)
        {
            auto a1 = *d1;
            auto a2 = *d2;
            
            if (a1 == a2)
                continue;
            
            auto a = a1 & a2;
            if (a == a1)
            {
                if (r == 1)
                    return 0;
                r = -1;
                continue;
            }

            if (a == a2)
            {
                if (r == -1)
                    return 0;
                r = 1;
                continue;
            }
            
            return 0;
        }

        if (r != 1)
            for (;d1 != b1.end; ++d1)
                if (*d1)
                    return r+1;

        if (r != -1)
            for (; d2 != b2.end; ++d2)
                if (*d2)
                    return r-1;

        if (r)
            return r;

        // если два набора совпадают, что считаем первый подмножеством второго
        return -1;
    }
    
    
    inline bool intersects( bit_chunk const& b ) const
    {
        assert(valid());
        assert(b.valid());
        
        for (auto d = begin, e = b.begin; d != end && e != b.end; ++d, ++e)
            if (*d & *e)
                return true;

        return false;
    }


    
public:

    inline int count() const
    {
        assert(valid());
        
        int c = 0;
        for (auto d = begin; d != end; ++d)
        {
            auto b = *d;
            if (b)
                c+= bits_count(b);
        }
        return c;
    }

    inline int count(int maxc) const
    {
        assert(valid());
        
        int c = 0;
        for (auto d = begin; d != end; ++d )
        {
            auto b = *d;
            if (b)
            {
                c+= bits_count(b);
                if (c > maxc)
                    return c;
            }
            
        }
        return c;
    }


    inline int bit_size() const
    {
        return size() << low_index_bit_count;
    }

    inline Block get_bit_inv_mask( int pos )
    {
        return ~(Block(1) << (pos & low_index_bit_mask));
    }

    inline Block get_bit_mask( int pos )
    {
        return Block(1) << (pos & low_index_bit_mask);
    }

    inline bit_chunk( Block* b, Block* e ) 
        : chunk(b, e)
    {
    }

    inline bit_chunk()
    {
    }
    
    inline bool any() const
    {
        return ! none();
    }

    inline void pad_bits( int n, bool bit /*= false*/ )
    {
        pad_blocks(block_for_bits(n), bit);
    }
    
};

typedef std::vector<bit_chunk> bit_chunks;

// Набор битов с прискоенным весом
template <typename Weight>
struct weighted_bit_chunk
    :bit_chunk
{
    Weight weight;

    weighted_bit_chunk(Block* b = 0, Block* e = 0, Weight w = 0)
        :bit_chunk(b,e), weight(w)
    {
    }

    weighted_bit_chunk(bit_chunk const& bits, Weight w = 0)
        :bit_chunk(bits), weight(w)
    {
    }

    void assign(weighted_bit_chunk const& r)
    {
        bit_chunk::assign(r);
        weight = r.weight;
    }
};