/*
    cmap is a resizable coordinate map

    Copyright (c) 2020, Sebastian Wouters
    All rights reserved.

    This file is part of cmap, licensed under the BSD 3-Clause License.
    A copy of the License can be found in the file LICENSE in the root
    folder of this project.
*/

#pragma once

#include <assert.h>
#include <array>
#include <memory>
#include <iterator>
#include <type_traits>
#include <map>

#include "permutation.hpp"


namespace tools {


template<class _Tc, size_t _DIM, class _Td>
class wrap {

    public:

        typedef std::array<_Tc, _DIM>   coord_t;
        typedef std::map<coord_t, _Td>  map_t;

    private:

        map_t std_map;
        uint8_t _num_resizes;

        inline void _shift_(coord_t& permute)
        {
            _Tc head = 0U;
            for (_Tc& perm : permute)
            {
                _Tc tail = perm & ((1U << _DIM) - 1U);
                perm = (perm >> _DIM) ^ (head << (8U * sizeof(_Tc) - _DIM));
                head = tail;
            }
        }

    public:

        wrap()
        {
            _num_resizes = 0U;
        }

        ~wrap() {}

        wrap(const wrap&) = delete;
        wrap(wrap&&) = delete;
        wrap& operator=(const wrap&) = delete;
        wrap& operator=(wrap&&) = delete;

        inline void insert(const coord_t& coord, const _Td& data)
        {
            coord_t permute;
            tools::permute<_Tc, _DIM>(&coord[0], &permute[0]);
            auto iter = std_map.find(permute);
            if (iter != std_map.end())
                merge((*iter).second, data);
            else
                std_map[permute] = data;
        }

        inline void resize()
        {
            ++_num_resizes;
            map_t new_map;
            auto iter = std_map.begin();
            auto end  = std_map.end();
            coord_t current = (*iter).first;
            _shift_(current);
            _Td data = (*iter).second;
            while (++iter != end)
            {
                coord_t permuted = (*iter).first;
                _shift_(permuted);
                if (current == permuted)
                {
                    merge(data, (*iter).second);
                }
                else
                {
                    new_map[current] = std::move(data);
                    current = std::move(permuted);
                    data = std::move((*iter).second);
                }
            }
            assert(new_map.find(current) == new_map.end());
            new_map[current] = data;
            std_map.swap(new_map);
        }

        inline uint8_t num_resizes() const { return _num_resizes; }

        inline size_t size() const { return std_map.size(); }

        inline typename map_t::iterator begin() noexcept { return std_map.begin(); }
        inline typename map_t::iterator end()   noexcept { return std_map.end();   }

        inline coord_t coord(const typename map_t::iterator& iter)
        {
            coord_t unravel;
            tools::unravel<_Tc, _DIM>(&((*iter).first[0]), &unravel[0]);
            return unravel;
        }

};


} // End of namespace tools



