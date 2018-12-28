
#include "test_suite.h"

/*
 * GetThreadNum() - Returns the number of threads used for multithreaded testing
 *
 * By default 40 threads are used
 */
uint64_t GetThreadNum() {
    uint64_t thread_num = 40;
    bool ret = Envp::GetValueAsUL("THREAD_NUM", &thread_num);
    if(ret == false) {
      throw "THREAD_NUM must be an unsigned ineteger!";
    } else {
      printf("Using thread_num = %lu\n", thread_num);
    }

    return thread_num;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("usage\n");
    printf("[fileName] [number of instructions] [number of threads]\n");
  }
  std::string instruction_num(argv[1]);
  std::string thread_num(argv[2]);

  // BenchmarkRandOperation(int total_operation, int thread_num, int insert_ratio, int delete_ratio)
  BenchmarkRandOperation(std::stoi(instruction_num), std::stoi(thread_num));
}

