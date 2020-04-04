/*
    cmap is a resizable coordinate map

    Copyright (c) 2020, Sebastian Wouters
    All rights reserved.

    This file is part of cmap, licensed under the BSD 3-Clause License.
    A copy of the License can be found in the file LICENSE in the root
    folder of this project.
*/

#include <iostream>
#include <random>
#include <chrono>

#include "cmap.hpp"
#include "wrap.hpp"

struct data_type
{
    double s;
    double p;
    double m;
};

using cmap = tools::cmap<uint32_t, 3, data_type>;
using wrap = tools::wrap<uint32_t, 3, data_type>;
using coord_t = cmap::coord_t;

void merge(data_type& left, const data_type& right)
{
    left.s += right.s;
    left.p *= right.p;
    left.m  = std::max(left.m, right.m);
}

int main()
{
    constexpr const double center = 2147483648;
    constexpr const double pi     = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 2.0);
    std::normal_distribution<double> rad(100000.0, 100.0);

    std::vector<std::pair<coord_t, data_type>> buffer;
    const size_t number = 1000000U;
    buffer.reserve(number);

    std::cout << "Generating " << number << " samples" << std::endl;
    auto start = std::chrono::system_clock::now();
    for (size_t count = 0; count < number; ++count)
    {
        double radius    = rad(gen);
        double phi       = dis(gen) * pi;
        double cos_theta = dis(gen) - 1.0;
        double sin_theta = sin(acos(cos_theta));
        uint32_t x = static_cast<uint32_t>(center + radius * sin_theta * cos(phi) + 0.5);
        uint32_t y = static_cast<uint32_t>(center + radius * sin_theta * sin(phi) + 0.5);
        uint32_t z = static_cast<uint32_t>(center + radius * cos_theta + 0.5);
        buffer.push_back({{ x, y, z }, { dis(gen), dis(gen), dis(gen) }});
    }
    auto stop = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    std::cout << "Generating samples                  : " << 1e-3 * time << " s." << std::endl;

    {
        cmap my_cmap;
        start = std::chrono::system_clock::now();
        for (auto& pair : buffer)
        {
            my_cmap.insert(pair.first, pair.second);
        }
        stop = std::chrono::system_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Inserting samples in cmap           : " << 1e-3 * time << " s." << std::endl;

        start = std::chrono::system_clock::now();
        while (8U * my_cmap.size() > number)
            my_cmap.resize();
        stop = std::chrono::system_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Resizing cmap                       : " << 1e-3 * time << " s." << std::endl;
        std::cout << "Performed " << static_cast<uint32_t>(my_cmap.num_resizes()) << " cmap resizes" << std::endl;
    }

    {
        wrap my_wrap;
        start = std::chrono::system_clock::now();
        for (auto& pair : buffer)
        {
            my_wrap.insert(pair.first, pair.second);
        }
        stop = std::chrono::system_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Inserting samples in wrap(std::map) : " << 1e-3 * time << " s." << std::endl;

        start = std::chrono::system_clock::now();
        while (8U * my_wrap.size() > number)
            my_wrap.resize();
        stop = std::chrono::system_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Resizing wrap(std::map)             : " << 1e-3 * time << " s." << std::endl;
        std::cout << "Performed " << static_cast<uint32_t>(my_wrap.num_resizes()) << " wrap(std::map) resizes" << std::endl;
    }

    return 0;
}


