#include "test_suite.h"

/*
 * Run Benchmark with given insert/delete ratio.
 * operations will be mixed up to simulate random.
 * Assume that insert_ratio + delete_ration == 10
*/
void BenchmarkRandOperation(int total_operation, int thread_num) {
  // Get an empty tree; do not print its construction message
  TreeType *t = GetEmptyTree(true);

  // This is used to record time taken for each individual thread
  double thread_time[thread_num];
  for(int i = 0; i < thread_num; i++) {
    thread_time[i] = 0.0;
  }

  auto func = [&thread_time,
               thread_num](uint64_t thread_id, TreeType *t) {
    std::string fName = "tmp" + std::to_string(thread_id) + ".txt";
    std::ifstream fin(fName);
    std::string operation;
    long int key, prev_key;
    long int ins_cnt = 0, del_cnt = 0, failed_del_cnt = 0;
    std::vector<long int> tmpVec;
    // Declare timer and start it immediately
    Timer timer{true};

    CacheMeter cache{true};

    while(fin >> operation >> key) {
      if (operation == "i") {
        ins_cnt++;
        t->Insert(key, key);
      } else if (operation == "d") {
        del_cnt++;
        bool result = t->Delete(key, key);
        if (!result)
          failed_del_cnt++;
      }
    }

    cache.Stop();
    double duration = timer.Stop();

    thread_time[thread_id] = duration;

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (ins_cnt + del_cnt) / duration \
              << " million random operation/sec" << "\n" << failed_del_cnt << " failed delete" << "\n" \
              << ins_cnt << " insertion & " << del_cnt << " deletion in " << duration << " seconds\n";

    // Print L3 total accesses and cache misses
    cache.PrintL3CacheUtilization();
    cache.PrintL1CacheUtilization();

    return;
  };

  ParallelTest(t, thread_num, func, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < thread_num;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << thread_num << " Threads BwTree: overall "
            << total_operation / elapsed_seconds
            << " million random insert & delete / sec" << "\n";

  // Remove the tree instance
  delete t;

  return;
}
