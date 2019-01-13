#!/usr/bin/python3

from subprocess import Popen, PIPE
import os, sys, signal
from random import shuffle
from timeit import default_timer as timer
from math import log, floor
from time import sleep
from threading import Timer
import struct

EXECUTABLE_NAME = 'main'
EXECUTABLE_ARGV = '--new-skew-update'
TOTAL_THREAD = 48

def run_perf(test_num, main_pid, perf_duration):
    perf_name = os.path.dirname(os.path.realpath(__file__)) + "/skew" + str(test_num) + ".perf"
    print(perf_name)
    print("Running perf for %d secs" % (perf_duration))
    print("pid %d perf will be recorded on %s" % (main_pid, perf_name))
    # os.system("perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, main_pid))
    s = timer()
    perf = Popen(["sudo", "perf", "record", "--call-graph", "dwarf", "-o", perf_name, "-p", str(main_pid)], \
               cwd=os.path.dirname(os.path.realpath(__file__)), preexec_fn=os.setsid)
    print("Perf Started on %d" % perf.pid)
    TerminateTimer = Timer(perf_duration, os.killpg, [os.getpgid(perf.pid), signal.SIGINT])
    TerminateTimer.start()
    perf.wait()
    e = timer()
    duration = e - s
    print("perf ended after %.2f secs" % duration)

def test_case(total_thread_num):
    start = timer()
    runPerf = False
    if len(sys.argv) > 1:
        if sys.argv[1] == 'perf':
            print("Recording Perf Enabled")
            runPerf = True
    for i in range(0, total_thread_num + 1):
        # cmd = "./" + EXECUTABLE_NAME + (EXECUTABLE_ARGV % i)
        print("RUNNING SKEW TEST with %d threads" % i)
        with Popen(["./" + EXECUTABLE_NAME, EXECUTABLE_ARGV, str(i)], cwd=os.path.dirname(os.path.realpath(__file__)), stdin=PIPE, stdout=PIPE, bufsize=1, universal_newlines=True) as p:
            for line in p.stdout:
                print(line, end='') # process line here
                if runPerf and (line.strip() == 'New Skew Update Test Starts'):
                    start_delay = 5
                    perf_duration = 3
                    t = Timer(start_delay, run_perf, [i, p.pid, perf_duration])
                    t.start()
    end = timer()

    return end - start


elapse = test_case(TOTAL_THREAD)

print("%d Tests took %.2f sec of Elapsed Time" % (TOTAL_THREAD, elapse))
