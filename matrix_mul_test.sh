export OMP_NUM_THREADS=$1
s=${2:-1000000}
make && ./matrix_mul_test.py | time ./bin/matrix_mul
