/*
    cmap is a resizable coordinate map

    Copyright (c) 2020, Sebastian Wouters
    All rights reserved.

    This file is part of cmap, licensed under the BSD 3-Clause License.
    A copy of the License can be found in the file LICENSE in the root
    folder of this project.
*/

#include <assert.h>
#include <random>
#include <iostream>
#include <chrono>

#include "permutation.hpp"

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0U, UINT32_MAX);

    bool equal = true;
    auto start = std::chrono::system_clock::now();
    const size_t test_size = 10000000U;

    for (size_t cnt = 0U; cnt < test_size; ++cnt)
    {
        const uint32_t coo1[3] = { dis(gen), dis(gen), dis(gen) };
        uint32_t perm[3];
        cmap::permute<uint32_t, 3>(coo1, perm);
        uint32_t coo2[3];
        cmap::unravel<uint32_t, 3>(perm, coo2);

        equal = equal && (coo1[0] == coo2[0])
                      && (coo1[1] == coo2[1])
                      && (coo1[2] == coo2[2]);
    }

    auto stop = std::chrono::system_clock::now();
    uint64_t time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    std::cout << "Time = " << 1e-9 * time_ns << " s." << std::endl;

    if (!equal)
        return 255;

    return 0;
}


