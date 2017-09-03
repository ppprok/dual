cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_FILES aheap.c
        aheap.h
        alist.c
        alist.h
        barray.c
        barray.h
        base.c
        base.h
        itemset.c
        itemset.h
        problem.c
        problem.h
        queue.c
        queue.h
        shd.c
        stdlib2.c
        stdlib2.h
        undo.c
        undo.h
        vec.c
        vec.h)

add_executable(shd shd.c)