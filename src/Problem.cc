/**************************************************************************************[Problem.cc]
Copyright (c) 2022, Jelle Pleunes

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**************************************************************************************************/

#include <vector>

#include "Problem.h"

using namespace RcpsptHeuristic;

Problem::Problem(int njobs, int horizon, int nresources)
    : njobs(njobs),
      horizon(horizon),
      nresources(nresources),
      nsuccessors(new int[njobs]),
      successors(new int*[njobs]),
      predecessors(new std::vector<int>[njobs]),
      durations(new int[njobs]),
      requests(new int**[njobs]),
      capacities(new int*[nresources]) {
    for (int i = 0; i < njobs; i++)
        predecessors[i] = std::vector<int>();

    for (int i = 0; i < njobs; i++)
        requests[i] = new int*[nresources];

    for (int i = 0; i < nresources; i++)
        capacities[i] = new int[horizon];
}

Problem::~Problem() {
    int i, j;

    delete[] nsuccessors;
    delete[] durations;

    for (i = 0; i < njobs; i++) delete[] successors[i];
    delete[] successors;

    for (i = 0; i < njobs; i++) std::vector<int>().swap(predecessors[i]); // Swapping with an empty vector deallocates the memory
    delete[] predecessors;

    for (i = 0; i < njobs; i++) {
        for (j = 0; j < nresources; j++) {
            delete[] requests[i][j];
        }
        delete[] requests[i];
    }
    delete[] requests;

    for (i = 0; i < nresources; i++) delete[] capacities[i];
    delete[] capacities;
}
