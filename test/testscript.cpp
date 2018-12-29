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

void TestBwTreeUpdatePerformance(int key_num) {
  TreeType *t = GetEmptyTree(true);
  std::chrono::time_point<std::chrono::system_clock> start, end;

  start = std::chrono::system_clock::now();
  printf("~~~~~~~~test running~~~~~~\n");
  for(int i = 0;i < key_num;i++) {
    t->Insert(i, i);
  }

  end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;

  std::cout << "BwTree: " << (key_num / (1024.0 * 1024.0)) / elapsed_seconds.count()
            << " million insertion/sec" << "\n";

  // Then test read performance
  int iter = 10;
  std::vector<long> v{};

  v.reserve(100);

  start = std::chrono::system_clock::now();

  for(int j = 0;j < iter;j++) {
    for(int i = 0;i < key_num;i++) {
      t->GetValue(i, v);

      v.clear();
    }
  }

  end = std::chrono::system_clock::now();

  elapsed_seconds = end - start;
  std::cout << "BwTree: " << (iter * key_num / (1024.0 * 1024.0)) / elapsed_seconds.count()
            << " million read/sec" << "\n";

  ///////////////////////////////////////////////////////////////////
  // Update Inserted Key-Value Pair
  ///////////////////////////////////////////////////////////////////

  bool succeed;
  start = std::chrono::system_clock::now();

  for(int i = key_num - 1;i >= 0;i--) {
    succeed = t->Update(i, i, i + 1);
  }

  end = std::chrono::system_clock::now();

  elapsed_seconds = end - start;

  std::cout << "BwTree: " << (key_num / (1024.0 * 1024.0)) / elapsed_seconds.count()
            << " million Update (reverse order)/sec" << "\n";

  // Read again
  start = std::chrono::system_clock::now();

  for(int j = 0;j < iter;j++) {
    for(int i = 0;i < key_num;i++) {
      t->GetValue(i, v);
      auto it = std::find(v.begin(), v.end(), i+1);
      v.clear();
    }
  }

  end = std::chrono::system_clock::now();

  elapsed_seconds = end - start;
  std::cout << "BwTree: " << (iter * key_num / (1024.0 * 1024.0)) / elapsed_seconds.count()
            << " million updated read (2 values)/sec" << "\n";

  // Verify updates
  for(int i = 0;i < key_num;i++) {
    t->GetValue(i, v);

    assert(v.size() == 1);
    assert(v[0] == i + 1);

    v.clear();
  }

  std::cout << "    All values are correct!\n";

  // Finally remove values

  start = std::chrono::system_clock::now();

  for(int i = 0;i < key_num;i++) {
    t->Delete(i, i + 1);
  }
  end = std::chrono::system_clock::now();

  elapsed_seconds = end - start;
  std::cout << "BwTree: " << (key_num / (1024.0 * 1024.0)) / elapsed_seconds.count()
            << " million remove/sec" << "\n";

  for(int i = 0;i < key_num;i++) {
    t->GetValue(i, v);

    assert(v.size() == 0);
  }

  std::cout << "    All values have been removed!\n";

  return;
}
