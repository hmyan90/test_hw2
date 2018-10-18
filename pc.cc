#include <chrono>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdint>
#include <queue>
#include <set>

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

    while (q.size() > 100) {}
    q.push(t);
}

int consume()
{
    while (q.empty()) {}

    int num_ops = 0;
    task t = q.front();
    q.pop();

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

int main( int argc, const char* argv[] ) {

    int num_ops; /*So the compiler does not optimize away our code */
    unsigned int seed  = 0;
    init();

    auto start = std::chrono::system_clock::now();

    for (int i = 0; i < NUM_ITER; i++) {
	produce(&seed);
	num_ops += consume();
    }
    
    auto end = std::chrono::system_clock::now();

    auto elapsed =
	std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << " operations " << num_ops << std::endl;;

    return 0;
}
