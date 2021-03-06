#include "test_suite.h"
#include <unistd.h>

using namespace std;

int skew_test_thread_num = 44;
int skew_test_max_key = 40000000;

void MakeBasicTree(TreeType *t){
  for(int i = 0; i < skew_test_max_key; i++){
    t->Insert(i, i);
  }
  return;
}

void UniformTest(uint64_t thread_id, TreeType *t){
  std::random_device r{};
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(0, skew_test_max_key - 1);

  std::chrono::time_point<std::chrono::system_clock> start, end;

  start = std::chrono::system_clock::now();

  for(size_t i = 0;i < ((size_t)skew_test_max_key/skew_test_thread_num);i++) {
    int key = uniform_dist(e1);

    t->Update(key, key, key);
  }

  end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;

  // Measure the overhead
  std::vector<long int> v{};
  v.reserve(100);

  start = std::chrono::system_clock::now();

  for(size_t i = 0;i < ((size_t)skew_test_max_key/skew_test_thread_num);i++) {
    int key = uniform_dist(e1);

    v.push_back(key);

    v.clear();
  }

  end = std::chrono::system_clock::now();

  std::chrono::duration<double> overhead = end - start;

  std::cout << "BwTree: at least " << ((skew_test_max_key/skew_test_thread_num)/(1024*1024)) / (elapsed_seconds.count())
            << " random update/sec" << "\n";

  std::cout << "    Overhead = " << overhead.count() << " seconds" << std::endl;
  return;
}

void SkewTest(TreeType *t) {
  const int num_thread = skew_test_thread_num;
  const int key_num = skew_test_max_key;
  int iter = 1;

  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }

  // Generate zipfian distribution into this list
  std::vector<long> zipfian_key_list{};
  zipfian_key_list.reserve(key_num);

  // Initialize it with time() as the random seed
  Zipfian zipf{(uint64_t)key_num, 0.99, (uint64_t)time(NULL)};

  // Populate the array with random numbers
  for(int i = 0;i < key_num;i++) {
    zipfian_key_list.push_back(zipf.Get());
  }

  auto func2 = [key_num,
                iter,
                &thread_time,
                &zipfian_key_list,
                num_thread](uint64_t thread_id, TreeType *t) {
    // This is the start and end index we read into the zipfian array
    long int start_index = key_num / num_thread * (long)thread_id;
    long int end_index = start_index + key_num / num_thread;

    std::vector<long> v{};

    v.reserve(1);

    Timer timer{true};
    CacheMeter cache{true};

    for(int j = 0;j < iter;j++) {
      for(long i = start_index;i < end_index;i++) {
        long int key = zipfian_key_list[i];

        t->Update(key, key, key);

        v.clear();
      }
    }

    cache.Stop();
    double duration = timer.Stop();

    thread_time[thread_id] = duration;

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * (end_index - start_index) / (1024.0 * 1024.0)) / duration \
              << " million update (zipfian)/sec" << "\n";

    cache.PrintL3CacheUtilization();
    cache.PrintL1CacheUtilization();

    return;
  };

  LaunchParallelTestID(t, num_thread, func2, t);

  double cpu_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BwTree: overall "
            << (iter * key_num / (1024.0 * 1024.0)) / (cpu_seconds / num_thread)
            << " million update (zipfian)/sec" << "\n";
  std::cout << "Total CPU Time: " << cpu_seconds << "\n";
  return;
}

void DistributeUpdateTest(TreeType *t, int start_index, int end_index) {
  const int num_thread = skew_test_thread_num;
  const int key_num = skew_test_max_key;
  const int iter = key_num / num_thread;

  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  int failed_cnt[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
    failed_cnt[i] = 0;
  }

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();

  auto func2 = [key_num,
                &thread_time,
		&failed_cnt,
                start_index,
                end_index,
                iter,
                num_thread](uint64_t thread_id, TreeType *t) {
    std::random_device r{};
    std::default_random_engine e1(r());
    std::uniform_int_distribution<long int> uniform_dist(start_index, end_index - 1);

    Timer timer{true};
    CacheMeter cache{true};
    int failed = 0;
    for(long i = 0; i < iter; i++) {
      long int key = uniform_dist(e1);

      bool ret = t->Update(key, key, key);
      if (!ret)
        failed += 1;
    }

    cache.Stop();
    double duration = timer.Stop();

    thread_time[thread_id] = duration;
    failed_cnt[thread_id] = failed;

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << iter / duration \
              << " update/sec in " << duration << " seconds" << "\n";

    cache.PrintL3CacheUtilization();
    cache.PrintL1CacheUtilization();

    return;
  };

  LaunchParallelTestID(t, num_thread, func2, t);
  end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;
  double cpu_seconds = 0.0;
  int total_failed = 0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
    total_failed += failed_cnt[i];
  }

  std::cout << num_thread << " Threads BwTree: overall " \
            << key_num / (cpu_seconds / num_thread) \
            << " update/sec with " << total_failed << " failed operations\n";
  std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds\n";
  return;
}
void CleanCache() {
     const int size = 20*1024*1024; // Allocate 20M. Set much larger then L2
     char *c = (char *)malloc(size);
     for (int i = 0; i < 0xffff; i++)
       for (int j = 0; j < size; j++)
         c[j] = i*j;
}

void DistributeUpdateTest2(TreeType *t, int start_index, int end_index, int skew_threads) {
/*
  Temporary code below
 */
//   const int num_thread = skew_test_thread_num;
//   const int key_num = skew_test_max_key;
//   const int iter = 2000000;

//   if (skew_threads > num_thread) {
//     std::cout << "skew_threads argv out of range\n";
//     std::cout << "skew_threads: " << skew_threads << ", " \
//               << "total_threads: " << num_thread << endl;

//     return;
//   }

//   // This is used to record time taken for each individual thread
//   double thread_time[num_thread];
//   int failed_cnt[num_thread];
//   for(int i = 0;i < num_thread;i++) {
//     thread_time[i] = 0.0;
//     failed_cnt[i] = 0;
//   }

//   std::chrono::time_point<std::chrono::system_clock> start, end;
// //  CleanCache();
//   printf("cache cleaned\n");
//   fflush(stdout);
//   sleep(1);
//   start = std::chrono::system_clock::now();
//   auto func2 = [key_num,
//                 &thread_time,
// 		&failed_cnt,
//                 start_index,
//                 end_index,
//                 skew_threads,
//                 iter,
//                 num_thread](uint64_t thread_id, TreeType *t) {
//     std::random_device r{};
//     std::default_random_engine e1(r());
//     std::uniform_int_distribution<long int> uniform_dist(0, key_num - 1);
//     bool isSkew = (int)thread_id < skew_threads;
//     if (isSkew)
//       uniform_dist = std::uniform_int_distribution<long int>(start_index, end_index - 1);

//     Timer timer{true};
//     int failed = 0;
//     for(long i = 0; i < iter; i++) {
//       long int key = uniform_dist(e1);

//       bool ret = t->Update(key, key, key);
//       if (!ret)
//         failed += 1;
//     }

//     double duration = timer.Stop();

//     thread_time[thread_id] = duration;
//     failed_cnt[thread_id] = failed;

//     std::cout << "[" << (isSkew ? "Skew   ": "Uniform") << " Thread " \
//               << std::setw(2) << thread_id << " Done] @ " \
//               << std::fixed << std::setprecision(1) << std::setw(6) << iter / duration << " update/sec in " \
//               << std::setprecision(5) << std::setw(8) <<  duration << " seconds" << endl;

//     return;
  // };

  // LaunchParallelTestID(t, num_thread, func2, t);
  // end = std::chrono::system_clock::now();

  // std::chrono::duration<double> elapsed_seconds = end - start;
  // double cpu_seconds = 0.0;
  // int total_failed = 0;
  // for(int i = 0;i < num_thread;i++) {
  //   cpu_seconds += thread_time[i];
  //   total_failed += failed_cnt[i];
  // }

  // std::cout << num_thread << " Threads BwTree: overall " \
  //           << key_num / (cpu_seconds / num_thread) \
  //           << " update/sec with " << total_failed << " failed operations\n";
  // std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  // std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds" << endl;

  // std::cout << "Skew Test with " << skew_threads << " threads," \
  //           << "Uniform Test with " << num_thread - skew_threads << " threads." << endl;
  // return;
  
  const int num_thread = skew_test_thread_num;
  const int key_num = skew_test_max_key;
  const int iter = 2000000;
  std::cout << "No Duplicate" << endl; 
  double thread_time[num_thread];
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  auto noDup = [key_num,
                &thread_time,
                start_index,
                end_index,
                skew_threads,
                iter,
                num_thread](uint64_t thread_id, TreeType *t){
                  std::random_device r{};
                  std::default_random_engine e1(r());
                  int each_range = key_num/num_thread;
                  std::uniform_int_distribution<long int> uniform_dist(thread_id*each_range, (thread_id+1)*each_range);
                  
                  Timer timer{true};
                  for(long i = 0; i < iter; i++) {
                    long int key = uniform_dist(e1);
                    bool ret = t->Update(key, key, key);

                  }
                  double duration = timer.Stop();
                  thread_time[thread_id] = duration;
                  return;                  
                };
  LaunchParallelTestID(t, num_thread, noDup, t);
  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  double cpu_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
  }
  std::cout << num_thread << " Threads BwTree: overall " \
            << key_num / (cpu_seconds / num_thread);
  std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds" << endl;

  std::cout << "OneKey" << endl;
  start = std::chrono::system_clock::now();
  auto oneKey = [key_num,
                &thread_time,
                start_index,
                end_index,
                skew_threads,
                iter,
                num_thread](uint64_t thread_id, TreeType *t){
                  
                  Timer timer{true};
                  for(long i = 0; i < iter; i++) {
                    bool ret = t->Update(0, 0, 0);

                  }
                  double duration = timer.Stop();
                  thread_time[thread_id] = duration;
                  return;                  
                };
  LaunchParallelTestID(t, num_thread, oneKey, t);
  end = std::chrono::system_clock::now();
  elapsed_seconds = end - start;
  cpu_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
  }
  std::cout << num_thread << " Threads BwTree: overall " \
            << key_num / (cpu_seconds / num_thread);
  std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds" << endl;

  std::cout << "Half and Half" << endl;
  start = std::chrono::system_clock::now();
  auto halfHalf = [key_num,
                &thread_time,
                start_index,
                end_index,
                skew_threads,
                iter,
                num_thread](uint64_t thread_id, TreeType *t){
                  bool isSkew = (int)thread_id < (num_thread/2);
                  std::random_device r{};
                  std::default_random_engine e1(r());
                  std::uniform_int_distribution<long int> uniform_dist(0, key_num - 1);
                  
                  if (isSkew)
                    uniform_dist = std::uniform_int_distribution<long int>(0, 2048);

                  Timer timer{true};
                  for(long i = 0; i < iter; i++) {
                    long int key = uniform_dist(e1);

                    bool ret = t->Update(key, key, key);
                  }
                  double duration = timer.Stop();
                  thread_time[thread_id] = duration;
                  return;                  
                };
  LaunchParallelTestID(t, num_thread, halfHalf, t);
  end = std::chrono::system_clock::now();
  elapsed_seconds = end - start;
  cpu_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
  }
  std::cout << num_thread << " Threads BwTree: overall " \
            << key_num / (cpu_seconds / num_thread);
  std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds" << endl;
  return;
}

void ZipfianSkewTest(TreeType *t, int start_index, int end_index, int skew_threads) {
  const int num_thread = skew_test_thread_num;
  const int key_num = skew_test_max_key;
  const int iter = 2000000;

  if (skew_threads > num_thread) {
    std::cout << "skew_threads argv out of range\n";
    std::cout << "skew_threads: " << skew_threads << ", " \
              << "total_threads: " << num_thread << endl;

    return;
  }

  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  int failed_cnt[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
    failed_cnt[i] = 0;
  }

  std::chrono::time_point<std::chrono::system_clock> start, end;
//  CleanCache();
  std::vector<long> zipfian_key_list{};
  zipfian_key_list.reserve(key_num);

  // Initialize it with time() as the random seed
  Zipfian zipf{(uint64_t)key_num, 0.99, (uint64_t)time(NULL)};

  // Populate the array with random numbers
  for(int i = 0;i < key_num;i++) {
    zipfian_key_list.push_back(zipf.Get());
  }
  printf("cache cleaned\n");
  fflush(stdout);
  sleep(1);
  start = std::chrono::system_clock::now();
  auto func2 = [key_num,
                &thread_time,
                &failed_cnt,
                start_index,
                end_index,
                skew_threads,
                iter,
                num_thread,
                &zipfian_key_list](uint64_t thread_id, TreeType *t) {
    std::random_device r{};
    std::default_random_engine e1(r());
    std::uniform_int_distribution<long int> uniform_dist(0, key_num - 1);
    bool isSkew = (int)thread_id < skew_threads;
    Timer timer{true};
    int failed = 0;
    for(long i = 0; i < iter; i++) {
      long int key;
      if(isSkew){
        key = zipfian_key_list[uniform_dist(e1)];
      }else{
        key = uniform_dist(e1);
      }
      bool ret = t->Update(key, key, key);
      if (!ret)
        failed += 1;
    }

    double duration = timer.Stop();

    thread_time[thread_id] = duration;
    failed_cnt[thread_id] = failed;

    std::cout << "[" << (isSkew ? "Skew   ": "Uniform") << " Thread " \
              << std::setw(2) << thread_id << " Done] @ " \
              << std::fixed << std::setprecision(1) << std::setw(6) << iter / duration << " update/sec in " \
              << std::setprecision(5) << std::setw(8) <<  duration << " seconds" << endl;

    return;
  };

  LaunchParallelTestID(t, num_thread, func2, t);
  end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;
  double cpu_seconds = 0.0;
  int total_failed = 0;
  for(int i = 0;i < num_thread;i++) {
    cpu_seconds += thread_time[i];
    total_failed += failed_cnt[i];
  }

  std::cout << num_thread << " Threads BwTree: overall " \
            << key_num / (cpu_seconds / num_thread) \
            << " update/sec with " << total_failed << " failed operations\n";
  std::cout << "Total CPU Time: " << cpu_seconds << " seconds\n";
  std::cout << "Total Elapsed Time: " << elapsed_seconds.count() << " seconds" << endl;

  std::cout << "Skew Test with " << skew_threads << " threads," \
            << "Uniform Test with " << num_thread - skew_threads << " threads." << endl;
  return;
}

