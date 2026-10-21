// Client of RuncFixed built together with its own -O2 copy of RuncFixed.cpp (see CMakeLists.txt):
// with optimization the RuncFixed members are inlined into RuncFixedDualize and are emitted only
// if they are instantiated explicitly, so this executable links only when they are.
#include <doctest.h>

#include <string>

#include "IntegersFileReader.h"
#include "RuncFixed.h"

TEST_CASE("RuncFixed client built with -O2 links and dualizes") {
    struct {
        const char* file;
        int cols;
        int count;
    } cases[] = {{"data/6_6_4.hg", 6, 4},
                 {"data/13_7_10.hg", 7, 10},
                 {"data/15_10_14.hg", 10, 14},
                 {"data/15_20_275.hg", 20, 275},
                 {"data/10_25_480.hg", 25, 480},
                 {"data/10_50_7187.hg", 50, 7187},
                 {"data/15_35_12226.hg", 35, 12226}};

    for (auto const& c : cases) {
        CAPTURE(std::string(c.file));
        auto rows64 = IntegersFileReader(c.file).ReadBitMatrix64();
        CHECK_EQ(c.count, (int) RuncFixedDualize<bits64>(rows64).size());

        RuncFixed<bits64> runc;
        runc.Dualize(rows64);
        CHECK_EQ(c.count, (int) runc.idealIndep.size());

        if (c.cols <= 32) {
            auto rows32 = IntegersFileReader(c.file).ReadBitMatrix32();
            CHECK_EQ(c.count, (int) RuncFixedDualize<bits32>(rows32).size());
        }
    }
}
