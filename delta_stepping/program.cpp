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

struct edge { int to, weight; };
typedef std::vector<std::vector<edge>> graph;

struct state {
    std::vector<int> prev;
    std::vector<int> dist;
    std::vector<std::unordered_map<int,std::unordered_set<int>>> buckets;
    std::vector<int> pu_by_vertex;
    int pu_count;
};

int add(int a, int b) {
    auto max = std::numeric_limits<int>::max();
    if (a > max - b)
        return max;
    return a + b;
}

struct request {
    int prev;
    int vert;
    int dist;
};

void relax(state &st, int delta, request &req) {
    auto x = req.dist;
    auto v = req.vert;
    auto pu = st.pu_by_vertex[v];
    if (x < st.dist[v]) {
        auto pus_buckets = &st.buckets[pu];
        auto prev_i = st.dist[v] / delta;
        auto next_i = x / delta;
        auto prev = pus_buckets->find(prev_i);
        if (prev != pus_buckets->end()) {
            prev->second.erase(v);
            if (prev->second.size() == 0) {
                pus_buckets->erase(prev_i);
            }
        }
        auto next = pus_buckets->find(next_i);
        if (next != pus_buckets->end()) {
            next->second.insert(v);
        } else {
            std::unordered_set<int> b;
            b.insert(v);
            (*pus_buckets)[next_i] = b;
        }        
        st.dist[v] = x;
        st.prev[v] = req.prev;
    }
}

int find_first_non_empty_bucket(state &st) {
    auto min = std::numeric_limits<int>::max();
    for (auto pu_buckets: st.buckets) {
        for (auto b: pu_buckets) {
            if (b.first < min)
                min = b.first;
        }        
    }
    return min == std::numeric_limits<int>::max() ? -1 : min;
}

std::vector<std::vector<request>> find_requests(graph &g, state &st, std::unordered_set<int> &vs, int delta, int estimated_degree, bool light) {
    std::vector<std::vector<request>> res {st.pu_count, std::vector<request>()};
    for (auto r: res) {
        r.resize(estimated_degree);
    }
    for (int v : vs) {
        for (auto e: g[v]) {
            auto v2 = e.to;
            auto w = e.weight;
            if ((light && w < delta) || (!light && w >= delta)) {
                request req {v, v2, add(w, st.dist[v])};
                auto pu = st.pu_by_vertex[v2];
                res[pu].push_back(req);
            }
        }
    }
    return res;
}

std::vector<int> distribute_between_pus(int vert_count, int pus) {
    std::vector<int> res;
    res.reserve(vert_count);
    for (int i = 0; i < vert_count; i++) {
        res.push_back(i % pus);
    }
    return res;
}

std::vector<int> dijkstra(const std::vector<std::vector<edge>> &g, int source) {
    std::vector<int> min_distance(g.size(), std::numeric_limits<int>::max());
    min_distance[source] = 0;
    std::set<std::pair<int,int>> active_vertices;
    active_vertices.insert( {0,source} );
        
    while (!active_vertices.empty()) {
        int where = active_vertices.begin()->second;
        active_vertices.erase( active_vertices.begin() );
        for (auto ed : g[where]) 
            if (min_distance[ed.to] > min_distance[where] + ed.weight) {
                active_vertices.erase( { min_distance[ed.to], ed.to } );
                min_distance[ed.to] = min_distance[where] + ed.weight;
                active_vertices.insert( { min_distance[ed.to], ed.to } );
            }
    }
    return min_distance;
}

bool bucket_is_empty_on_all_pus(state &st, int buck_i) {
    for (int pu = 0; pu < st.pu_count; pu++) {
        if (st.buckets[pu].find(buck_i) != st.buckets[pu].end())
            return false;
    }
    return true;
}

std::shared_ptr<state> delta_stepping(graph &g, int s, int delta, int threads) {
    auto st = new state {
        prev: std::vector<int>(g.size(), -1),
        dist: std::vector<int>(g.size(), std::numeric_limits<int>::max()),
        buckets: std::vector<std::unordered_map<int,std::unordered_set<int>>>(threads, std::unordered_map<int,std::unordered_set<int>>()),
        pu_by_vertex: distribute_between_pus(g.size(), threads),
        pu_count: threads
        };

    request init {-1, s, 0};
    relax(*st, delta, init);
    int buck_i;
    int iter = 0;
    auto max_deg = 0;
    for (auto es: g) {
        if (es.size() > max_deg)
            max_deg = es.size();
    }
    while ((buck_i = find_first_non_empty_bucket(*st)) != -1) {
        iter++;
        std::vector<std::unordered_set<int>> r {st->pu_count, std::unordered_set<int>()};
        while (!bucket_is_empty_on_all_pus(*st, buck_i)) {
            std::vector<std::vector<std::vector<request>>> light_reqs {st->pu_count, std::vector<std::vector<request>>()};
            #pragma omp parallel for
            for (auto pu = 0; pu < st->pu_count; pu++) {
                if (st->buckets[pu].find(buck_i) != st->buckets[pu].end()) {
                    for (auto v: st->buckets[pu][buck_i]) {
                        r[pu].insert(v);
                    }                
                    light_reqs[pu] = find_requests(g, *st, st->buckets[pu][buck_i], delta, max_deg, true);
                    st->buckets[pu].erase(buck_i);                    
                } else {
                    light_reqs[pu] = {st->pu_count, std::vector<request>()};
                }
            }
            #pragma omp parallel for
            for (auto pu = 0; pu < st->pu_count; pu++)
            {
                auto start = std::chrono::system_clock::now();

                for (auto sender_pu = 0; sender_pu < st->pu_count; sender_pu++) {
                    for (auto req: light_reqs[sender_pu][pu]) {
                        relax(*st, delta, req);
                    }            
                }

            }
        }
        std::vector<std::vector<std::vector<request>>> heavy_reqs {st->pu_count, std::vector<std::vector<request>>()};
        #pragma omp parallel for
        for (auto pu = 0; pu < st->pu_count; pu++) {
            heavy_reqs[pu] = find_requests(g, *st, r[pu], delta, max_deg, false);
        }
        #pragma omp parallel for
        for (auto pu = 0; pu < st->pu_count; pu++)
        {
            for (auto sender_pu = 0; sender_pu < st->pu_count; sender_pu++) {
                for (auto req: heavy_reqs[sender_pu][pu]) {
                    relax(*st, delta, req);
                }            
            }
        }
    }
    std::cout << "iterations: " << iter << std::endl;
    return std::shared_ptr<state>{st};
}



std::vector<int> split(const std::string& input, const std::string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    std::vector<int> res;
    for (auto i = first; i != last; i++) {
        if (i->length() > 0) {
            res.push_back(std::stoi(*i));
        }
    }
    return res;
}

std::shared_ptr<graph> read_graph(std::istream &in) {
    auto g = new graph();
    std::string size_line;
    std::getline(in, size_line);
    auto size = std::stoi(size_line);
    for (auto v = 0; v < size; v++) {
        std::string line;
        std::getline(in, line);
        auto verts_and_weights = split(line, "\\s+|\\:");
        std::vector<edge> edges;
        for (auto i = 0; i < verts_and_weights.size(); i+=2) {
            edge e {verts_and_weights[i], verts_and_weights[i+1]};
            edges.push_back(e);
        }
        g->push_back(edges);
    }
    return std::shared_ptr<graph>{g};
}

void print_graph(graph &g, std::ostream &out) {
    out << "graph:" << std::endl;
    for (size_t v = 0; v < g.size(); v++) {
        out << " vert " << v << std::endl;
        for (auto e: g[v]) {
            out << " -> " << e.to << ", weight: " << e.weight << std::endl;
        }
    }
}


int main(int argc, char *argv[]) {
    std::ifstream in("../test_graph_100000");
    auto g = read_graph(in);
    std::cout << "graph read" << std::endl;
        
    auto dj_start = std::chrono::system_clock::now();
    auto dj_res = dijkstra(*g, 0);
    auto dj_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - dj_start).count();
    std::cout << "dijkstra ended. time: " << dj_dur << std::endl;
    
    int max = omp_get_max_threads();
    for (auto i = 1; i <= max; i++) {
        omp_set_num_threads(i);
        auto start = std::chrono::system_clock::now();
        auto my_res = delta_stepping(*g, 0, 1000, i)->dist;
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
        std::cout << i << "-thread delta ended. time: " << dur << std::endl;
        
        for (auto i = 0; i < dj_res.size(); i++) {
            if (my_res[i] != dj_res[i])
                std::cerr << "ERR: my dist to " << i << ": " << my_res[i] << " their dist : " << dj_res[i] << std::endl;
        }
        
    }

}