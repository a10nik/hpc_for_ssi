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
    int segment_count;
    int sequential_steps_in_segment_count;
    double marginal_tolerance;
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

std::shared_ptr<std::vector<point>> multi_shooting_solve_ode(
                                    multi_shooting_config conf,
                                    std::function<double(double, double)> f,
                                    double x0,
                                    double y0,
                                    double x_max) {
                                        
    auto coarse_solution = midpoint_method(conf.segment_count, f, x0, y0, x_max);
    std::vector<double> seg_y0s(conf.segment_count);
    for (auto i = 0; i < conf.segment_count; i++) {
        seg_y0s[i] = (*coarse_solution)[i].y;
    }
    auto segment_width = (x_max - x0) / conf.segment_count;
    std::vector<std::shared_ptr<std::vector<point>>> solution_segments(conf.segment_count);
    auto converged = false;
    for (auto iteration = 0; iteration < conf.segment_count && !converged; iteration++) {
        #pragma omp parallel for
        for (auto i = 0; i < conf.segment_count; i++) {
            auto seg_x0 = i * segment_width + x0;
            auto seg_x_max = seg_x0 + segment_width;
            solution_segments[i] = midpoint_method(
                conf.sequential_steps_in_segment_count,
                f,
                seg_x0,
                seg_y0s[i],
                seg_x_max);      
        }
        converged = true;
        for (auto i = 1; i < conf.segment_count; i++) {
            auto prev_y = (*solution_segments[i - 1])[conf.sequential_steps_in_segment_count].y;
            if (fabs(seg_y0s[i] - prev_y) > conf.marginal_tolerance) {
                converged = false;
            }
            seg_y0s[i] = prev_y;
        }
        if (converged)
            std::cout << "converged after " << iteration << std::endl;
    }
    auto v = new std::vector<point>();
    v->reserve((conf.sequential_steps_in_segment_count + 1) * (conf.segment_count + 1));
    for (auto sol_seg: solution_segments) {
        v->insert(v->end(), sol_seg->begin(), sol_seg->end());
    }
    return std::shared_ptr<std::vector<point>> {v};
}

void output_points_to_dat_file(std::vector<point> &points, std::ostream &out) {
    for (auto pt: points) {
        out << pt.x << " " << pt.y << std::endl;
    }
}

int main(int argc, char *argv[]) {
    
    
    auto f = [](auto x, auto y){ return 1 / x * sin(x * y); };
    multi_shooting_config conf = {
        segment_count: 500,
        sequential_steps_in_segment_count: 100000,
        marginal_tolerance: 0.001
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
