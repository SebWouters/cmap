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
    uint32_t num1;
    uint32_t num2;
};

using hexamap = tools::cmap<uint16_t, data_type, 4>;
using coord_t = hexamap::coord_t;
using  pair_t = hexamap::pair_t;

void merge(data_type& left, const data_type& right)
{
    left.num1 += right.num1;
    left.num2 += right.num2;
}

void print_line(const coord_t& coord, const data_type& data)
{
    std::cout << "coord (" << coord[0] << ", " << coord[1] << ", "<< coord[2] << ", "<< coord[3] << ") holds data {" << data.num1 << ", " << data.num2 << "}" << std::endl;
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> co(0, 64);
    std::uniform_int_distribution<uint32_t> dt(0, 1024);

    hexamap my_map;

    for (uint32_t count = 0; count < 20U; ++count)
    {
        coord_t coord = { co(gen), co(gen), co(gen), co(gen) };
        data_type data = { dt(gen), dt(gen) };
        my_map.insert(coord, data);
        std::cout << "Insert: ";
        print_line(coord, data);
    }

    std::cout << "---------------------------------------------" << std::endl;

    std::cout << "Overwrite via operator[](coord):" << std::endl;
    auto reti = my_map.rbegin();
    const coord_t coord1 = (*reti).first;
    const data_type old1 = (*reti).second;
    std::cout << "    Before: "; print_line((*reti).first, (*reti).second);
    data_type& data1 = my_map[(*reti).first];
    while (old1.num1 == data1.num1)
        data1 = { dt(gen), dt(gen) };
    std::cout << "    After:  "; print_line((*reti).first, (*reti).second);
  //const data_type new1 = (*reti).second;

    if (old1.num1 == (*reti).second.num1)
        return 255;

    std::cout << "Overwrite via (*reverse_iterator).second:" << std::endl;
    ++reti;
    ++reti;
    const coord_t coord2 = (*reti).first;
    const data_type old2 = (*reti).second;
    std::cout << "    Before: "; print_line((*reti).first, (*reti).second);
    data_type& data2 = (*reti).second;
    while (old2.num1 == data2.num1)
        data2 = { dt(gen), dt(gen) };
    std::cout << "    After:  "; print_line((*reti).first, (*reti).second);
    const data_type new2 = (*reti).second;

    if (old2.num1 == (*reti).second.num1)
        return 253;

    std::cout << "Overwrite via (*iterator).second:" << std::endl;
    auto iter = my_map.begin();
    ++iter;
    const coord_t coord3 = (*iter).first;
    const data_type old3 = (*iter).second;
    std::cout << "    Before: "; print_line((*iter).first, (*iter).second);
    data_type& data3 = (*iter).second;
    while (old3.num1 == data3.num1)
        data3 = { dt(gen), dt(gen) };
    std::cout << "    After:  "; print_line((*iter).first, (*iter).second);
  //const data_type new3 = (*iter).second;

    if (old3.num1 == (*iter).second.num1)
        return 251;

    std::cout << "Overwrite via (*find(coord)).second:" << std::endl;
    ++iter;
    ++iter;
    const coord_t coord4 = (*iter).first;
    const data_type old4 = (*iter).second;
    std::cout << "    Before: "; print_line((*iter).first, (*iter).second);
    data_type& data4 = (*my_map.find(coord4)).second;
    while (old4.num1 == data4.num1)
        data4 = { dt(gen), dt(gen) };
    std::cout << "    After:  "; print_line((*iter).first, (*iter).second);
    const data_type new4 = (*iter).second;

    if (old4.num1 == (*iter).second.num1)
        return 249;

    if (!my_map.contains(coord1))
        return 247;
    if (!my_map.contains(coord3))
        return 245;

    auto find2 = my_map.find(coord2);
    auto find4 = my_map.find(coord4);

    if ((*find2).second.num1 != new2.num1)
        return 243;
    if ((*find4).second.num1 != new4.num1)
        return 241;

    std::cout << "Erasing: "; print_line((*find4).first, (*find4).second);
    if (1U != my_map.erase((*find4).first))
        return 239;

    if (my_map.contains(coord4))
        return 237;

    std::cout << "---------------------------------------------" << std::endl;
    for (const pair_t& pair : my_map)
    {
        std::cout << "Cmap: ";
        print_line(pair.first, pair.second);
    }

    return 0;
}


