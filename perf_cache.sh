#!/bin/bash

TEST_COMMAND='perf stat'
TEST_OPTION='-B -d -d -d -e cache-references,cache-misses,LLC-load-misses,LLC-loads,LLC-prefetch-misses,LLC-prefetches,LLC-store-misses,LLC-stores -p'
TEST_MAIN='./main --new-skew-update'
TEST_RESULT=$HOME'/BwTree/result_cache.txt'
ITER_NUM=44
PERF_RESULT=$HOME'/BwTree/cache_perf'

for i in $(seq 0 $ITER_NUM)
do
	Q=`expr $i % 4`
	if [ $Q -ne 0 ]
	then
		continue
	fi
	echo "Test $i/$ITER_NUM skew_update threads"
	$TEST_MAIN $i >> $TEST_RESULT &
	export MAIN_PID=$!
	sleep 36s
#	MAIN_PID=`ps -ef --sort=-pcpu | grep 'main' | awk '{print $2}'| head -1`
	$TEST_COMMAND -o $PERF_RESULT/$i.result.txt $TEST_OPTION $MAIN_PID &
	export PERF_PID=$!
	echo $PERF_PID
	echo `ps -ef | grep 'perf' | awk '{print $2}'|head -2|tail -1`
#	wait $MAIN_PID
	sleep 5s
	if [[ "" !=  "$PERF_PID" ]]; then
	  kill -2 $PERF_PID
	fi
done

echo "Test Done"


