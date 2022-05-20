/*****************************************************************************************[Main.cc]
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

#include <iostream>
#include <fstream>
#include <ctime>
#include <filesystem>
#include <algorithm>

#include "Solver.h"
#include "Parser.h"
#include "Problem.h"

#define FILE_EXTENSION ".smt"

using namespace RcpsptHeuristic;

bool checkValid(const Problem& problem, const int* solution) {
    // Initialize remaining resource availabilities
    int** available = new int*[problem.nresources];
    for (int k = 0; k < problem.nresources; k++) {
        available[k] = new int[problem.horizon];
        for (int t = 0; t < problem.horizon; t++)
            available[k][t] = problem.capacities[k][t];
    }

    for (int job = 0; job < problem.njobs; job++) {
        // Precedence constraints
        for (int predecessor: problem.predecessors[job]) {
            int start = solution[job] - problem.durations[job];
            if (start < solution[predecessor]) {
                std::cout << "invalid precedence!" << std::endl;
                return false;
            }
        }
    }

    for (int job = 0; job < problem.njobs; job++) {
        // Resource constraints
        for (int k = 0; k < problem.nresources; k++) {
            for (int t = 0; t < problem.durations[job]; t++) {
                int curr = solution[job] - problem.durations[job] + t;
                available[k][curr] -= problem.requests[job][k][t];
                if (available[k][curr] < 0) {
                    std::cout << "resource demand exceeds availability at t=" << curr << '!' << std::endl;
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * Recursively finds all .smt test data files in the given directory, and writes the results to the given output file.
 *
 * @param directory the directory path to recursively search under
 * @param output file to writes results to (file path, makespan that was found, cpu time)
 */
void findInstancesAndSolveAll(const std::string& directory, ofstream& output) {
    std::vector<string> paths;
    for (const auto& f : std::filesystem::recursive_directory_iterator(directory)) {
        if (!std::filesystem::is_directory(f) && f.path().extension() == FILE_EXTENSION)
            paths.push_back(f.path());
    }
    std::sort(paths.begin(), paths.end());

    std::cout << "Solving " << paths.size() << " problems..." << std::endl;
    for (int i = 0; i < (int)paths.size(); i++) {
        std::ifstream inpFile(paths[i]);
        if (!inpFile) {
            std::cerr << "Can't open input file: " << paths[i] << std::endl;
            exit(1);
        }
        Problem problem = Parser::parseProblemInstance(inpFile);
        inpFile.close();

        int* result = new int[problem.njobs];
        std::clock_t start = std::clock();
        bool found = PrSolver(problem).solve(result);
        std::clock_t end = std::clock();

        long milis = ((end - start) * 1000) / CLOCKS_PER_SEC;
        output << paths[i] << std::endl;
        if (found) output << "makespan " << result[problem.njobs - 1] << std::endl;
        else output << "nosolution" << std::endl;
        output << "cpu_milis " << milis << std::endl;
        output << std::endl;
        if (found && !checkValid(problem, result)) std::cout << "Invalid solution: " << paths[i] << std::endl;

        delete[] result;
        if (i % (paths.size()/100) == 0) std::cout << i / (paths.size()/100) << '%' << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Missing first argument(s), use one of the following options: " << std::endl;
        std::cout << " - [path to file of problem instance]. Results are written to standard output." << std::endl;
        std::cout << " - [path to directory] [output file]. Recursively problem instance files (from the directory) are found and run, and all results are written to the output file." << std::endl;
        exit(1);
    }

    if (argc <= 2) { // Only an input file
        std::cout << "File: " << argv[1] << std::endl << std::endl;
        std::ifstream inpFile(argv[1]);
        if (!inpFile) {
            std::cerr << "Can't open input file." << std::endl;
            exit(1);
        }
        Problem problem = Parser::parseProblemInstance(inpFile);
        inpFile.close();

        int *result = new int[problem.njobs];
        std::clock_t start = std::clock();
        bool found = PrSolver(problem).solve(result);
        std::clock_t end = std::clock();

        long milis = ((end - start) * 1000) / CLOCKS_PER_SEC;
        if (found) std::cout << "Makespan: " << result[problem.njobs - 1] << std::endl << std::endl;
        else std::cout << "Found no feasible solution." << std::endl;
        std::cout << "Took " << milis << " ms" << std::endl;
        if (found) std::cout << "Valid? " << checkValid(problem, result);

        delete[] result;
    }
    else { // Input data directory and output file
        std::cout << "Test data directory: " << argv[1] << std::endl;
        std::ofstream outFile(argv[2]);
        if (!outFile) {
            std::cerr << "Can't create or open output file." << std::endl;
            exit(1);
        }
        findInstancesAndSolveAll(argv[1], outFile);
        outFile.close();
        std::cout << std::endl;
        std::cout << "Results written to output file: " << argv[2] << std::endl;
    }

    return 0;
}
