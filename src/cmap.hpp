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

namespace _cmap
{


namespace _types
{

template<class _Tc, class _Td, size_t _DIM>
struct pair_t
    {
        std::array<_Tc, _DIM> coord;  // _Tc of type uint{8,16,32,64,128,256}_t and _DIM <= 8U
        _Td                   data;   // _Td any data type
    };


template<class _Tc, class _Td, size_t _DIM>
struct node_t
    {
        node_t<_Tc, _Td, _DIM> *                                           _parent;
        std::unique_ptr<std::vector<pair_t<_Tc, _Td, _DIM>>>               _data;      // list indices
        std::unique_ptr<std::array<node_t<_Tc, _Td, _DIM>, (1U << _DIM)>>  _children;  // tree (e.g. 3D = 2**3 octants)
        uint8_t                                                            _level;     // _level allows for bit numbering of _Tc
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
            && (seven == 7U)                      // double check _Tc not of type bool
            && (DIM <= 8U)
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
template<class _Tc, class _Td, size_t _DIM>
inline _cmap::_types::node_t<_Tc, _Td, _DIM>& _child(const _cmap::_types::node_t<_Tc, _Td, _DIM>& node, const std::array<_Tc, _DIM>& coordinates)
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
    Find the matching node for novel recursively, and insert novel in the matchting node
*/
template<class _Tc, class _Td, size_t _DIM>
size_t _insert(_cmap::_types::node_t<_Tc, _Td, _DIM>& node, const _cmap::_types::pair_t<_Tc, _Td, _DIM>& novel)
    {
        if (node._children)
        {
            assert(!node._data);
            return _insert(_child(node, novel.coord), novel);
        }
        else
        {
            assert(node._data);

            // merge
            for (_cmap::_types::pair_t<_Tc, _Td, _DIM>& pair : *(node._data))
            {
                if (pair.coord == novel.coord)
                {
                    merge(pair.data, novel.data);
                    return 0U;
                }
            }

            // add
            if (node._data->size() < (1U << _DIM))
            {
                node._data->push_back(std::move(novel));
                return 1U;
            }

            // children
            assert(node._level != 0U);
            const uint8_t child_level = node._level - 1U;
            node._children = std::make_unique<std::array<_cmap::_types::node_t<_Tc, _Td, _DIM>, (1U << _DIM)>>();
            for (_cmap::_types::node_t<_Tc, _Td, _DIM>& newchild : *(node._children))
            {
                newchild._data     = std::make_unique<std::vector<_cmap::_types::pair_t<_Tc, _Td, _DIM>>>();
              //newchild._data->reserve(1U << _DIM);
                newchild._children = nullptr;
                newchild._parent   = &node;
                newchild._level    = child_level;
            }
            for (const _cmap::_types::pair_t<_Tc, _Td, _DIM>& pair : *(node._data))
            {
                _child(node, pair.coord)._data->push_back(std::move(pair));
            }
            node._data.reset(nullptr);
            return _insert(_child(node, novel.coord), novel);
        }
    }


/*
    Resize the nodes recursively: coordinates are divided by two & colliding data is merged
*/
template<class _Tc, class _Td, size_t _DIM>
size_t _resize(_cmap::_types::node_t<_Tc, _Td, _DIM>& node)
    {
        size_t num_removed = 0U;
        if (node._data)
        {
            assert(!node._children);
            for (_cmap::_types::pair_t<_Tc, _Td, _DIM>& pair : *(node._data))
            {
                _shift(pair.coord, 1U);
            }
            if (node._data->size() > 1U)
            {
                for (auto iter = node._data->begin(); iter != node._data->end(); /*custom update*/)
                {
                    _cmap::_types::pair_t<_Tc, _Td, _DIM>& target = *iter;
                    ++iter;
                    node._data->erase(std::remove_if(iter, node._data->end(),
                            [&target, &num_removed](const _cmap::_types::pair_t<_Tc, _Td, _DIM>& remove)
                            {
                                if (target.coord == remove.coord)
                                {
                                    merge(target.data, remove.data);
                                    ++num_removed;
                                    return true;
                                }
                                return false;
                            }
                        ), node._data->end());
                }
            }
        }
        else
        {
            assert(node._children);
            if (node._level == 1U)
            {
                node._data = std::make_unique<std::vector<_cmap::_types::pair_t<_Tc, _Td, _DIM>>>();
                for (_cmap::_types::node_t<_Tc, _Td, _DIM>& child : *(node._children))
                {
                    assert( child._data);
                    assert(!child._children);
                    if (child._data->size() != 0U)
                    {
                        num_removed += child._data->size() - 1U;
                        _cmap::_types::pair_t<_Tc, _Td, _DIM>& target = (*(child._data))[0U];
                        _shift(target.coord, 1U);
                        for (auto iter = child._data->begin() + 1U; iter != child._data->end(); ++iter)
                        {
                            merge(target.data, (*iter).data);
                        }
                        node._data->push_back(std::move(target));
                    }
                }
                node._children.reset(nullptr);
            }
            else
            {
                assert(node._level > 1U);
                for (_cmap::_types::node_t<_Tc, _Td, _DIM>& child : *(node._children))
                {
                    num_removed += _resize(child);
                }
            }
        }

        assert(node._level != 0U);
        node._level--;
        return num_removed;
    }


/*
    Find the leftmost data holding node (with data)
*/
template<class _Tc, class _Td, size_t _DIM>
const _cmap::_types::node_t<_Tc, _Td, _DIM> * _leftmost(const _cmap::_types::node_t<_Tc, _Td, _DIM>& node)
    {
        if (node._children)
        {
            assert(!node._data);
            for (const _cmap::_types::node_t<_Tc, _Td, _DIM>& child : *(node._children))
            {
                if (child._children)
                {
                    assert(!child._data);
                    return _leftmost(child);
                }
                else
                {
                    assert(child._data);
                    if (child._data->size() != 0U)
                        return &child;
                }
            }
        }
        else
        {
            assert(node._data);
            if (node._data->size() != 0U)
                return &node;
        }
        assert(false);
        return nullptr;
    }


/*
    Goto the next data holding node (with data)
*/
template<class _Tc, class _Td, size_t _DIM>
const _cmap::_types::node_t<_Tc, _Td, _DIM> * _next(const _cmap::_types::node_t<_Tc, _Td, _DIM>& node)
    {
        if (node._parent == nullptr)
            return nullptr;

        // Position?
        assert(node._parent->_children);
        auto search = node._parent->_children->begin();
        auto    end = node._parent->_children->end();
        while (search != end)
        {
            if (&(*search) == &node)
                break;
            ++search;
        }
        assert(search != end);

        // Sibling has data?
        for (auto sibling = search + 1U; sibling != end; ++sibling)
        {
            if (sibling->_children)
            {
                assert(!sibling->_data);
                return _leftmost(*sibling); // Search down the sibling branch
            }
            else
            {
                assert(sibling->_data);
                if (sibling->_data->size() != 0U)
                    return &(*sibling);
            }
        }

        // No data in siblings of node --> siblings of parent?
        return _next(*(node._parent));
    }


} // End of namespace _tools



template<class _Tc, class _Td, size_t _DIM>
class cmap
{

    public:

        using coord_t = std::array<_Tc, _DIM>;
        using pair_t  = _cmap::_types::pair_t<_Tc, _Td, _DIM>;
        using node_t  = _cmap::_types::node_t<_Tc, _Td, _DIM>;

    private:

        uint8_t _num_shifts;
        size_t  _size;
        std::unique_ptr<node_t> _root;

    public:

        cmap()
        {
            assert(_cmap::_tools::_template_checks(static_cast<_Tc>(7U), _DIM));
            _num_shifts = 0U;
            _size = 0U;
            _root = std::make_unique<node_t>();
            _root->_data     = std::make_unique<std::vector<pair_t>>();
            _root->_children = nullptr;
            _root->_parent   = nullptr;
            _root->_level    = 8U * sizeof(_Tc) - 1U;
            _root->_data->reserve(1U << _DIM);
        }

        ~cmap() {}

        cmap(const cmap&) = delete;
        cmap(cmap&&) = delete;
        cmap& operator=(const cmap&) = delete;
        cmap& operator=(cmap&&) = delete;

        void insert(const coord_t& coord_in, const _Td& data_in)
        {
            pair_t novel = { coord_in, data_in };
            _cmap::_tools::_shift(novel.coord, _num_shifts);
            _size += _cmap::_tools::_insert(*_root, novel);
        }

        void resize()
        {
            _size -= _cmap::_tools::_resize(*_root);
            ++_num_shifts;
        }

        inline uint8_t num_resizes() const { return _num_shifts; }
        inline size_t  size() const { return _size; }

    public:

        class const_iterator
        {
            private:

                const node_t * _node;
                size_t         _elem;

            public:

                const_iterator(const node_t * node_in, const size_t elem_in) : _node(node_in), _elem(elem_in) {}
                const_iterator(const const_iterator& in) : _node(in._node), _elem(in._elem) {}
                const_iterator& operator++() { this->next_iter(); return *this; }
                const_iterator  operator++(int) { const_iterator returnval = *this; this->next_iter(); return returnval; }
                const pair_t&   operator*()  { return (*(_node->_data))[_elem]; }
                const pair_t&   operator->() { return (*(_node->_data))[_elem]; }
                bool            operator==(const_iterator other) const { return (_node == other._node) && (_elem == other._elem); }
                bool            operator!=(const_iterator other) const { return (_node != other._node) || (_elem != other._elem); }

            private:

                void next_iter()
                {
                    if (_node->_data->size() != _elem + 1U)
                    {
                        ++_elem;
                    }
                    else
                    {
                        _elem = 0U;
                        _node = _cmap::_tools::_next(*_node);
                    }
                }

        };

        const_iterator begin() const
        {
            if (_size == 0U)
                return end();
            return const_iterator(_cmap::_tools::_leftmost(*_root), 0U);
        }

        const_iterator end() const
        {
            return const_iterator(nullptr, 0U);
        }


};


}


