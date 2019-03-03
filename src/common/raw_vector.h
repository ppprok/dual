#pragma once

#include <memory>
#include <cstring>

// Контейнер, не вызывающий конструкторов своих элементов
template<typename T>
class raw_vector
{

    T* _begin;
    T* _end;
    int _capacity;

public:

    typedef T* iterator;
    typedef T const* const_iterator;
    typedef T value_type;

    raw_vector()
        :_begin(0), _end(0), _capacity(0)
    {
    }

    ~raw_vector()
    {
        reset();
    }

    inline int capacity() const
    {
        return _capacity;
    }

    void reset()
    {
        if (_begin)
		{
			free(_begin);
			_begin = 0;
		}
        _capacity = 0;
        _end = 0;
    }

    raw_vector(raw_vector&& b)
    {
        _begin = b._begin;
        _end = b._end;
        _capacity = b._capacity;

        b._begin = 0;
        b._end = 0;
        b._capacity = 0;
    }

	raw_vector(raw_vector const& vec)
	{
		if (! vec.empty())
			assign(vec._begin, vec._end);
	}

	raw_vector& operator =(raw_vector const& vec)
	{
		if (! vec.empty())
			assign(vec._begin, vec._end);
		else
			clear();

		return *this;
	}

    raw_vector& operator =(raw_vector&& b)
    {
        if (this == &b)
            return *this;

        reset();

        _begin = b._begin;
        _end = b._end;
        _capacity = b._capacity;

        b._begin = 0;
        b._end = 0;
		b._capacity = 0;
        
        return *this;
    }
    
    bool valid() const
    {
        return _begin && size() >= 0;
    }

    int size() const
    {
        return _end - _begin;
    }

    bool empty() const
    {
        return size() == 0;
    }

    T* begin()
    {
        return _begin;
    }

    T* end()
    {
        return _end;
    }

    T const* begin() const
    {
        return _begin;
    }

    T const* end() const
    {
        return _end;
    }

	/*T* rbegin()
	{
		return _end - 1;
	}

	T* rend()
	{
		return _begin - 1;
	}

	T const* rbegin() const
	{
		return _end - 1;
	}

	T const* rend() const
	{
		return _begin - 1;
	}*/

    T operator [](int i) const
    {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    T& operator [](int i)
    {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    void reserve(int s)
    {
        if (_capacity >= s)
            return;

        if (! _begin) {
            _begin = _end = (T*)malloc(s*sizeof(T));
            _capacity = s;
            return;
        }

        int cs = size();
        _begin = (T*)realloc(_begin, s*sizeof(T));
        _end = _begin + cs;
        _capacity = s;
    }

    void reserve_extra(int s)
    {
        if (s > capacity())
            reserve(s + capacity() / 2);
    }

    void clear()
    {
        _end = _begin;
    }

    void prepare_push_back(int s = 1)
    {
		reserve_extra(s + size());		
    }
	
    void push_back_raw(T a)
    {
        assert(valid() && size() < capacity());
        *_end++ = a;
    }
    	
	void push_back(T a)
	{
		prepare_push_back();
		push_back_raw(a);
	}
    	
    T back() const
    {
        assert(! empty());
        return *(_end-1);
    }

    T& back()
    {
        assert(! empty());
        return *(_end-1);
    }

    T front() const
    {
        assert(! empty());
        return *_begin;
    }

    T& front()
    {
        assert(! empty());
        return *_begin;
    }

    
    void pop_back()
    {
        if (! empty())
			--_end;
    }

    void resize_raw(int s)
    {
        if (s < 0)
            s = 0;
        reserve_extra(s);
        _end = _begin + s;
    }

    void resize(int s, T val = T())
    {
        if (size() >= s)
            return resize_raw(s);
        
        reserve_extra(s);
        memset((void*)_end, 0, sizeof(T));
        
        if (*_end != val)
        {
            for (int c = size(); c < s; ++c)
                push_back_raw(val);
        }
        else
        {
            auto p = _end + 1;
            _end = _begin + s;
            if (p != _end)
                memset((void*)p, 0, (_end - p) * sizeof(T));
        }
    }

    // Удалить один элемент с указанным номером
    void erase(int i)
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
    raw_vector(Iter b, Iter e)
    {
        assign_range(b, e);
    }

	raw_vector(T const* b, T const* e)
	{
		assign(b, e);
	}

};