#pragma once

#include <boost/noncopyable.hpp>

// Куча виндоуса для работы с памятью без лишних поползновений
class WinHeap
    :boost::noncopyable
{
    void* _heap;

public:

    WinHeap();

    ~WinHeap();

    void* Alloc(int size);

    void Free(void* ptr);

    void* ReAlloc(void* ptr, int size);
        
    //void* ReAlloc(void* begin, int oldsize, int newsize);

    template <typename T>
    T* Alloc(int size)
    {
        return (T*)Alloc(size * sizeof(T));
    }

    template<typename T>
    void Free(T* ptr)
    {
        return Free((void*)ptr);
    }

    template <typename T>
    T* ReAlloc(T* ptr, int size)
    {
        return (T*)ReAlloc((void*)ptr, size * sizeof(T));
    }

   /* template <typename T>
    T* ReAlloc(T* ptr, int oldsize, int newsize)
    {
        return (T*)ReAlloc((void*)ptr, oldsize * sizeof(T), newsize * sizeof(T));
    }*/

    template <typename tag>
    static WinHeap& Instance()
    {
        static WinHeap h;
        return h;
    }

    static WinHeap& Instance()
    {
        static WinHeap h;
        return h;
    }

    int Size(void* ptr);

    template<typename T>
    int Size(T* ptr)
    {
        return Size((void*)ptr) / sizeof(T);
    }
};