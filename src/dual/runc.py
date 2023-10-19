import ctypes

mylib = ctypes.CDLL("/home/user/dual/cmake-build-debug/src/dual/librunc.so")

mylib.init_dualizer.restype = ctypes.c_void_p
mylib.init_dualizer.argtypes = [ctypes.c_int, ctypes.c_int]

mylib.release_dualizer.argtypes = [ctypes.c_void_p]

dualizer = mylib.init_dualizer(30, 40)

print(dualizer)

mylib.release_dualizer(dualizer)

