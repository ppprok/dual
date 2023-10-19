include_directories(dual common logical ${Boost_INCLUDE_DIRS})

add_subdirectory(dual)
add_subdirectory(logical)
add_subdirectory(common)
add_subdirectory(tests)