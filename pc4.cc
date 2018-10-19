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
#define MAX_CUSOMER 32

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
std::mutex global_mutex_q;
std::vector<std::set<int>> set_vec(4*MAX_CUSOMER);
std::vector<std::mutex> set_lock_vec(4*MAX_CUSOMER);

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
	}
    }
    q.push(t);
    global_mutex_q.unlock();
}

int consume(int sets)
{
    int num_ops = 0;
	
    bool flag = true;
    while (flag == true) {
        global_mutex_q.lock();
        if (!q.empty()) {
	    flag = false;
	} else {
	    global_mutex_q.unlock();
	}
    }

    task t = q.front();
    q.pop();
    global_mutex_q.unlock();

    int index = t.item % sets;
    switch (t.action) {
    case INSERT: {
        set_lock_vec[index].lock();
	auto res = set_vec[index].insert(t.item);
        set_lock_vec[index].unlock();
	num_ops += res.second;
	break;
    }
    case DELETE: {
        set_lock_vec[index].lock();
	num_ops += set_vec[index].erase(t.item);
        set_lock_vec[index].unlock();
	break;
    }
    default: {
        set_lock_vec[index].lock();
        auto res = set_vec[index].find(t.item);
	num_ops += (res != sec_vec[index].end());
        set_lock_vec[index].unlock();
	break;
    }
    }

    return num_ops;
}

void init(int sets)
{
    unsigned int seed = 0;
    
    for (int i = 0; i < NUM_ITEM / 2; i++) {
        int item = rand_r(&seed) % NUM_ITEM;
	set_vec[item % sets].insert(item);
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

void run_consumer(int num_consumer, int sets) {
    int num_ops = 0;
    for (int i = 0; i < NUM_ITER / num_consumer; i++) {
        if ((i+1)%100000 == 0) {
            std::cout << "consumer iteration: " << i + 1 << "\n";
        }
        num_ops += consume(sets);
    }
    std::cout << " operations " << num_ops << std::endl;
}

int main( int argc, const char* argv[] ) {

    int num_ops; /*So the compiler does not optimize away our code */
    int num_consumer = atoi(argv[1]);
    int sets = 4*num_consumer;
    unsigned int seed  = 0;

    init(sets);

    int num_thread = num_consumer + 1;
    std::thread thrs[num_thread];
    auto start = std::chrono::system_clock::now();

    thrs[0] = std::thread(run_producer);

    for (int i = 1; i < num_thread; ++i) {
        thrs[i] = std::thread(run_consumer, num_consumer, sets);
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
