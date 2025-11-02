#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <random>
#include <limits>
#include <memory>
#include <thread>
#include "fast_rng.h"
#include "external/pcg/pcg_random.hpp"
// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() noexcept {
    
    static pcg32 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    double r = dist(rng);
    return r;
    
    /*
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
    
    uint64_t rng_state = 1337u + std::hash<std::thread::id>{}(std::this_thread::get_id());
    return randf(rng_state);
    */
}

inline float random_double(float min, float max) {
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
    return int(random_double(min, max + 1));
}
// Common Headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

#endif