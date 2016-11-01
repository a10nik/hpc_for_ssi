export OMP_NUM_THREADS=$1
s=${2:-500}
make && ./matrix_mul_test.py ${s} | time ./bin/matrix_mul
