#ifndef SIMPLE_THREAD_POOL_H
#define SIMPLE_THREAD_POOL_H

#include <atomic>
#include <optional>
#include <queue>
#include <condition_variable>
#include <thread>

// Ignore the details of this queue.
// The point is just that it's a thread-safe FIFO queue.
template<typename T>
struct ThreadSafeQueue {
  std::queue<T> entries;
  std::mutex lock;
  std::condition_variable cv;

  void push(T x) {
    std::unique_lock<std::mutex> g(lock);
    entries.push(x);
    cv.notify_one();
  }

  void pop(T& x) {
    std::unique_lock<std::mutex> g(lock);
    cv.wait(g, [this]() { return entries.size() > 0; });
    x = entries.front();
    entries.pop();
  }
};

struct ThreadPool {
  struct Future {
    std::atomic<bool> is_completed = false;
    std::atomic<bool> is_releasing = false;
    std::string inputs;
    std::string outputs;
  };

  struct Command {
    Future* future;
    void (*f)(const void* inputs, void* outputs);
  };

  ThreadSafeQueue<std::optional<Command>> command_queue;
  std::vector<std::thread> workers;

  ThreadPool() {
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++)
      workers.emplace_back(worker_main, this);
  }

  ~ThreadPool() {
    // Send each worker a death signal.
    for (size_t i = 0; i < workers.size(); i++)
      command_queue.push(std::nullopt);
    for (std::thread& thread : workers)
      thread.join();
  }

  static void worker_main(ThreadPool* self) {
    std::optional<Command> command;
    while (true) {
      self->command_queue.pop(command);
      if (!command.has_value())
        break;
      command->f((const void*) command->future->inputs.data(), (void*) command->future->outputs.data());
      command->future->is_completed.store(true);
      self->half_release(command->future);
    }
  }

  void half_release(Future* future) {
    bool was_releasing = future->is_releasing.exchange(true);
    if (was_releasing)
      delete future;
  }

  template<typename Outputs>
  struct FutureView {
    Future* future_pointer;

    bool is_completed() const {
      return future_pointer->is_completed.load();
    }

    Outputs& outputs() {
      return *(Outputs*) future_pointer->outputs.data();
    }
  };

  template<typename Inputs, typename Outputs>
  FutureView<Outputs> submit(void (*f)(const Inputs* inputs, Outputs* outputs), const Inputs& inputs) {
    Future* future = new Future();
    future->inputs.resize(sizeof(Inputs));
    future->outputs.resize(sizeof(Outputs));
    *(Inputs*) future->inputs.data() = inputs;
    command_queue.push(Command{
      .future = future,
      .f = (void (*)(const void*, void*)) f,
    });
    return {future};
  }

  template<typename Outputs>
  void release(FutureView<Outputs> future_view) {
    half_release(future_view.future_pointer);
  }
};

#endif
