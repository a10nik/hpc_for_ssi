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
#include <regex>
#include <fstream>
#include <limits>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <random>
#include <chrono>
#include <functional>


struct point { double x, y; };

struct multi_shooting_config {
    size_t segment_count;
    size_t sequential_steps_in_segment_count;
    double epsilon;
};

std::shared_ptr<std::vector<point>> midpoint_method(
                                    int steps,
                                    std::function<double(double, double)> f,
                                    double x0,
                                    double y0,
                                    double x_max) {
    auto res = new std::vector<point>();
    res->reserve(steps + 1);
    auto step = (x_max - x0) / steps;
    auto x = x0;
    auto y = y0;
    for (auto i = 0; i < steps + 1; i++) {
        res->push_back({x, y});
        y = y + step * f(x + step / 2.0, y + step * f(x, y) / 2.0);
        x += step;
    }
    return std::shared_ptr<std::vector<point>> {res};
}

double dist_sqr(std::vector<double> &v1, std::vector<double> &v2) {
    auto sum = 0.0;
    for (size_t i = 0; i < v1.size(); i++) {
        sum += (v1[i] - v2[i]) * (v1[i] - v2[i]);
    }
    //std::cout << "dist was " << sum << std::endl;
    return sum;
}

struct matrix {
    std::vector<double> values;
    unsigned int size;
    
    double get(size_t row, size_t col) {
        return values[size * row + col];
    }
    
    void set(size_t row, size_t col, double val) {
        values[size * row + col] = val;
    }
    
    matrix(size_t n): values(n * n, 0.0), size(n) {}
    
    matrix(size_t n, std::function<double(size_t, size_t)> f):
            values(n * n, 0.0),
            size(n) {
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                set(i, j, f(i, j));
            }
        }
    }
    
    std::shared_ptr<std::vector<double>> mul_vect(std::vector<double>& v) {
        std::shared_ptr<std::vector<double>> res(new std::vector<double>(size, 0.0));
        for (size_t i = 0; i < size; i++) {
            auto scalar_prod = 0.0;
            for (size_t j = 0; j < size; j++) {
                scalar_prod += get(i, j) * v[j];
            }
            (*res)[i] = scalar_prod;
        }
        return res;
    }

};

std::shared_ptr<std::vector<double>> add_vect(std::vector<double> &v1, std::vector<double> &v2) {
    std::shared_ptr<std::vector<double>> res(new std::vector<double>(v1.size(), 0.0));
    for (size_t i = 0; i < v1.size(); i++) {
        (*res)[i] = v1[i] + v2[i];
    }
    return res;
}

std::shared_ptr<std::vector<double>> minus_vect(std::vector<double> &v1) {
    std::shared_ptr<std::vector<double>> res(new std::vector<double>(v1.size(), 0.0));
    for (size_t i = 0; i < v1.size(); i++) {
        (*res)[i] = -v1[i];
    }
    return res;
}

struct inv_jac_and_func_val {
    std::shared_ptr<std::vector<double>> func_val;
    std::shared_ptr<matrix> inv_jac;
};

std::shared_ptr<std::vector<double>> newton_method_find_root(std::vector<double> &init_vector,
                                        double epsilon,
                                        std::function<inv_jac_and_func_val(std::vector<double>&)> get_inv_jac_and_func
                                        ) {
    std::shared_ptr<std::vector<double>> x_prev;
    std::shared_ptr<std::vector<double>> x_curr(new std::vector<double>(init_vector));
    
    int i = 0;
    while (!x_prev || dist_sqr(*x_prev, *x_curr) > epsilon * epsilon) {
        i++;
        x_prev = x_curr;
        auto inv_jac_and_func = get_inv_jac_and_func(*x_curr);
        x_curr = add_vect(*x_curr, *inv_jac_and_func.inv_jac->mul_vect(*minus_vect(*inv_jac_and_func.func_val)));
        
        //for (auto i = 0; i < x_curr->size(); i ++)
        //    std::cout << "\t" << (*x_curr)[i];
        //std::cout << std::endl;
    }
    std::cout << "newton iterations " << i << std::endl;
    return x_curr;
    
}

struct right_y_with_deriv {
    double right_y, right_y_deriv;
};

double segment_right_y_deriv_by_its_left_y(
                                            multi_shooting_config& conf,
                                            std::function<double(double, double)> f, 
                                            double x_left,
                                            double y_left,
                                            double x_right) {
    
    auto dy0 = conf.epsilon;
    auto y_right_of_y0_plus_dy0 = midpoint_method(
                1,
                f,
                x_left,
                y_left + dy0,
                x_right)->back();
    auto y_right_of_y0 = midpoint_method(
            1,
            f,
            x_left,
            y_left,
            x_right)->back();
    return 
        (y_right_of_y0_plus_dy0.y - y_right_of_y0.y) / dy0;
}

inv_jac_and_func_val mismatch_and_its_inv_jacobian(multi_shooting_config& conf,
                                            std::function<double(double, double)> f,
                                            double x0,
                                            double y0,
                                            double x_max,
                                            std::vector<double>& seg_y0s
                                            ) {
    
    auto segment_width = (x_max - x0) / conf.segment_count;
    std::vector<double> mismatch_derivs(conf.segment_count - 1);
    std::shared_ptr<std::vector<double>> mismatch(new std::vector<double>(conf.segment_count));
    (*mismatch)[0] = seg_y0s[0] - y0;
    #pragma omp parallel for
    for (size_t i = 0; i < conf.segment_count - 1; i++) {
        auto seg_x0 = i * segment_width + x0;
        auto seg_x_max = seg_x0 + segment_width;
        auto seg_y0 = seg_y0s[i];
        auto seg_y_right = midpoint_method(
                conf.sequential_steps_in_segment_count,
                f,
                seg_x0,
                seg_y0,
                seg_x_max)->back().y;
            
        mismatch_derivs[i] = segment_right_y_deriv_by_its_left_y(conf, f, seg_x0, seg_y0, seg_x_max);
        (*mismatch)[i + 1] = seg_y0s[i + 1] - seg_y_right;
    }
    std::shared_ptr<matrix> jac(new matrix(conf.segment_count));
    #pragma omp parallel for
    for (size_t i = 0; i < jac->size; i++) {
        jac->set(i, i, 1.0);
        auto prod = 1.0;
        for (size_t j = i + 1; j < jac->size; j++) {
            prod *= mismatch_derivs[j - 1];
            jac->set(j, i, prod);
        }
    }
    return inv_jac_and_func_val { mismatch, jac };
}

std::shared_ptr<std::vector<point>> multi_shooting_solve_ode(
                                    multi_shooting_config& conf,
                                    std::function<double(double, double)> f,
                                    double x0,
                                    double y0,
                                    double x_max) {
   auto coarse_solution = *midpoint_method(
                conf.segment_count,
                f,
                x0,
                y0,
                x_max);
    std::vector<double> init_seg_y0s(conf.segment_count);
    for (size_t i = 0; i < conf.segment_count; i++) {
        init_seg_y0s[i] = coarse_solution[i].y;
    }
    auto mismatch_and_inv_jac = [&conf, f, x0, y0, x_max](auto seg_y0s) {
        return mismatch_and_its_inv_jacobian(conf, f, x0, y0, x_max, seg_y0s);
    };
    
    auto root_y0s = *newton_method_find_root(init_seg_y0s, conf.epsilon, mismatch_and_inv_jac);
    
    std::shared_ptr<std::vector<point>> v(
        new std::vector<point>((conf.sequential_steps_in_segment_count + 1) * conf.segment_count)
    );
    auto segment_width = (x_max - x0) / conf.segment_count;
    #pragma omp parallel for
    for (size_t i = 0; i < root_y0s.size(); i++) {
        auto seg_x0 = i * segment_width + x0;
        auto seg_y0 = root_y0s[i];
        auto seg_x_max = seg_x0 + segment_width;
        auto seg_solution = *midpoint_method(conf.sequential_steps_in_segment_count, f, seg_x0, seg_y0, seg_x_max);
        for (size_t j = 0; j < seg_solution.size(); j++) {
            (*v)[seg_solution.size() * i + j] = seg_solution[j];
        }
    }
    return v;
}

void output_points_to_dat_file(std::vector<point> &points, std::ostream &out) {
    for (auto pt: points) {
        out << pt.x << " " << pt.y << std::endl;
    }
}

int main(int argc, char *argv[]) {
    
    
    auto f = [](auto x, auto y){ return 1 / x * sin(x * y); };
    multi_shooting_config conf = {
        segment_count: 100,
        sequential_steps_in_segment_count: 1000000,
        epsilon: 0.0001
    };
    auto from = 0.0001;
    auto initial_value = 10;
    auto to = 3.0;

    {    
        auto start = std::chrono::system_clock::now();
        
        auto points = midpoint_method(conf.sequential_steps_in_segment_count * conf.segment_count, f, from, initial_value, to);
        
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
        std::cout << "single-threaded version ended. time: " << dur << std::endl;        
    }
    
    int max = omp_get_max_threads();
    for (auto i = 1; i <= max; i++) {
        std::ofstream points_out_file("../multi_shooting.dat");
        omp_set_num_threads(i);
        auto start = std::chrono::system_clock::now();
        
        //auto points = midpoint_method(2000, f, from, initial_value, to);
        
        auto points = multi_shooting_solve_ode(conf, f, from, initial_value, to);
        
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
        std::cout << i << "-thread ended. time: " << dur << std::endl;        
        //output_points_to_dat_file(*points, points_out_file);
    }

}
