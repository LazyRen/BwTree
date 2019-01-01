#include "test_suite.h"
using namespace std;

int skew_test_thread_num = 8;
int skew_test_max_key = 1000000;

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
    
    t->Update(key, key);
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

        t->Update(key, i);

        v.clear();
      }
    }

    cache.Stop();
    double duration = timer.Stop();
    
    thread_time[thread_id] = duration;

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * (end_index - start_index) / (1024.0 * 1024.0)) / duration \
              << " million read (zipfian)/sec" << "\n";
    
    cache.PrintL3CacheUtilization();
    cache.PrintL1CacheUtilization();

    return;
  };

  LaunchParallelTestID(t, num_thread, func2, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BwTree: overall "
            << (iter * key_num / (1024.0 * 1024.0)) / (elapsed_seconds / num_thread)
            << " million read (zipfian)/sec" << "\n";

  return;
}