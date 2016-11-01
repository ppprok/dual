#include "stdafx.h"

#include <windows.h>

#include "errors.h"
#include "WinHeap.h"
#include "logging_utils.h"
//#include "Exceptions.h"

WinHeap::WinHeap()
{
    _heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
    
    if (! _heap)
        BOOST_THROW_EXCEPTION(error("Create heap error") <<
        boost::errinfo_api_function("HeapCreate") <<
        boost::errinfo_errno(GetLastError()));
}

WinHeap::~WinHeap()
{
    if (! _heap)
        return;
    
    auto r = HeapDestroy(_heap);    
    if (! r)
        BOOST_THROW_EXCEPTION(error("Destroy heap error") <<
        boost::errinfo_api_function("HeapDestroy") <<
        boost::errinfo_errno(GetLastError()));
    
    _heap = 0;
}

void* WinHeap::Alloc( int size )
{
    if (size <= 0)
        size = 1;

    void* r = HeapAlloc(_heap, HEAP_NO_SERIALIZE, size);
    
    if (! r)
    {
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::bad_alloc()) << 
            boost::errinfo_api_function("HeapAlloc")<<
            boost::errinfo_errno(GetLastError()));
    }
        
    
    return r;
}

void WinHeap::Free( void* ptr )
{
    auto r = HeapFree(_heap, HEAP_NO_SERIALIZE, ptr);
    if (! r)
    {
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::bad_alloc()) << 
            boost::errinfo_api_function("HeapFree")<<
            boost::errinfo_errno(GetLastError()));
    }

}

void* WinHeap::ReAlloc( void* ptr, int size )
{
    if (! ptr)
        return Alloc(size);

    if (size <= 0)
        size = 1;
    
    ptr = HeapReAlloc(_heap, HEAP_NO_SERIALIZE, ptr, size);
    
    if (! ptr)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::bad_alloc()) << 
            boost::errinfo_api_function("HeapReAlloc")<<
            boost::errinfo_errno(GetLastError()));
    }


    return ptr;
}


int WinHeap::Size( void* ptr )
{
    auto s = HeapSize(_heap, HEAP_NO_SERIALIZE, ptr);

    if (s == (SIZE_T)-1)
    {
        BOOST_THROW_EXCEPTION(
            error("Can't define size of memory block") << 
            boost::errinfo_api_function("HeapSize"));
    }

    return s;
}