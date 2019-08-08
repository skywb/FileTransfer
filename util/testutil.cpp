#include "testutil.h"
#include "random"
bool Abandon(int x) {
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(1, 100);
    int mean = uniform_dist(e1);
    return mean <= x;
}
