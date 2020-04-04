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

#include "cmap.hpp"
#include "wrap.hpp"

struct data_type
{
    double s;
    double p;
    double m;
};

using quadcmap = tools::cmap<uint16_t, 4, data_type>;
using quadwrap = tools::wrap<uint16_t, 4, data_type>;

using coord_t = quadcmap::coord_t;

void merge(data_type& left, const data_type& right)
{
    left.s += right.s;
    left.p *= right.p;
    left.m  = std::max(left.m, right.m);
}

bool print(quadcmap& my_cmap, quadwrap& my_wrap)
{
    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Size(cmap) = " << my_cmap.size() << std::endl;
    std::cout << "Size(wrap) = " << my_wrap.size() << std::endl;
    if (my_cmap.size() != my_wrap.size())
        return false;
    std::cout << "Resizes(cmap) = " << static_cast<uint32_t>(my_cmap.num_resizes()) << std::endl;
    std::cout << "Resizes(wrap) = " << static_cast<uint32_t>(my_wrap.num_resizes()) << std::endl;
    if (my_cmap.num_resizes() != my_wrap.num_resizes())
        return false;

    for (auto iter1 = my_wrap.begin(); iter1 != my_wrap.end(); ++iter1)
    {
        const coord_t coord = my_wrap.coord(iter1);
        const data_type& cmap_data = my_cmap[coord];
        std::cout << "At coord = { " << coord[0] << ", " << coord[1] << ", " << coord[2] << ", " << coord[3] << " }" << std::endl;
        std::cout << "    data(cmap) = { " << cmap_data.s << ", " << cmap_data.p << ", " << cmap_data.m << " }" << std::endl;
        std::cout << "    data(wrap) = { " << (*iter1).second.s << ", " << (*iter1).second.p << ", " << (*iter1).second.m << " }" << std::endl;
        double diff1 = fabs(cmap_data.s - (*iter1).second.s) / cmap_data.s;
        double diff2 = fabs(cmap_data.p - (*iter1).second.p) / cmap_data.p;
        double diff3 = fabs(cmap_data.m - (*iter1).second.m) / cmap_data.m;
        if ((diff1 > 1e-10) || (diff2 > 1e-10) || (diff3 > 1e-10))
            return false;
    }
    std::cout << "--------------------------------------------------------------------" << std::endl;
    return true;
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> co(0, 63);
    std::uniform_real_distribution<double>  dt(1.1, 1.9);

    quadcmap my_cmap;
    quadwrap my_wrap;

    const size_t number = 10000U;

    for (size_t count = 0; count < number; ++count)
    {
        coord_t coord  = { co(gen), co(gen), co(gen), co(gen) };
        data_type data = { dt(gen), dt(gen), dt(gen) };
        my_cmap.insert(coord, data);
        my_wrap.insert(coord, data);
    }

    bool success = print(my_cmap, my_wrap);
    if (!success)
        return 255;

    while (8 * my_cmap.size() > number)
        my_cmap.resize();
    while (8 * my_wrap.size() > number)
        my_wrap.resize();

    success = print(my_cmap, my_wrap);
    if (!success)
        return 253;

    return 0;
}


