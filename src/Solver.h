/****************************************************************************************[Solver.h]
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

#ifndef RCPSPT_HEURISTIC_SOLVER_H
#define RCPSPT_HEURISTIC_SOLVER_H

#include "Problem.h"

namespace RcpsptHeuristic {

/**
 * Abstract base class for a solver for the RCPSP/t.
 */
class Solver {
public:
    // Constructor
    Solver(Problem& p);
    // Destructor
    virtual ~Solver();

    // Solver interface

    /**
     * Calculates a solution for the problem.
     *
     * @param out vector to which end times for all activities will be written
     * @param infeasible indicates whether the preprocessing steps found the instance to be infeasible
     * @return true if a solution was found, false otherwise
     */
    virtual bool solve(int* out, bool* infeasible);

protected:
    Problem& problem;
};

/**
 * Tournament-based solver, using priority rule heuristic.
 */
class PrSolver : public Solver {
public:
    explicit PrSolver(Problem& p)
        : Solver(p) {}

    bool solve(int* out, bool* infeasible);
};

/**
 * Genetic algorithm solver.
 */
class GaSolver : public Solver {
public:
    explicit GaSolver(Problem& p)
        : Solver(p) {}

    bool solve(int* out);
};
}

#endif //RCPSPT_HEURISTIC_SOLVER_H
