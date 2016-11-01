#include "stdafx.h"
#include "CppUnitTest.h"
#include "bits.h"
#include <vector>
#include <boost/assign/std.hpp>
#include "containers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{
	TEST_CLASS(BitsTests)
	{
	public:
		
		TEST_METHOD(bit_count_test)
		{
			bits32 bits = 0x12345678;
            Assert::AreEqual(13, bits_count(bits));

            bits32 bits1 = 0x87654321;
            Assert::AreEqual(13, bits_count(bits1));

            bits64 bits2 = 0x8765432112345678;
            Assert::AreEqual(26, bits_count(bits2));

        }


        TEST_METHOD(one_bit_count_test)
        {
            for (int i = 0; i != 32; ++i)
            {
                bits32 bits = 1 << i;
                Assert::AreEqual(1, bits_count(bits));
            }
        }

        TEST_METHOD(low_bit_iterator_test)
        {
            std::vector<int> i;

            bits32 bits = 0x12345678;

            for (auto lb = low_bit<bits32>(bits); lb; ++lb)
                i.push_back(lb);

            Assert::AreEqual(13, (int)i.size());
            std::vector<int> i0 = boost::assign::list_of(3)(4)(5)(6)(9)(10)(12)(14)(18)(20)(21)(25)(28);
            Assert::IsTrue(equal(i0,i));
        }



	};
}