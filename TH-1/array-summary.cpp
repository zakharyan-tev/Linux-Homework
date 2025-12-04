#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <pthread.h>
#include <cstdlib>

using ull = unsigned long long;
using ll = long long;

struct ThreadArg {
    const std::vector<ll>* arr;
    size_t start;
    size_t end;
    ull idx;
    std::vector<long long>* partials;
};

void* thread_sum(void* arg) {
    auto* a = static_cast<ThreadArg*>(arg);
    long long s = 0;

    for (size_t i = a->start; i < a->end; ++i)
        s += (*a->arr)[i];

    (*a->partials)[a->idx] = s;
    return nullptr;
}

void cancel_all(std::vector<pthread_t>& threads, size_t created) {
    for (size_t i = 0; i < created; ++i) {
        pthread_cancel(threads[i]);
    }
    for (size_t i = 0; i < created; ++i) {
        pthread_join(threads[i], nullptr);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <N> <M>\n";
        return 1;
    }

    size_t N = std::stoull(argv[1]);
    size_t M = std::stoull(argv[2]);

    if (N < 2) {
        std::cerr << "N should be >= 2\n";
        return 1;
    }
    if (M < 1) {
        std::cerr << "M should be >= 1\n";
        return 1;
    }
    
    std::vector<ll> arr(N);
    std::mt19937_64 rng(12345678);
    std::uniform_int_distribution<ll> dist(0, 100);

    for (size_t i = 0; i < N; ++i)
        arr[i] = dist(rng);

    auto t0 = std::chrono::steady_clock::now();
    long long sum_single = 0;

    for (ll v : arr)
        sum_single += v;

    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dur_single = t1 - t0;

    std::vector<pthread_t> threads(M);
    std::vector<ThreadArg> args(M);
    std::vector<long long> partials(M, 0);

    size_t base = N / M;
    size_t rem = N % M;

    auto t2 = std::chrono::steady_clock::now();

    size_t offset = 0;
    size_t created = 0;

    for (size_t i = 0; i < M; ++i) {
        size_t chunk = base + (i < rem ? 1 : 0);

        args[i] = ThreadArg{
            .arr = &arr,
            .start = offset,
            .end = offset + chunk,
            .idx = i,
            .partials = &partials
        };

        offset += chunk;

        int rc = pthread_create(&threads[i], nullptr, thread_sum, &args[i]);
        if (rc != 0) {
            std::cerr << "Error: failed to create thread " << i
                      << ", code = " << rc << "\n";

            cancel_all(threads, created);
            return 2;
        }

        created++;
    }

    for (size_t i = 0; i < M; ++i) {
        int rc = pthread_join(threads[i], nullptr);
        if (rc != 0) {
            std::cerr << "Error: pthread_join failed for thread "
                      << i << ", code = " << rc << "\n";
            return 3;
        }
    }

    long long sum_multi = 0;
    for (ll p : partials)
        sum_multi += p;

    auto t3 = std::chrono::steady_clock::now();
    std::chrono::duration<double> dur_multi = t3 - t2;

    std::cout << "Time spent without threads: " << dur_single.count() << " sec\n";
    std::cout << "Time spent with " << M << " threads: " << dur_multi.count() << " sec\n";
    std::cout << "Single-threaded sum = " << sum_single << "\n";
    std::cout << "Multi-threaded  sum = " << sum_multi << "\n";

    return 0;
}
