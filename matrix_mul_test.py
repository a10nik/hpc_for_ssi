#!/usr/bin/python
import numpy
import sys

def print_m(m):
    print("%d %d"%m.shape)
    print("\n".join(" ".join(map(str,row)) for row in m))
n = int(sys.argv[1])
m1 = numpy.random.random((n,n))
m2 = numpy.random.random((n,n))
print_m(m1)
print("")
print_m(m2)
print("")
