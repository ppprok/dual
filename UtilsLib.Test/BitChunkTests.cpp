#include "stdafx.h"
#include "CppUnitTest.h"
#include "bit_vector.h"

#include <boost/assign/std/vector.hpp>
#include "containers.h"
#include "bit_mask.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{
	TEST_CLASS(BitChunkTests)
	{
	public:
		
        // Проверка устаовки и получения битов
		TEST_METHOD(get_set_bits_test)
		{
			bit_vector bits(1000);

            bits.set(44);
            bits.set(64);
            bits.set(1000);
            bits.set(511);
            bits.set(999);
            bits.reset(1000);

            Assert::IsTrue(bits.get(44));
            Assert::IsTrue(bits.get(64));
            Assert::IsTrue(bits.get(999));
            Assert::IsTrue(bits.get(511));
            Assert::IsTrue(! bits.get(1000));

            std::vector<int> actual;
            bits.get_bits(actual);
                        
            std::vector<int> expected = boost::assign::list_of(44)(64)(511)(999);

            Assert::IsTrue(equal(actual, expected));
            
            for (int i = 0; i != 1024; ++i)
            {
                if (contains(actual, i))
                    continue;
                Assert::IsFalse(bits.get(i));
            }
            
		}


        TEST_METHOD(assing_bits_test)
        {
            bit_vector bits1(1000);
            bit_vector bits2(1000);

            bits1.set(44);
            bits1.set(64);
            bits1.set(1000);
            bits1.set(511);
            bits1.set(999);

            bits2.assign(bits1);

            Assert::IsTrue(bits1 == bits2);            
        }

        TEST_METHOD(assing_and_bits_test)
        {
            bit_vector bits1(1000);
            bit_vector bits2(1000);

            bits1.set(44);
            bits1.set(64);
            bits1.set(1000);
            bits1.set(511);
            bits1.set(999);
            bits1.set(399);

            bits2.set(4);
            bits2.set(64);
            bits2.set(100);
            bits2.set(511);
            bits2.set(999);

            bit_vector bits3(1000);

            bits3.assign_and(bits1, bits2);

            std::vector<int> actual;
            bits3.get_bits(actual);

            std::vector<int> expected = boost::assign::list_of(64)(511)(999);

            Assert::IsTrue(equal(actual, expected));            
        }


        // Проверка перечисления различающихся битов двух 
        TEST_METHOD(find_diff_bits_test)
        {
            bit_vector b1(1000);
            bit_vector b2(1000);

            b1.set(44);
            b1.set(31);
            b1.set(678);
            b1.set(549);
            b1.set(78);
            b1.trim();

            b2.set(440);
            b2.set(31);
            b2.set(78);
            b2.set(59);
            b2.trim();

            bit_mask m2(b2);

            std::vector<int> actual;

            for (int j = m2.find_first_diff(b1);
                j != -1;
                j = m2.find_next_diff(b1, j))
            {
                actual.push_back(j);
            }

            std::vector<int >expected = 
                boost::assign::list_of(44)(59)(440)(549)(678);

            Assert::IsTrue(equal(expected, actual));
        }

	};
}