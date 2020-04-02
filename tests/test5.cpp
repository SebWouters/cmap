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

struct data_type
{
    double s;
    double p;
    double m;

    data_type(double _s, double _p, double _m) : s(_s), p(_p), m(_m) {}
};

using quadmap = tools::cmap<uint16_t, 2, data_type>;

void merge(data_type& left, const data_type& right)
{
    left.s += right.s;
    left.p *= right.p;
    left.m  = std::max(left.m, right.m);
}

void print_map(const quadmap& my_map)
{
    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Size(cmap) = " << my_map.size() << std::endl;
    size_t cnt = 0U;
    for (auto iter = my_map.rbegin(); iter != my_map.rend(); ++iter)
    {
        std::cout << "Pair " << cnt << " = " << std::endl;
        std::cout << "    coord = { " << (*iter).first[0] << ", " << (*iter).first[1] << " }" << std::endl;
        std::cout << "    data  = { " << (*iter).second.s << ", " << (*iter).second.p << ", " << (*iter).second.m << " }" << std::endl;
        std::cout << "    node  = { " << iter.node() << " } with level = { " << static_cast<uint32_t>(iter.node()->_level) << " }" << std::endl;
        ++cnt;
    }
    std::cout << "--------------------------------------------------------------------" << std::endl;;
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> co(0, 3);
    std::uniform_real_distribution<double>  dt(1.1, 1.9);

    quadmap my_map;

    for (uint32_t count = 0; count < 100U; ++count)
    {
        my_map.emplace( { co(gen), co(gen) }, dt(gen), dt(gen), dt(gen) );
    }

    print_map(my_map);

    my_map.resize();
    std::cout << "cmap::resize()" << std::endl;
    
    print_map(my_map);

    my_map.prune();
    std::cout << "cmap::prune()" << std::endl;

    print_map(my_map);

    return 0;
}


