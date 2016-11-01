#include "stdafx.h"
#include "CppUnitTest.h"
#include "WinHeap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{		
	TEST_CLASS(WinHeapTests)
	{
	public:
		
		TEST_METHOD(win_heap_alloc)
		{
            WinHeap& h = WinHeap::Instance();

            auto p = h.Alloc<int>(10);
            Assert::AreEqual(h.Size(p), 10);
            h.Free(p);
		}

        TEST_METHOD(win_heap_realloc)
        {
            WinHeap& h = WinHeap::Instance();

            auto p = h.Alloc<int>(10);
            Assert::AreEqual(h.Size(p), 10);

            p = h.ReAlloc(p, 4);
            Assert::AreEqual(h.Size(p), 4);

            p = h.ReAlloc(p, 12);
            Assert::AreEqual(h.Size(p), 12);

        }
	};
}