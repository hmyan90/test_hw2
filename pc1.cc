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

void produce(unsigned int *seed)
{
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

    bool flag = true;
    while (flag == true) {
        global_mutex_q.lock();
        if (q.size() <= 100) {
	    flag = false;
	} else {
	    global_mutex_q.unlock();
            // usleep(1);
	}
    }
    q.push(t);
    global_mutex_q.unlock();
}

int consume()
{
    int num_ops = 0;
	
    bool flag = true;
    while (flag == true) {
        global_mutex_q.lock();
        if (!q.empty()) {
	    flag = false;
	} else {
	    global_mutex_q.unlock();
            // usleep(1);
	}
    }

    task t = q.front();
    q.pop();
    global_mutex_q.unlock();

    switch (t.action) {
    case INSERT: {
	auto res = s.insert(t.item);
	num_ops += res.second;
	break;
    }
    case DELETE: {
	num_ops += s.erase(t.item);
	break;
    }
    default: {
        auto res = s.find(t.item);
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

void run_producer() {
    unsigned int seed  = 0;
    for (int i = 0; i < NUM_ITER; i++) {
        if ((i+1)%100000 == 0) {
            std::cout << "producer iteration: " << i + 1 << "\n";
        }
        produce(&seed);
    }
}

void run_consumer() {
    int num_ops=0;
    for (int i = 0; i < NUM_ITER; i++) {
        if ((i+1)%100000 == 0) {
            std::cout << "consumer iteration: " << i + 1 << "\n";
        }
        num_ops += consume();
    }
    std::cout << " operations " << num_ops << std::endl;;
}

int main( int argc, const char* argv[] ) {

    int num_ops; /*So the compiler does not optimize away our code */
    unsigned int seed  = 0;
    init();

    int num_thread = 2;
    std::thread thrs[num_thread];
    auto start = std::chrono::system_clock::now();

    thrs[0] = std::thread(run_producer);
    thrs[1] = std::thread(run_consumer);

    thrs[0].join();
    thrs[1].join();

    auto end = std::chrono::system_clock::now();

    auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << std::endl;;

    return 0;
}
