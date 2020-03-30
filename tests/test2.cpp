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
    double radius;
};

using octomap = _cmap::cmap<uint32_t, data_type, 3>;
using coord_t = octomap::coord_t;
using  pair_t = octomap::pair_t;

void merge(data_type& left, const data_type& right)
{
    left.radius = sqrt(left.radius * left.radius + right.radius * right.radius);
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> co(0, 16);
    std::uniform_real_distribution<double>  rad(0.0, 3.0);

    octomap my_map;
    const size_t num_points = 1000U;

    for (size_t npoints = 0; npoints < num_points; ++npoints)
    {
        coord_t coord = { co(gen), co(gen), co(gen) };
        data_type data = { rad(gen) };
        my_map.insert(coord, data);
        std::cout << "Novel = {" << coord[0] << ", " << coord[1] << ", " << coord[2] << " } and rad = " << data.radius << std::endl;
    }


    while (8 * my_map.size() > num_points)
    {
        std::cout << "--------------------------" << std::endl;
        my_map.resize();

        size_t cnt = 0U;
        for (const pair_t& pair : my_map)
        {
            std::cout << "Pair " << cnt << " = {" << pair.coord[0] << ", " << pair.coord[1] << ", " << pair.coord[2] << " } and rad = " << pair.data.radius << std::endl;
            ++cnt;
        }

        if (cnt != my_map.size())
            return 255;
    }

    return 0;
}


