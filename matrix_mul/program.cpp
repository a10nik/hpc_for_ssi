#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <random>
#include <ctime>
#include <memory>

template<typename E>
class Matrix
{
public:
    Matrix(std::size_t height, std::size_t width)
    : array(width * height), width(width), height(height) {}

    E& operator()(std::size_t y, std::size_t x) {
        return array[x + width * y];
    }

private:
    std::vector<E> array;
public:
    std::size_t width;
    std::size_t height;
};

void print_matrix(Matrix<double>& m) {
    std::cerr << "matrix: height = " << m.height << " width = " << m.width << std::endl;
    for (int i = 0; i < m.height; i++) {
        for (int j = 0; j < m.width; j++) {
            std::cerr << m(i, j) << " ";
        }
        std::cerr << std::endl;
    }
    std::cerr << "end matrix" << std::endl;
}

std::shared_ptr<Matrix<double>> transpose(Matrix<double>& m) {
    auto transposed = new Matrix<double>{m.width, m.height};
    for (int i = 0; i < m.height; i++) {
        for (int j = 0; j < m.width; j++) {
            (*transposed)(j, i) = m(i, j);
        }
    }
    std::shared_ptr<Matrix<double>> ptr(transposed);
    return ptr;
}

std::shared_ptr<Matrix<double>> mul(Matrix<double>& a, Matrix<double>& b) {
    if (a.width != b.height)
        std::cerr << "can't multiply: a.width = " << a.width << " while b.height = " << b.height << std::endl;
    auto res = new Matrix<double>(a.height, b.width);
    auto b_t = transpose(b);

    #pragma omp parallel for
    for (int i = 0; i < res->height; i++) {
        for (int j = 0; j < res->width; j++) {
            float s = 0;
            for (int k = 0; k < a.width; k++) {
                s += a(i, k) * (*b_t)(j, k);
            }
            (*res)(i, j) = s;
        }
    }
    std::shared_ptr<Matrix<double>> ptr(res);
    return ptr;
}

std::shared_ptr<Matrix<double>> input_matrix() {
    int height;
    std::cin >> height;
    int width;
    std::cin >> width;
    auto m = new Matrix<double>(height, width);
    std::cerr << "inputted dims " << width << " x " << height << std::endl;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            double x;
            std::cin >> x;
            (*m)(i, j) = x;
        }
    }
    std::shared_ptr<Matrix<double>> ptr(m);
    return ptr;
}

int main(int argc, char *argv[]) {
    std::cerr << "threads: " << omp_get_max_threads() << std::endl;
    auto m1 = input_matrix();
    auto m2 = input_matrix();
    auto m3 = mul(*m1, *m2);
}