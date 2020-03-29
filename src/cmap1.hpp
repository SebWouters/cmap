/*
    cmap is a collapsible hierarchically ordered coordinate map

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
#include <functional>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace cmap1
{


namespace _types
{

template<class _Tc, class _Td, size_t _DIM>
struct pair_t
    {
        std::array<_Tc, _DIM> coord;  // _Tc of type uint{8,16,32,64,128,256}_t and _DIM <= 8U
        _Td                   data;   // _Td any data type
    };


template<size_t _DIM>
struct node_t
    {
        std::unique_ptr<std::vector<size_t>>                     _indices;   // list indices
        std::unique_ptr<std::array<node_t<_DIM>, (1U << _DIM)>>  _children;  // kd-tree
        uint8_t                                                  _level;     // _level allows for bit numbering of _Tc
    };


template<class _Tc, class _Td, size_t _DIM>
struct ref_t
    {
        pair_t<_Tc, _Td, _DIM> _p;
        node_t<_DIM> *         _n; // ref == _list[(*(ref._n->_indices))[ref._i]]
        uint8_t                _i; // pointer to node_t::_children element: _i < (1U << _DIM)
    };

} // End of namespace _types


namespace _tools
{

/*
    Sanity check for class types and sizes
*/
template<class _Tc>
bool _template_checks(const _Tc seven, const size_t DIM)
    {
        return (std::is_unsigned<_Tc>::value)     // _Tc of type bool or uint{8,16,32,64,128,256}_t
            && (!std::is_same<_Tc, bool>::value)  // _Tc not of type bool
            && (seven == 7U)
            && (DIM <= 8U)
            && ((1U << DIM) <= 256U)              // ref_t::_i < (1U << DIM)
            && (8U * sizeof(_Tc) <= 256U);        // node_t::_level allows for bit numbering of _Tc
    }


/*
    Divide the elements of coordinates by power(2, amount)
    Assumes _Tc to be of type uint{8,16,32,64,128,256}_t
*/
template<class _Tc, size_t _DIM>
inline void _shift(std::array<_Tc, _DIM>& coordinates, const uint8_t amount)
    {
        for (_Tc& element : coordinates)
        {
            element = element >> amount;
        }
    }


/*
    Return the child of node to which coordinates correspond
*/
template<class _Tc, size_t _DIM>
inline cmap1::_types::node_t<_DIM>& _child(const cmap1::_types::node_t<_DIM>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        assert(node._children);
        uint32_t child_idx = 0U;
        for (const _Tc& element : coordinates)
        {
            child_idx = (child_idx << 1U) | ((element >> node._level) & 1U);
        }
        return (*(node._children))[child_idx];
    }


/*
    Remove items from list for which ref_t::_n == nullptr
    Adjust node references to remaining items from list
*/
template<class _Tc, class _Td, size_t _DIM>
void _clean(std::vector<cmap1::_types::ref_t<_Tc, _Td, _DIM>>& list)
    {
        size_t new_idx = 0U;
        auto first  = list.begin();
        auto result = list.begin();
        const auto last = list.end();
        while (first != last)
        {
            if ((*first)._n)
            {
                assert(  (*first)._n->_indices  );
                assert(!((*first)._n->_children));
                assert((*first)._n->_indices->size() > (*first)._i);
                assert((*((*first)._n->_indices))[(*first)._i] == first - list.begin());
                (*((*first)._n->_indices))[(*first)._i] = new_idx;
                ++new_idx;
                *result = std::move(*first);
                ++result;
            }
            ++first;
        }
        list.erase(result, last);
    }


/*
    Find the matching node for novel recursively, and insert novel in the matchting node
*/
template<class _Tc, class _Td, size_t _DIM>
void _insert(cmap1::_types::node_t<_DIM>& node, std::vector<cmap1::_types::ref_t<_Tc, _Td, _DIM>>& list, const cmap1::_types::pair_t<_Tc, _Td, _DIM>& novel)
    {
        if (node._children)
        {
            assert(!node._indices);
            _insert(_child(node, novel.coord), list, novel);
        }
        else
        {
            assert(node._indices);

            // merge
            for (const size_t list_idx : *(node._indices))
            {
                cmap1::_types::pair_t<_Tc, _Td, _DIM>& pair = list[list_idx]._p;
                if (pair.coord == novel.coord)
                {
                    merge(pair.data, novel.data);
                    return;
                }
            }

            // add
            if (node._indices->size() < (1U << _DIM))
            {
                const uint8_t idx_idx = node._indices->size();
                node._indices->push_back(list.size());
                list.push_back({ novel, &node, idx_idx });
                return;
            }

            // children
            assert(node._level != 0U);
            const uint8_t child_level = node._level - 1U;
            node._children = std::make_unique<std::array<cmap1::_types::node_t<_DIM>, (1U << _DIM)>>();
            for (cmap1::_types::node_t<_DIM>& newchild : *(node._children))
            {
                newchild._indices  = std::make_unique<std::vector<size_t>>();
                newchild._children = nullptr;
                newchild._level    = child_level;
            }
            for (const size_t list_idx : *(node._indices))
            {
                cmap1::_types::ref_t<_Tc, _Td, _DIM>& ref = list[list_idx];
                cmap1::_types::node_t<_DIM>& child = _child(node, ref._p.coord);
                ref._n = &child;
                ref._i = child._indices->size();
                child._indices->push_back(list_idx);
            }
            node._indices.reset(nullptr);
            _insert(_child(node, novel.coord), list, novel);
        }
    }


/*
    Resize the nodes recursively: coordinates are divided by two & colliding data is merged
*/
template<class _Tc, class _Td, size_t _DIM>
void _resize(cmap1::_types::node_t<_DIM>& node, std::vector<cmap1::_types::ref_t<_Tc, _Td, _DIM>>& list)
    {
        if (node._indices)
        {
            assert(!node._children);
            for (const size_t lidx : *(node._indices))
            {
                _shift(list[lidx]._p.coord, 1U);
            }
            if (node._indices->size() > 1U)
            {
                auto head = node._indices->begin();
                auto  end = node._indices->end();
                uint8_t head_idx = 0U;
                while (head != end)
                {
                    cmap1::_types::ref_t<_Tc, _Td, _DIM>& ref = list[*head];
                    ++head;
                    ++head_idx;

                    auto iter = head;
                    auto keep = head;
                    uint8_t keep_idx = head_idx;
                    while (iter != end)
                    {
                        cmap1::_types::ref_t<_Tc, _Td, _DIM>& candidate = list[*iter];
                        if (ref._p.coord == candidate._p.coord)
                        {
                            merge(ref._p.data, candidate._p.data);
                            candidate._n = nullptr;
                        }
                        else
                        {
                            *keep = std::move(*iter);
                            candidate._i = keep_idx;
                            ++keep_idx;
                            ++keep;
                        }
                        ++iter;
                    }
                    end = keep;
                }
                node._indices->erase(end, node._indices->end());
            }
        }
        else
        {
            assert(node._children);
            if (node._level == 1U)
            {
                node._indices = std::make_unique<std::vector<size_t>>();
                for (cmap1::_types::node_t<_DIM>& child : *(node._children))
                {
                    assert( child._indices);
                    assert(!child._children);
                    if (child._indices->size() != 0U)
                    {
                        const size_t first_idx = *(child._indices->begin());
                        cmap1::_types::ref_t<_Tc, _Td, _DIM>& ref = list[first_idx];
                        _shift(ref._p.coord, 1U);
                        for (auto iter = child._indices->begin() + 1U; iter != child._indices->end(); ++iter)
                        {
                            cmap1::_types::ref_t<_Tc, _Td, _DIM>& remove = list[*iter];
                            merge(ref._p.data, remove._p.data);
                            remove._n = nullptr;
                        }
                        ref._n = &node;
                        ref._i = node._indices->size();
                        node._indices->push_back(first_idx);
                    }
                }
                node._children.reset(nullptr);
            }
            else
            {
                assert(node._level > 1U);
                for (cmap1::_types::node_t<_DIM>& child : *(node._children))
                {
                    _resize(child, list);
                }
            }
        }

        assert(node._level != 0U);
        node._level--;
    }

} // End of namespace _tools



template<class _Tc, class _Td, size_t _DIM>
class cmap
{

    public:

        using coord_t = std::array<_Tc, _DIM>;
        using pair_t  = cmap1::_types::pair_t<_Tc, _Td, _DIM>;
        using node_t  = cmap1::_types::node_t<_DIM>;
        using ref_t   = cmap1::_types::ref_t<_Tc, _Td, _DIM>;

    private:

        uint8_t _num_shifts;
        std::unique_ptr<node_t> _root;
        std::vector<ref_t> _list;

    public:

        cmap()
        {
            assert(cmap1::_tools::_template_checks(static_cast<_Tc>(7U), _DIM));
            _num_shifts = 0U;
            _root = std::make_unique<node_t>();
            _root->_indices  = std::make_unique<std::vector<size_t>>();
            _root->_children = nullptr;
            _root->_level    = 8U * sizeof(_Tc) - 1U;
        }

        ~cmap() {}

        cmap(const cmap&) = delete;
        cmap(cmap&&) = delete;
        cmap& operator=(const cmap&) = delete;
        cmap& operator=(cmap&&) = delete;

        void insert(const coord_t& coord_in, const _Td& data_in)
        {
            pair_t novel = { coord_in, data_in };
            cmap1::_tools::_shift(novel.coord, _num_shifts);
            cmap1::_tools::_insert(*_root, _list, novel);
        }

        void resize()
        {
            cmap1::_tools::_resize(*_root, _list);            
            cmap1::_tools::_clean(_list);
            ++_num_shifts;
        }

        std::vector<pair_t> collect() const
        {
            std::vector<pair_t> result;
            result.reserve(_list.size());
            for (const ref_t& element : _list)
            {
                assert(element._n);
                result.push_back(element._p);
            }
            return result;
        }

        inline uint8_t num_resizes() const { return _num_shifts; }
        inline size_t  size() const { return _list.size(); }

    public:

        class const_iterator
        {
            using ref_iter = typename std::vector<ref_t>::const_iterator;

            private:

                ref_iter _iter;

            public:

                const_iterator(ref_iter _in) : _iter(_in) {}
                const_iterator(const const_iterator& _in) : _iter(_in._iter) {}
                const_iterator& operator++() { ++_iter; return *this; }
                const_iterator  operator++(int) { const_iterator returnval = *this; ++_iter; return returnval; }
                const pair_t&   operator*()  { return _iter->_p; }
                const pair_t&   operator->() { return _iter->_p; }
                bool            operator==(const_iterator other) const { return _iter == other._iter; }
                bool            operator!=(const_iterator other) const { return _iter != other._iter; }
        };

        const_iterator begin() const
        {
            return const_iterator(_list.begin());
        }

        const_iterator end() const
        {
            return const_iterator(_list.end());
        }


};


}


