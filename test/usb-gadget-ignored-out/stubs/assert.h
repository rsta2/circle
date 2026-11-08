#pragma once
#include <stdexcept>
#define assert(expr) ((expr) ? (void)0 : throw std::runtime_error (#expr))
