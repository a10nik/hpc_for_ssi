export OMP_NUM_THREADS=$1
s=${2:-1000000}
make && shuf -i 0-10000000 -n $s | time ./bin/hello
