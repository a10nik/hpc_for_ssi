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

std::vector<int> decimate(std::vector<int>& numbers, int step) {
    std::vector<int> res;
    for (int i = step; i < numbers.size(); i+=step) {
        res.push_back(numbers[i]);
    }
    return res;
}

std::vector<int> get_sample(std::vector<int>& numbers, int size, int decimation_step) {
    std::vector<int> sample;
    std::random_device seeder;
    std::mt19937 engine(seeder());
    std::uniform_int_distribution<int> dist(0, numbers.size()-1);
    for (int i = 0; i < (size+1)*decimation_step; i++) {
        int rand_index = dist(engine);
        sample.push_back(numbers[rand_index]);
    }
    sort(sample.begin(), sample.end());
    return decimate(sample, decimation_step);
}

int get_bucket_num(std::vector<int>& splitters, int number) {
    for (int i = 0; i < splitters.size(); i++) {
        if (number < splitters[i])
            return i;
    }
    return splitters.size();
}

std::vector<int> par_sort(std::vector<int>& numbers) {
    auto begin = clock();
    auto threads = omp_get_max_threads();
    auto splitters = get_sample(numbers, threads-1, 8);
    std::vector<std::vector<int>> buckets;
    for (int i = 0; i < threads; i++) {
        std::vector<int> bucket;
        bucket.reserve(numbers.size() / threads * 2);
        buckets.push_back(bucket);
    }
    std::cout << ((clock() - begin) * 1000 / CLOCKS_PER_SEC) << ": initialized buckets \n";
    for (auto n: numbers) {
        buckets[get_bucket_num(splitters, n)].push_back(n);
    }
    std::cout << ((clock() - begin) * 1000 / CLOCKS_PER_SEC) << ": splitted \n";
    #pragma omp parallel for
    for (int i = 0; i < buckets.size(); i++) {
        auto begin = clock();
        std::sort(buckets[i].begin(), buckets[i].end());
        std::cout << "bucket " << i << ". " << ((clock() - begin) * 1000 / CLOCKS_PER_SEC) << ": sorted " << buckets[i].size() << "numbers \n";
    }
    std::cout << ((clock() - begin) * 1000 / CLOCKS_PER_SEC) << ": sorted buckets \n";
    std::vector<int> res;
    res.reserve(numbers.size());
    for (auto b: buckets) {
        res.insert(res.end(), b.begin(), b.end());
    }
    std::cout << ((clock() - begin) * 1000 / CLOCKS_PER_SEC) << ": merged \n";
    return res;
}

void check_sort(std::vector<int>& numbers, std::vector<int>& sorted) {
    if (numbers.size() != sorted.size())
        std::cerr << "The size do not match!\n";
    auto count = 0;
    for (int i = 0; i < sorted.size()-1; i++) {
        if (sorted[i] > sorted[i+1]) {
            if (count <= 3) {
                std::cerr << "Mismatch at " << (i-1) << " and " << i << std::endl;
            }
            count++;
        }
    }
    if (count == 0)
        std::cout << "Ok" << std::endl;
    else
        std::cout << "Found " << count << " mismatches" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cerr << "threads: " << omp_get_max_threads() << std::endl;
    std::vector<int> numbers(std::istream_iterator<int>(std::cin),
                    std::istream_iterator<int>());
    auto sorted = par_sort(numbers);
    check_sort(numbers, sorted);
}
