#include <cstdio>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <barrier>

constexpr int ITERS = 3;
constexpr int INCREMENTS = 1'000'000;
constexpr int THREAD_COUNT = 32;

std::barrier sync_point(1 + THREAD_COUNT);
std::atomic<int> atomic_value = 0;
int plain_value = 0;
volatile int volatile_value = 0;
std::mutex lock;
std::chrono::time_point<std::chrono::high_resolution_clock> timer_start;

void start_timer() {
    timer_start = std::chrono::high_resolution_clock::now();
}

double end_timer() {
    auto timer_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = timer_end - timer_start;
    return elapsed.count();
}

void thread_main(int thread_index) {
    // ===== Test 1: Contended increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        for (int j = 0; j < INCREMENTS; j++)
            atomic_value++;
        sync_point.arrive_and_wait();
    }

    // ===== Test 2: Uncontended increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        if (thread_index == 0)
            for (int j = 0; j < THREAD_COUNT * INCREMENTS; j++)
                atomic_value++;
        sync_point.arrive_and_wait();
    }

    // ===== Test 3: Contended increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        for (int j = 0; j < INCREMENTS; j++)
            atomic_value.fetch_add(1, std::memory_order_relaxed);
        sync_point.arrive_and_wait();
    }

    // ===== Test 4: Uncontended increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        if (thread_index == 0)
            for (int j = 0; j < THREAD_COUNT * INCREMENTS; j++)
                atomic_value.fetch_add(1, std::memory_order_relaxed);
        sync_point.arrive_and_wait();
    }

    // ===== Test 5: Mutex increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        for (int j = 0; j < INCREMENTS; j++) {
            std::unique_lock<std::mutex> g(lock);
            plain_value++;
        }
        sync_point.arrive_and_wait();
    }

    // ===== Test 6: Mutex increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        if (thread_index == 0) {
            for (int j = 0; j < THREAD_COUNT * INCREMENTS; j++) {
                std::unique_lock<std::mutex> g(lock);
                plain_value++;
            }
        }
        sync_point.arrive_and_wait();
    }

    // ===== Test 7: Contended volatile increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        for (int j = 0; j < INCREMENTS; j++)
            volatile_value++;
        sync_point.arrive_and_wait();
    }

    // ===== Test 8: Uncontended volatile increment =====
    for (int i = 0; i < ITERS; i++) {
        sync_point.arrive_and_wait();
        if (thread_index == 0)
            for (int j = 0; j < THREAD_COUNT * INCREMENTS; j++)
                volatile_value++;
        sync_point.arrive_and_wait();
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < THREAD_COUNT; i++)
        threads.emplace_back(thread_main, i);

    printf("==== Increment speed tests ====\n");
    printf("In all cases we're issuing %.1fM increments for each of %i threads.\n", INCREMENTS * 1e-6, THREAD_COUNT);
    printf("In the cases with no contention we simply have one thread do all %.1fM increments.\n", THREAD_COUNT * INCREMENTS * 1e-6);
    printf("\n");

    for (const char* test_name : {
        "contended_atomic_increment",
        "uncontended_atomic_increment",
        "contended_atomic_increment_relaxed",
        "uncontended_atomic_increment_relaxed",
        "contended_mutex_increment",
        "uncontended_mutex_increment",
        "contended_volatile_increment",
        "uncontended_volatile_increment",
    }) {
        for (int i = 0; i < ITERS; i++) {
            // Wait for everyone to be ready.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            start_timer();
            // Set everyone into motion.
            sync_point.arrive_and_wait();

            // Wait for everyone to be done.
            sync_point.arrive_and_wait();
            double elapsed = end_timer();
            double speed = THREAD_COUNT * INCREMENTS / elapsed;
            printf("Elapsed: %7.2f ms (%6.1fM increments/second overall) [%s]\n", test_name, 1e3 * elapsed, speed * 1e-6);
        }
        printf("\n");
    }

    for (auto& thread : threads)
        thread.join();
}
