#!/usr/bin/python
import numpy

def print_m(m):
    print("%d %d"%m.shape)
    print("\n".join(" ".join(map(str,row)) for row in m))

m1 = numpy.random.random((2000,300))
m2 = numpy.random.random((300,400))
print_m(m1)
print("")
print_m(m2)
print("")
