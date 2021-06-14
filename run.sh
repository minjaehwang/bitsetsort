#!/bin/bash

# Run quicksort based algorithms only.
#
# If you want to produce a .csv file like in results/*.csv, do the following
#
# BENCHMARK_FORMAT=json ./run.sh 
build/sortbench --benchmark_filter="BM_Sort_|BM_Bitset|BM_Pdq"
