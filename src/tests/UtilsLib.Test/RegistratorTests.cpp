#include <doctest.h>

#include "Registrator.h"
#include <string>

TEST_CASE("register_instance") {
    auto& registrator = Registrator<std::string>::Instance();
    registrator.Register("s1", "str1");
    registrator.Register("s2", "str2");

    CHECK_EQ(std::string("str1"), registrator.Get("s1"));
    CHECK_EQ(std::string("str2"), registrator.Get("s2"));
    try {
        registrator.Get("s3");
        FAIL("");
    }
    catch (std::exception& e) {
    }
}

TEST_CASE("register_factory") {
    auto& registrator = Registrator<std::string>::Instance();
    registrator.RegisterFactory("s1", []() { return "str1"; });
    registrator.RegisterFactory("s2", []() { return "str2"; });

    CHECK_EQ(std::string("str1"), registrator.Create("s1"));
    CHECK_EQ(std::string("str2"), registrator.Create("s2"));
    try {
        registrator.Create("s3");
        FAIL("");
    }
    catch (std::exception& e) {
    }
}
