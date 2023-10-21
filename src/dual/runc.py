import ctypes

import numpy

mylib = ctypes.CDLL("/home/user/dual/cmake-build-debug/src/dual/librunc.so")

mylib.init_dualizer.restype = ctypes.c_void_p
mylib.init_dualizer.argtypes = [ctypes.c_int, ctypes.c_int]

mylib.release_dualizer.argtypes = [ctypes.c_void_p]

mylib.set_input_row.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                                numpy.ctypeslib.ndpointer(dtype=numpy.int32)]

mylib.dualize.argtypes = [ctypes.c_void_p]
mylib.dualize.restype = ctypes.c_int

dualizer = mylib.init_dualizer(0, 40)

row = numpy.array([0, 4, 7, 10], dtype=numpy.int32)
mylib.set_input_row(dualizer, 0, len(row), row)

print(mylib.dualize(dualizer))

mylib.release_dualizer(dualizer)

