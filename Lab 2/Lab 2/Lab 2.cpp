#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <deque>
#include <vector>
#include <chrono>
#include <cmath>
#include <windows.h>

using namespace std;

// Helper function: Check if a number is prime
bool isPrime(long long n) {
    if (n <= 1) return false;
    for (long long i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// Helper function: Find the n-th prime number
long long findNthPrime(int n) {
    if (n <= 0) return 0;
    int count = 0;
    long long current = 1;
    while (count < n) {
        current++;
        if (isPrime(current)) {
            count++;
        }
    }
    return current;
}

// 1.2.1. Synchronization using a flag and sleep_for
namespace Task1_2_1 {
    queue<int> data_queue;
    bool data_ready = false;
    mutex mtx;

    void DataPreparation() {
        cout << "[DataPreparation] Enter 4 integers separated by space: ";
        int val;
        for (int i = 0; i < 4; ++i) {
            if (cin >> val) {
                lock_guard<mutex> lk(mtx);
                data_queue.push(val);
            }
        }
        {
            lock_guard<mutex> lk(mtx);
            data_ready = true;
        }
    }

    void DataProcessing() {
        while (true) {
            {
                unique_lock<mutex> lk(mtx);
                if (data_ready && data_queue.empty()) {
                    break;
                }
                if (!data_queue.empty()) {
                    int num = data_queue.front();
                    data_queue.pop();
                    lk.unlock();

                    if (isPrime(num)) {
                        cout << "  [DataProcessing] Number " << num << " is PRIME.\n";
                    }
                    else {
                        cout << "  [DataProcessing] Number " << num << " is COMPOSITE.\n";
                    }
                    continue;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.1: Synchronization via flag (sleep_for)\n";
        cout << "--------------------------------------------------\n";

        while (!data_queue.empty()) data_queue.pop();
        data_ready = false;

        thread t1(DataPreparation);
        thread t2(DataProcessing);

        t1.detach();
        t2.join();
    }
}

// 1.2.2. notify_all() and condition_variable handling
namespace Task1_2_2 {
    mutex mtx;
    condition_variable cv;
    int i_val = 0;

    void Waits(int id) {
        unique_lock<mutex> lk(mtx);
        cout << "  [Waits " << id << "] Entering waiting state...\n";
        cv.wait(lk, [] { return i_val == 1; });
        cout << "  [Waits " << id << "] Waiting completed!\n";
    }

    void Awake() {
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "[Awake] First notify_all() call (i_val is still 0)...\n";
        cv.notify_all();

        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lk(mtx);
            i_val = 1;
        }
        cout << "[Awake] Assigned i_val = 1. Repeated notify_all() call...\n";
        cv.notify_all();
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.2: condition_variable with notify_all()\n";
        cout << "--------------------------------------------------\n";
        i_val = 0;

        thread t1(Waits, 1);
        thread t2(Waits, 2);
        thread t3(Waits, 3);
        thread t_awake(Awake);

        t_awake.join();
        t1.join();
        t2.join();
        t3.join();
    }
}

// 1.2.3. notify_one() to wake up a single thread
namespace Task1_2_3 {
    mutex mtx;
    condition_variable cv;
    int i_val = 0;

    void ThreadWorker(int id) {
        unique_lock<mutex> lk(mtx);
        cout << "  [Thread " << id << "] Entering waiting state...\n";
        cv.wait(lk, [] { return i_val == 1; });
        cout << "  --> Message from thread " << id << "\n";
    }

    void NotifyThread() {
        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lk(mtx);
            i_val = 1;
        }
        cout << "[Notify] Calling notify_one()...\n";
        cv.notify_one();
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.3: condition_variable with notify_one()\n";
        cout << "--------------------------------------------------\n";
        i_val = 0;

        thread t1(ThreadWorker, 1);
        thread t2(ThreadWorker, 2);
        thread t3(ThreadWorker, 3);
        thread t_notify(NotifyThread);

        t_notify.join();

        this_thread::sleep_for(chrono::milliseconds(100));
        cv.notify_all();

        t1.join();
        t2.join();
        t3.join();
    }
}

// 1.2.4. Modification of 1.2.1 using condition_variable
namespace Task1_2_4 {
    queue<int> data_queue;
    bool input_finished = false;
    mutex mtx;
    condition_variable cv;

    void DataPreparation() {
        cout << "[DataPreparation CV] Enter 4 integers: ";
        int val;
        for (int i = 0; i < 4; ++i) {
            if (cin >> val) {
                {
                    lock_guard<mutex> lk(mtx);
                    data_queue.push(val);
                }
                cv.notify_one();
            }
        }
        {
            lock_guard<mutex> lk(mtx);
            input_finished = true;
        }
        cv.notify_all();
    }

    void DataProcessing() {
        while (true) {
            unique_lock<mutex> lk(mtx);
            cv.wait(lk, [] { return !data_queue.empty() || input_finished; });

            while (!data_queue.empty()) {
                int num = data_queue.front();
                data_queue.pop();
                lk.unlock();

                if (isPrime(num)) {
                    cout << "  [DataProcessing CV] Number " << num << " is PRIME.\n";
                }
                else {
                    cout << "  [DataProcessing CV] Number " << num << " is COMPOSITE.\n";
                }

                lk.lock();
            }

            if (input_finished && data_queue.empty()) {
                break;
            }
        }
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.4: Synchronization via condition_variable\n";
        cout << "--------------------------------------------------\n";

        while (!data_queue.empty()) data_queue.pop();
        input_finished = false;

        thread t1(DataPreparation);
        thread t2(DataProcessing);

        t1.detach();
        t2.join();
    }
}

// 1.2.5. Comparing std::launch::deferred and std::launch::async
namespace Task1_2_5 {
    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.5: async (deferred vs async)\n";
        cout << "--------------------------------------------------\n";

        int n = 100000;
        cout << "Calculating n = " << n << "-th prime number.\n";

        cout << "\n1) Running with launch::deferred...\n";
        auto t_start1 = chrono::high_resolution_clock::now();
        future<long long> fut_def = async(launch::deferred, findNthPrime, n);

        cout << "   Executing sqrt(" << n << ") = " << sqrt(n) << "\n";
        cout << "   Calling fut_def.get() (calculation starts now)...\n";
        long long res_def = fut_def.get();
        auto t_end1 = chrono::high_resolution_clock::now();
        cout << "   [Deferred] Result: " << res_def << ", Time: "
            << chrono::duration<double>(t_end1 - t_start1).count() << " s.\n";

        cout << "\n2) Running with launch::async...\n";
        auto t_start2 = chrono::high_resolution_clock::now();
        future<long long> fut_async = async(launch::async, findNthPrime, n);

        cout << "   Executing sqrt(" << n << ") = " << sqrt(n) << "\n";
        cout << "   Calling fut_async.get()...\n";
        long long res_async = fut_async.get();
        auto t_end2 = chrono::high_resolution_clock::now();
        cout << "   [Async] Result: " << res_async << ", Time: "
            << chrono::duration<double>(t_end2 - t_start2).count() << " s.\n";
    }
}

// 1.2.6. Task queue using packaged_task and deques
namespace Task1_2_6 {
    deque<packaged_task<long long()>> task_deque;
    deque<int> n_deque;
    mutex mtx;
    bool stop_worker = false;

    void WorkerThread() {
        while (true) {
            packaged_task<long long()> task;
            int n_val = 0;
            {
                unique_lock<mutex> lk(mtx);
                if (task_deque.empty()) {
                    if (stop_worker) break;
                    lk.unlock();
                    this_thread::sleep_for(chrono::milliseconds(20));
                    continue;
                }
                task = move(task_deque.front());
                task_deque.pop_front();
                n_val = n_deque.front();
                n_deque.pop_front();
            }

            cout << "  [WorkerThread] Computing n = " << n_val << "...\n";
            task();
        }
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.6: packaged_task queue in deques\n";
        cout << "--------------------------------------------------\n";
        stop_worker = false;

        thread worker(WorkerThread);
        vector<future<long long>> futures;
        vector<int> test_numbers = { 5000, 10000, 15000 };

        for (int n : test_numbers) {
            packaged_task<long long()> task([n]() { return findNthPrime(n); });
            futures.push_back(task.get_future());

            {
                lock_guard<mutex> lk(mtx);
                task_deque.push_back(move(task));
                n_deque.push_back(n);
            }
        }

        {
            lock_guard<mutex> lk(mtx);
            stop_worker = true;
        }
        worker.join();

        cout << "\nResults of deque tasks execution:\n";
        for (size_t i = 0; i < futures.size(); ++i) {
            cout << "  - " << test_numbers[i] << "-th prime number: " << futures[i].get() << "\n";
        }
    }
}

// 1.2.7. Result transfer via promise / future
namespace Task1_2_7 {
    void FirstThreadTask(int n,
        promise<long long> prom_nth,
        promise<bool> prom_signal,
        promise<long long> prom_ten_n)
    {
        long long nth_prime = findNthPrime(n);
        prom_nth.set_value(nth_prime);

        prom_signal.set_value(true);

        long long ten_n_prime = findNthPrime(n * 2);
        prom_ten_n.set_value(ten_n_prime);
    }

    void SecondThreadTask(future<bool> fut_signal, int n) {
        bool ready = fut_signal.get();
        if (ready) {
            this_thread::sleep_for(chrono::seconds(2));
            cout << "  [SecondThread] (after 2s delay) sqrt(" << n << ") = " << sqrt(n) << "\n";
        }
    }

    void run() {
        cout << "\n--------------------------------------------------\n";
        cout << "Task 1.2.7: Synchronization via promise / future\n";
        cout << "--------------------------------------------------\n";

        int n = 5000;
        cout << "Input value n = " << n << "\n";

        promise<long long> prom_nth;
        future<long long> fut_nth = prom_nth.get_future();

        promise<bool> prom_signal;
        future<bool> fut_signal = prom_signal.get_future();

        promise<long long> prom_ten_n;
        future<long long> fut_ten_n = prom_ten_n.get_future();

        thread t1(FirstThreadTask, n, move(prom_nth), move(prom_signal), move(prom_ten_n));
        thread t2(SecondThreadTask, move(fut_signal), n);

        cout << "  [MainThread] n-th prime number: " << fut_nth.get() << "\n";
        cout << "  [MainThread] (n*2)-th prime number: " << fut_ten_n.get() << "\n";

        t1.join();
        t2.join();
    }
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    Task1_2_1::run();
    Task1_2_2::run();
    Task1_2_3::run();
    Task1_2_4::run();
    Task1_2_5::run();
    Task1_2_6::run();
    Task1_2_7::run();

    return 0;
}