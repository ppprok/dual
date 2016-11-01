#include "stdafx.h"
#include "..\UtilsLib\Registrator.h"


namespace UtilsLibTest
{
    TEST_CLASS(RegistratorTests)
    {
    public:


        TEST_METHOD(register_instance)
        {
            auto& registrator = Registrator<std::string>::Instance();
            registrator.Register("s1", "str1");
            registrator.Register("s2", "str2");

            Assert::AreEqual(std::string("str1"), registrator.Get("s1"));
            Assert::AreEqual(std::string("str2"), registrator.Get("s2"));
            try
            {
                registrator.Get("s3");
                Assert::Fail();
            }
            catch(error& e)
            {
                Assert::AreEqual(
                    std::string("s3"), 
                    *boost::get_error_info<object_name>(e) );
            }
            catch(...)
            {
                Assert::Fail();
            }
        }

        TEST_METHOD(register_factory)
        {
            auto& registrator = Registrator<std::string>::Instance();
            registrator.RegisterFactory("s1", [](){return "str1";});
            registrator.RegisterFactory("s2", [](){return "str2";});
            
            Assert::AreEqual(std::string("str1"), registrator.Create("s1"));
            Assert::AreEqual(std::string("str2"), registrator.Create("s2"));
            try
            {
                registrator.Create("s3");
                Assert::Fail();
            }
            catch(error& e)
            {
                Assert::AreEqual(
                    std::string("s3"), 
                    *boost::get_error_info<object_name>(e) );
            }
            catch(...)
            {
                Assert::Fail();
            }
        }

        

    };
}