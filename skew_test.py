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
RECORD_COMMAND='perf record'
RECORD_OPTION='--call-graph dwarf'
STAT_COMMAND='perf stat'
STAT_OPTION= '-B -d -d -d -e cache-references,cache-misses,LLC-load-misses,LLC-loads,LLC-prefetch-misses,LLC-prefetches,LLC-store-misses,LLC-stores'
TOTAL_THREAD = 48
SKIP_THREAD = 4

def run_perf(testNum, mainPID, perfDuration):
    resultFile = os.path.dirname(os.path.realpath(__file__)) + "/skew" + str(testNum) + ".perf"
    print("Running perf record for %d secs" % (perfDuration))
    print("pid %d perf will be recorded on %s" % (mainPID, resultFile))
    # os.system("perf record --call-graph dwarf -o skew%d.perf -p %d" % (i, mainPID))
    args = RECORD_COMMAND.split() + RECORD_OPTION.split() + ["-p", str(mainPID)] + ["-o", resultFile]
    s = timer()
    perf = Popen(args, cwd=os.path.dirname(os.path.realpath(__file__)), preexec_fn=os.setsid)
    print("Perf started on %d" % perf.pid)
    TerminateTimer = Timer(perfDuration, os.killpg, [os.getpgid(perf.pid), signal.SIGINT])
    TerminateTimer.start()
    perf.wait()
    e = timer()
    duration = e - s
    print("perf ended after %.2f secs" % duration)

def run_stat(testNum, mainPID, perfDuration):
    resultFile = os.path.dirname(os.path.realpath(__file__)) + "/skew" + str(testNum) + "cache.txt"
    print("Running perf stat for %d secs" % (perfDuration))
    print("pid %d stat will be recorded on %s" % (mainPID, resultFile))
    args = STAT_COMMAND.split() + STAT_OPTION.split() + ["-p", str(mainPID)] + ["-o", resultFile]
    s = timer()
    perf = Popen(args, cwd=os.path.dirname(os.path.realpath(__file__)), preexec_fn=os.setpgrp)
    print("Perf Stat started on %d" % perf.pid)
    TerminateTimer = Timer(perfDuration, os.killpg, [os.getpgid(perf.pid), signal.SIGINT])
    TerminateTimer.start()
    perf.wait()
    e = timer()
    duration = e - s
    print("perf Stat ended after %.2f secs" % duration)

def test_case(total_thread_num):
    runPerf = False
    runStat = False
    start_delay = 10
    perf_duration = 5
    for i in range(1, len(sys.argv)):
        if sys.argv[i] == 'perf':
            print("Recording Perf Enabled")
            runPerf = True
        if sys.argv[i] == 'cache':
            print("Analyze Cache using Perf Stat")
            runStat = True
    # if (runPerf and runStat):
    #     print("You can only activate one perf option at a time")
    #     return 0

    start = timer()
#    for i in range(0, total_thread_num + 1, SKIP_THREAD):
    for i in range(0, TOTAL_THREAD + 1, SKIP_THREAD):
        # cmd = "./" + EXECUTABLE_NAME + (EXECUTABLE_ARGV % i)
        print("RUNNING SKEW TEST with %d threads" % i)
        with Popen(["./" + EXECUTABLE_NAME, EXECUTABLE_ARGV, str(i)], cwd=os.path.dirname(os.path.realpath(__file__)), stdin=PIPE, stdout=PIPE, bufsize=1, universal_newlines=True) as p:
            for line in p.stdout:
                print(line, end='') # process line here
                if runPerf and (line.strip() == 'New Skew Update Test Starts'):
                    t = Timer(start_delay, run_perf, [i, p.pid, perf_duration])
                    t.start()
                if runStat and (line.strip() == 'New Skew Update Test Starts'):
                    t = Timer(start_delay, run_stat, [i, p.pid, perf_duration])
                    t.start()
    end = timer()

    return end - start


elapse = test_case(TOTAL_THREAD)

print("%d Tests took %.2f sec of Elapsed Time" % (TOTAL_THREAD / SKIP_THREAD, elapse))
