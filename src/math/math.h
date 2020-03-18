
#pragma once

#include <cstddef>
#include <cmath>

#define PI 3.14159265359f

float radians(float degrees);
float degrees(float radians);

float moveValueTo(float value, float target, float step);
float moveAngleTo(float value, float target, float step);

size_t align(size_t value, size_t alignment);
