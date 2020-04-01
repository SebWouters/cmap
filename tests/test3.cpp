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
using  node_t = octomap::node_t;

void merge(data_type& left, const data_type& right)
{
    left.radius = sqrt(left.radius * left.radius + right.radius * right.radius);
}

void print_info(const octomap& my_map)
{
    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Size(cmap) = " << my_map.size() << std::endl;
    size_t cnt = 0U;
    for (auto iter = my_map.begin(); iter != my_map.end(); ++iter)
    {
        std::cout << "Pair " << cnt << " = " << std::endl;
        std::cout << "    coord = { " << (*iter).first[0] << ", " << (*iter).first[1] << ", " << (*iter).first[2] << " }" << std::endl;
        std::cout << "    rad   = " << (*iter).second.radius << std::endl;
        std::cout << "    node  = " << iter.node() << " with level " << static_cast<uint32_t>(iter.node()->_level) << std::endl;
        ++cnt;
    }
    std::cout << "--------------------------------------------------------------------" << std::endl;
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> co(0, 16);
    std::uniform_real_distribution<double>  rad(0.0, 3.0);

    octomap my_map;

    while (my_map.size() < 8U)
    {
        coord_t coord = { co(gen), co(gen), co(gen) };
        data_type data = { rad(gen) };
        my_map.insert(coord, data);
        std::cout << "Novel = { " << coord[0] << ", " << coord[1] << ", " << coord[2] << " } and rad = " << data.radius << std::endl;
    }

    const node_t * root = my_map.begin().node();
    if (root->_level != 31U)
        return 255;

    print_info(my_map);

    while (my_map.size() <= 8U)
    {
        coord_t coord = { 2048 + co(gen), 2048 + co(gen), 2048 + co(gen) };
        data_type data = { rad(gen) };
        my_map.insert(coord, data);
        std::cout << "Novel = { " << coord[0] << ", " << coord[1] << ", " << coord[2] << " } and rad = " << data.radius << std::endl;
    }

    const node_t * leaf = my_map.begin().node();
    if (leaf->_level >= 31U)
        return 253;

    print_info(my_map);

    while (my_map.size() > 8U)
    {
        size_t num_removed = my_map.erase((*(my_map.begin())).first);
        std::cout << "Removed " << num_removed << " elements. " << std::endl;
    }

    const node_t * node = my_map.begin().node();
    if (node->_level != 31U)
        return 251;
    if (root != node)
        return 249;

    print_info(my_map);

    return 0;
}


