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

using octomap = tools::cmap<uint32_t, data_type, 3>;
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

        size_t cnt_fw = 0U;
        double check_fw = 0.0;
        for (const pair_t& pair : my_map)
        {
            std::cout << "Pair " << cnt_fw << " = {" << pair.first[0] << ", " << pair.first[1] << ", " << pair.first[2] << " } and rad = " << pair.second.radius << std::endl;
            ++cnt_fw;
            check_fw += pair.second.radius * pair.second.radius;
        }

        if (cnt_fw != my_map.size())
            return 255;

        size_t cnt_bw = 0U;
        double check_bw = 0.0;
        for (auto iter = my_map.rbegin(); iter != my_map.rend(); ++iter)
        {
            ++cnt_bw;
            check_bw += (*iter).second.radius * (*iter).second.radius;
        }

        if (cnt_bw != my_map.size())
            return 253;

        if (fabs(check_fw - check_bw) > 1e-6)
            return 251;
    }

    return 0;
}


