#include <chrono>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdint>
#include <queue>
#include <set>
#include <mutex>
#include <thread>
#include <unistd.h>

#define LOOKUP_RATIO 10
#define NUM_ITEM 1000000
#define NUM_ITER 10000000

enum {
    INSERT,
    DELETE,
    LOOKUP,
};

struct task {
    int action;
    int item;
};

std::queue<task> q;
std::set<int> s;
std::mutex global_mutex_q;
std::mutex global_mutex_s;

void produce(unsigned int *seed)
{
    task task_batch[100];

    for (int i = 0; i < 100; ++i) {
        task t;
        int action = rand_r(seed) % LOOKUP_RATIO;

        switch (action) {
        case 0:
	    t.action = INSERT;
	    break;
        case 1:
	    t.action = DELETE;
	    break;
        default:
	    t.action = LOOKUP;
	    break;
        }
    
        t.item = rand_r(seed) % NUM_ITEM;
        task_batch[i] = t;
    }

    while (q.size() > 100) {}
    global_mutex_q.lock();
    for (int i = 0; i < 100; ++i) {
        q.push(task_batch[i]);
    }
    global_mutex_q.unlock();
}

int consume()
{
    while (q.empty()) {}

    int num_ops = 0;
    global_mutex_q.lock();
    task t = q.front();
    q.pop();
    global_mutex_q.unlock();

    switch (t.action) {
    case INSERT: {
        global_mutex_s.lock();
	auto res = s.insert(t.item);
        global_mutex_s.unlock();
	num_ops += res.second;
	break;
    }
    case DELETE: {
        global_mutex_s.lock();
	num_ops += s.erase(t.item);
        global_mutex_s.unlock();
	break;
    }
    default: {
        global_mutex_s.lock();
        auto res = s.find(t.item);
        global_mutex_s.unlock();
	num_ops += (res != s.end());
	break;
    }
    }

    return num_ops;
}

void init()
{
    unsigned int seed = 0;
    
    for (int i = 0; i < NUM_ITEM / 2; i++) {
	s.insert(rand_r(&seed) % NUM_ITEM);
    }
}

void run_producer(int mili_second) {
    unsigned int seed  = 0;
    for (int i = 0; i < NUM_ITER; i++) {
        if ((i+1)%100000 == 0) {
            std::cout << "producer iteration: " << i + 1 << "\n";
        }
        produce(&seed);
        usleep(mili_second);
    }
}

void run_consumer(int* num_consumer) {
    int num_ops = 0;
    for (int i = 0; i < NUM_ITER / (*num_consumer); i++) {
        if ((i+1)%100000 == 0) {
            std::cout << "consumer iteration: " << i + 1 << "\n";
        }
        num_ops += consume();
    }
    std::cout << " operations " << num_ops << std::endl;
}

int main( int argc, const char* argv[] ) {

    int num_ops; /*So the compiler does not optimize away our code */
    int num_consumer = atoi(argv[1]);
    int sleep_mili_second = atoi(argv[2]);
    unsigned int seed  = 0;
    init();

    int num_thread = num_consumer + 1;
    std::thread thrs[num_thread];
    auto start = std::chrono::system_clock::now();

    thrs[0] = std::thread(run_producer, sleep_mili_second);

    for (int i = 1; i < num_thread; ++i) {
        thrs[i] = std::thread(run_consumer, &num_consumer);
    }

    thrs[0].join();

    for (int i = 1; i < num_thread; ++i) {
        thrs[i].join();
    }

    auto end = std::chrono::system_clock::now();

    auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << std::endl;;

    return 0;
}
