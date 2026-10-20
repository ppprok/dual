add_executable(TestLogical ECVotingTests.cpp
        BoostingTests.cpp
        AlgorithmEstimatorTests.cpp
        PSVotingTests.cpp
        FeatureSelectionTests.cpp
        ECmpVotingInitLBTests.cpp
        FTVotingTests.cpp
        ECTests.cpp
        DatasetTests.cpp
        CmpMatrixTests.cpp
        Logging.cpp
        ECmpVotingLBTests.cpp
        ECmpVotingTests.cpp)

target_link_libraries(TestLogical PRIVATE locical)