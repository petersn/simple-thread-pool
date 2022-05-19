
#define SIMPLE_THREAD_POOL_IMPLEMENTATION
#include "simple_thread_pool.h"

void function(const int* inputs, int* outputs) {
  int n = *inputs;
  int sum = 0;
  for (int i = 0; i < n; i++)
    sum += i;
  *outputs = sum;
}

int main() {
  ThreadPool thread_pool;
  auto future = thread_pool.submit(function, 10);
  while (!future.is_completed())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  printf("Future value: %i\n", future.outputs());
}
