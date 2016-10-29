#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
    std::vector<std::string> words;
    std::string s("asdasdasd");
    int nthreads, tid;
    /* Fork a team of threads giving them their own copies of variables */
    #pragma omp parallel private(nthreads, tid)
    {

        /* Obtain thread number */
        tid = omp_get_thread_num();
        printf("Hello World from thread = %d\n", tid);

        /* Only master thread does this */
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }

    }  /* All threads join master thread and disband */
}
