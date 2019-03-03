#pragma once

#include <assert.h>
#include <cstdlib>
#include <cstring>

// Контейнер, не вызывающий конструкторов своих элементов
template<typename T>
class blob
{

    T* _begin;
    T* _end;
    //int _capacity;

public:

    typedef T* iterator;
    typedef T const* const_iterator;
    typedef T value_type;

    inline blob()
        :_begin(0), _end(0)//, _capacity(0)
    {
    }

    inline blob(int n)
        //:_capacity(n)
    {
        assert(n>=0);
        if (n > 0)
        {
            _begin = (T *) malloc(n * sizeof(T));
            _end = _begin + n;
        }
        else
        {
            _begin = _end = 0;
        }
    }

    inline ~blob()
    {
        reset();
    }
        
    inline void reset()
    {
        if (_begin)
		{
			free(_begin);
			_begin = 0;
		}
        _end = 0;
    }

    inline blob(blob&& b)
    {
        _begin = b._begin;
        _end = b._end;

        b._begin = 0;
        b._end = 0;
    }
    	
    inline blob& operator =(blob&& b)
    {
        if (this == &b)
            return *this;

        reset();
        
        _begin = b._begin;
        _end = b._end;

        b._begin = 0;
        b._end = 0;
        
        return *this;
    }
        
    inline int size() const
    {
        return _end - _begin;
    }

    inline bool empty() const
    {
        return _begin >= _end;
    }

    inline T* begin()
    {
        return _begin;
    }

    inline T* end()
    {
        return _end;
    }

    inline T const* begin() const
    {
        return _begin;
    }

    inline T const* end() const
    {
        return _end;
    }
    
    inline T get(int i) const
    {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    inline T const& operator [](int i) const
    {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    inline T& operator [](int i)
    {
        assert(i >= 0 && i < size());
        return _begin[i];
    }
        
    /*inline bool push_back(T a)
    {
        if (size() >= capacity())
            return false;
        *_end++ = a;
        return true;
    }
    
    inline bool push_back_cref(T const& a)
    {
        if (size() >= capacity())
            return false;
        *_end++ = a;
        return true;
    }*/

    inline void pop_back()
    {
        assert(! empty());
        --_end;
    }


    inline T back() const
    {
        assert(! empty());
        return *(_end-1);
    }

    inline T& back()
    {
        assert(! empty());
        return *(_end-1);
    }

    inline T front() const
    {
        assert(! empty());
        return *_begin;
    }

    inline T& front()
    {
        assert(! empty());
        return *_begin;
    }
    
    
    inline void resize(int s)
    {
        if (s <= 0)
        {
            _end = _begin;
        }
        else if (s < size())
        {
            _end = _begin + s;
        }
        else if (s > size())
        {
            if (! _begin)
            {
                _begin = _end = (T*)calloc(s, sizeof(*_begin));
            }
            else {
                _begin = (T*)realloc(_begin, s*sizeof(*_begin));
                _end = _begin + s;
            }
        }
    }
        
    // Удалить один элемент с указанным номером
    /*inline void erase(int i)
    {
        erase(_begin + i);
    }
    
    // Удалить один элемент
    void erase(T* b)
    {
        erase(b, b+1);
    }

    // Удалить несколько элементов
    void erase(T* b, T*e)
    {
        int s = b - e;
        if (s == 0)
            return;
        assert(_begin <= b && s > 0 && e <= _end);
		memmove(b, e, (_end - e) * sizeof(T));
        _end -= s;
    }
    */

    void assign(T const* b, T const* e)
	{
		resize(e - b);
		if (! empty())
			memcpy(_begin, b, (e - b)*sizeof(T));
	}
	
	template<typename Iter>
    void assign_range(Iter b, Iter e)
    {
		resize(e - b);
        for (auto i = _begin; b != e;)
            *(i++) = *(b++);
    }

    template<typename C>
    void assign(C& c)
    {
        assign_range(std::begin(c), std::end(c));
    }
            
    template<typename Iter>
    blob(Iter b, Iter e)
    {
        assign_range(b, e);
    }

	blob(T const* b, T const* e)
	{
		assign(b, e);
	}

};