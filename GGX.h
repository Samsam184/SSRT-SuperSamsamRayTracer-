#ifndef GGX_H
#define GGX_H

#include "color.h"
#include <algorithm>
#include <cmath>
#include <ostream>
#include <fstream>

// --- helpers BRDF / Fresnel / GGX ---
float saturate(float v) { return std::max(0.0f, std::min(1.0f, v)); }

color schlick_F(const color& F0, float cosTheta) {
    float f = std::pow(1.0f - cosTheta, 5.0f);
    return F0 + (color(1, 1, 1) - F0) * f;
}

float distribution_GGX(float NdotH, float a) {
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    denom = 3.14159265359 * denom * denom;
    return a2 / std::max(1e-8f, denom);
}

float geometry_schlick_ggx(float NdotV, float k) {
    return NdotV / (NdotV * (1.0f - k) + k);
}

float geometry_smith(float NdotV, float NdotL, float k) {
    return geometry_schlick_ggx(NdotV, k) * geometry_schlick_ggx(NdotL, k);
}

inline color mix(const color& a, const color& b, float t) {
    return a * (1.0f - t) + b * t;
}


#endif
