import random
import sys

count = int(sys.argv[1]) or 100
deg = int(sys.argv[2]) or 10
weight = int(sys.argv[3]) or 100


print(count)
for x in range(count):
    print(
        " ".join(
            "%s:%s"%(random.randint(0, count-1), random.randint(1, weight)) for _ in range(random.randint(0, deg))
        )
    )
