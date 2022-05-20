/***************************************************************************************[Solver.cc]
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
#include <queue>
#include <random>
#include <limits>

#include "Solver.h"

#define NPASSES 1000
#define TOURN_FACTOR 0.5
#define OMEGA1 0.4
#define OMEGA2 0.6

using namespace RcpsptHeuristic;

Solver::Solver(Problem &p)
    : problem(p) {}

Solver::~Solver() = default;

bool Solver::solve(int* out) {
    std::cerr << "Calling solve(int* out) is not supported on abstract base class Solver" << std::endl;

    return false;
}

bool PrSolver::solve(int* out) {
    // This function is completely based on the tournament heuristic that is described by Hartmann (2013) (reference in README.md)

    std::queue<int> q; // Use a queue for breadth-first traversal of the precedence graph

    // Calculate earliest feasible finish times, using the definition from Hartmann (2013) (reference in README.md)

    int* ef = new int[problem.njobs](); // Earliest feasible finish times for all jobs

    // Enqueue job 0
    q.push(0);

    while (!q.empty()) {
        int job = q.front();
        q.pop();
        int duration = problem.durations[job];

        // Move finish until it is feasible considering resource constraints
        bool feasibleFinal = false;
        while (!feasibleFinal) {
            bool feasible = true;
            for (int k = 0; feasible && k < problem.nresources; k++) {
                for (int t = duration - 1; feasible && t >= 0; t--) {
                    if (problem.requests[job][k][t] > problem.capacities[k][ef[job] - duration + t]) {
                        feasible = false;
                        ef[job]++;
                    }
                }
            }
            if (feasible) feasibleFinal = true;
            if (ef[job] > problem.horizon) {
                delete[] ef;
                return false;
            }
        }

        // Update finish times, and enqueue successors
        for (int i = 0; i < problem.nsuccessors[job]; i++) {
            int successor = problem.successors[job][i];
            int f = ef[job] + problem.durations[successor];
            if (f > ef[successor]) ef[successor] = f; // Use maximum values, because we are interested in critical paths
            q.push(successor); // Enqueue successor
        }
    }

    // Calculate latest feasible start times, again using the definition from Hartmann (2013) (reference in README.md)

    int* ls = new int[problem.njobs];
    for (int i = 0; i < problem.njobs; i++) ls[i] = problem.horizon;

    // Enqueue the sink job
    q.push(problem.njobs - 1);

    while (!q.empty()) {
        int job = q.front();
        q.pop();
        int duration = problem.durations[job];

        // Move start until it is feasible considering resource constraints
        bool feasibleFinal = false;
        while (!feasibleFinal) {
            bool feasible = true;
            for (int k = 0; feasible && k < problem.nresources; k++) {
                for (int t = 0; feasible && t < duration; t++) {
                    if (problem.requests[job][k][t] > problem.capacities[k][ls[job] + t]) {
                        feasible = false;
                        ls[job]--;
                    }
                }
            }
            if (feasible) feasibleFinal = true;
            if (ls[job] < 0) {
                delete[] ef;
                delete[] ls;
                return false;
            }
        }

        // Update start times, and enqueue predecessors
        for (int predecessor : problem.predecessors[job]) {
            int s = ls[job] - problem.durations[predecessor];
            if (s < ls[predecessor]) ls[predecessor] = s; // Use minimum values for determining critical paths
            q.push(predecessor); // Enqueue predecessor
        }
    }

    // Calculate extended resource utilization values, using the definition from Hartmann (2013) (reference in README.md)

    double* ru = new double[problem.njobs];

    // Enqueue the sink job
    q.push(problem.njobs - 1);

    while (!q.empty()) {
        int job = q.front();
        q.pop();
        int duration = problem.durations[job];

        int demand = 0, availability = 0;
        for (int k = 0; k < problem.nresources; k++) {
            for (int t = 0; t < duration; t++)
                demand += problem.requests[job][k][t];
            for (int t = ef[job] - duration; t < ls[job] + duration; t++) // Use the time window from earliest start to latest finish
                availability += problem.capacities[k][t];
        }


        ru[job] = OMEGA1 * (((double) problem.nsuccessors[job] / (double) problem.nresources) *
                            ((double) demand / (double) availability));
        for (int successor = 0; successor < problem.nsuccessors[job]; successor++)
            ru[job] += OMEGA2 * ru[successor];
        if (std::isnan(ru[job]) || ru[job] < 0.0) ru[job] = 0.0; // Prevent errors from strange values here

        // Enqueue predecessors
        for (int predecessor : problem.predecessors[job])
            q.push(predecessor);
    }

    // Calculate the CPRU (critical path and resource utilization) priority value for each activity, using the definition from Hartmann (2013) (reference in README.md)
    double* cpru = new double[problem.njobs];
    for (int job = 0; job < problem.njobs; job++) {
        int cp = problem.horizon - ls[job]; // Critical path length
        cpru[job] = cp * ru[job];
    }

    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<double> distribution(0, 1);

    // Run a set number of passes ('tournaments'), as described by Hartmann (2013) (reference in README.md)

    int** available = new int*[problem.nresources];
    for (int k = 0; k < problem.nresources; k++)
        available[k] = new int[problem.horizon];
    int* eligible = new int[problem.njobs];
    int* selected = new int[problem.njobs];
    int neligible, nselected;
    int* schedule = new int[problem.njobs];
    int bestMakespan = INT32_MAX / 2;
    for (int pass = 0; pass < NPASSES; pass++) {
        for (int i = 0; i < problem.njobs; i++) schedule[i] = -1;

        // Initialize remaining resource availabilities
        for (int k = 0; k < problem.nresources; k++)
            for (int t = 0; t < problem.horizon; t++)
                available[k][t] = problem.capacities[k][t];

        // Schedule the starting dummy activity
        schedule[0] = 0;

        // Schedule all remaining jobs
        for (int i = 1; i < problem.njobs; i++) {
            // Randomly select a fraction of the eligible activities (with replacement)
            neligible = 0;
            for (int j = 1; j < problem.njobs; j++) {
                if (schedule[j] < 0) {
                    bool predecessorsScheduled = true;
                    for (int predecessor : problem.predecessors[j])
                        if (schedule[predecessor] < 0) predecessorsScheduled = false;
                    if (predecessorsScheduled) eligible[neligible++] = j;
                }
            }
            int Z = std::max((int)(TOURN_FACTOR * neligible), 2);
            nselected = 0;
            for (int j = 0; j < Z; j++) {
                int choice = (int)(distribution(eng) * neligible);
                selected[nselected++] = eligible[choice];
            }

            // Select the activity with the best priority value
            int winner = -1;
            double bestPriority = -std::numeric_limits<double>::max()/2.0;
            for (int j = 0; j < nselected; j++) {
                int sjob = selected[j];
                if (cpru[sjob] >= bestPriority) {
                    bestPriority = cpru[sjob];
                    winner = sjob;
                }
            }

            // Schedule it as early as possible
            int finish = -1;
            for (int predecessor : problem.predecessors[winner]) {
                int newFinish = schedule[predecessor] + problem.durations[winner];
                if (newFinish > finish) finish = newFinish;
            }
            int duration = problem.durations[winner];
            bool feasibleFinal = false;
            while (!feasibleFinal) {
                bool feasible = true;
                for (int k = 0; feasible && k < problem.nresources; k++) {
                    for (int t = duration - 1; feasible && t >= 0; t--) {
                        if (problem.requests[winner][k][t] > available[k][finish - duration + t]) {
                            feasible = false;
                            finish++;
                        }
                    }
                }
                if (feasible) feasibleFinal = true;
                if (finish > problem.horizon) {
                    delete[] ef;
                    delete[] ls;
                    delete[] ru;
                    delete[] cpru;
                    for (int x = 0; x < problem.nresources; x++) delete[] available[x];
                    delete[] available;
                    delete[] eligible;
                    delete[] selected;
                    delete[] schedule;
                    return false;
                }
            }
            schedule[winner] = finish;

            // Update remaining resource availabilities
            for (int k = 0; k < problem.nresources; k++) {
                for (int t = 0; t < duration; t++)
                    available[k][finish - duration + t] -= problem.requests[winner][k][t];
            }
        }

        if (schedule[problem.njobs - 1] < bestMakespan) {
            bestMakespan = schedule[problem.njobs - 1];
            for (int i = 0; i < problem.njobs; i++) out[i] = schedule[i];
        }
    }

    delete[] ef;
    delete[] ls;
    delete[] ru;
    delete[] cpru;
    for (int x = 0; x < problem.nresources; x++) delete[] available[x];
    delete[] available;
    delete[] eligible;
    delete[] selected;
    delete[] schedule;
    return bestMakespan <= problem.horizon;
}

bool GaSolver::solve(int *out) {
    // TODO: implement
    return false;
}
