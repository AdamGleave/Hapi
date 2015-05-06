#!/bin/bash

echo "PID = $$"
./distributed_benchmark.py /tmp/redo_$$ $* > ~/adam/tmp/benchmark_log_$$
