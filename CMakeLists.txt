cmake_minimum_required(VERSION 3.16)
project(rcpspt_heuristic)

set(CMAKE_CXX_STANDARD 17)

add_executable(rcpspt_heuristic src/Main.cc src/Solver.cc src/Problem.cc src/Parser.cc)
