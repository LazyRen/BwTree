#!/usr/bin/python3

from subprocess import Popen, PIPE
import os
from random import shuffle
from timeit import default_timer as timer
from math import log, floor
from time import sleep
import struct

EXECUTABLE_NAME = 'main'
EXECUTABLE_ARGV = '--new-skew-update'

def test_case(total_thread_num):
    start = timer()
    for i in range(1, total_thread_num + 1):
        # cmd = "./" + EXECUTABLE_NAME + (EXECUTABLE_ARGV % i)
        print("RUNNING SKEW TEST WITH %d threads" % i)
        with Popen(["./" + EXECUTABLE_NAME, EXECUTABLE_ARGV, str(i)], cwd=os.path.dirname(os.path.realpath(__file__)), stdin=PIPE, stdout=PIPE, bufsize=1, universal_newlines=True) as p:
            for line in p.stdout:
                print(line, end='') # process line here
                if line.strip() == 'New Skew Update Test Starts':
                    print("sudo perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, p.pid))
                    os.system("sudo perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, p.pid))
        # p = Popen(["./" + EXECUTABLE_NAME, EXECUTABLE_ARGV, str(i)], cwd=os.path.dirname(os.path.realpath(__file__)), stdin=PIPE, stdout=PIPE, bufsize=1, universal_newlines=True)
        # while True:
        #     output = p.stdout.readline().decode('utf-8')
        #     print(output)
        #     if output == 'New Skew Update Test Starts':
        #         break
        # print("sudo perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, p.pid))
        # os.system("sudo perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, p.pid))
        # while True:
        #     output = p.stdout.readline().decode('utf-8')
        #     print(output)
    end = timer()

    return end - start


elapse = test_case(8)

print("Total Elapsed Time: %.2f sec" % (elapse))
