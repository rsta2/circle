// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdlib>
#include <cstdio>
#define assert(condition) ((condition) ? (void) 0 : (std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition), std::abort()))
