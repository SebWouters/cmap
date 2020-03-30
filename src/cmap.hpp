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


namespace _cmapbase
{


/*
    node_t<_Tc, _Td, _DIM>:
        * holds one of _children or _data, but not both
        * key = coordinates (std::array<_Tc, _DIM>)
        * value = data (_Td)
        * _Tc of type uint{8,16,32,64,128,256}_t
        * _DIM <= 8U
        * _level indicates which bit of _Tc to check in _child(...)
*/
template<class _Tc, class _Td, size_t _DIM>
struct node_t
    {
        node_t<_Tc, _Td, _DIM> *                                             _parent;
        std::unique_ptr<std::vector<std::pair<std::array<_Tc, _DIM>, _Td>>>  _data;
        std::unique_ptr<std::array<node_t<_Tc, _Td, _DIM>, (1U << _DIM)>>    _children;
        uint8_t                                                              _level;
    };


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
inline node_t<_Tc, _Td, _DIM>& _child(const node_t<_Tc, _Td, _DIM>& node, const std::array<_Tc, _DIM>& coordinates)
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
    Find the matching node for novel recursively, and insert novel in the matching node
*/
template<class _Tc, class _Td, size_t _DIM>
size_t _insert(node_t<_Tc, _Td, _DIM>& node, const std::pair<std::array<_Tc, _DIM>, _Td>& novel)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

        if (node._children)
        {
            assert(!node._data);
            return _insert(_child(node, novel.first), novel);
        }
        else
        {
            assert(node._data);

            // merge
            for (pair_t& target : *(node._data))
            {
                if (target.first == novel.first)
                {
                    merge(target.second, novel.second);
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
            node._children = std::make_unique<std::array<node_t, (1U << _DIM)>>();
            for (node_t& newchild : *(node._children))
            {
                newchild._data     = std::make_unique<std::vector<pair_t>>();
              //newchild._data->reserve(1U << _DIM);
                newchild._children = nullptr;
                newchild._parent   = &node;
                newchild._level    = child_level;
            }
            for (const pair_t& item : *(node._data))
            {
                _child(node, item.first)._data->push_back(std::move(item));
            }
            node._data.reset(nullptr);
            return _insert(_child(node, novel.first), novel);
        }
    }


/*
    Resize the nodes recursively: coordinates are divided by two & colliding data is merged
*/
template<class _Tc, class _Td, size_t _DIM>
size_t _resize(node_t<_Tc, _Td, _DIM>& node)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

        size_t num_removed = 0U;
        if (node._data)
        {
            assert(!node._children);
            for (pair_t& item : *(node._data))
            {
                _shift(item.first, 1U);
            }
            if (node._data->size() > 1U)
            {
                auto head = node._data->begin();
                auto end  = node._data->end();
                while (head != end)
                {
                    pair_t& target = *head;
                    ++head;
                    auto iter = head;
                    auto res  = head;
                    while (iter != end)
                    {
                        if (target.first == (*iter).first)
                        {
                            merge(target.second, (*iter).second);
                            ++num_removed;
                        }
                        else
                        {
                            *res = std::move(*iter);
                            ++res;
                        }
                        ++iter;
                    }
                    end = res;
                }
                node._data->erase(end, node._data->end());
            }
        }
        else
        {
            assert(node._children);
            if (node._level == 1U)
            {
                node._data = std::make_unique<std::vector<pair_t>>();
                for (node_t& child : *(node._children))
                {
                    assert( child._data);
                    assert(!child._children);
                    if (child._data->size() != 0U)
                    {
                        num_removed += child._data->size() - 1U;
                        pair_t& target = (*(child._data))[0U];
                        _shift(target.first, 1U);
                        for (auto iter = child._data->begin() + 1U; iter != child._data->end(); ++iter)
                        {
                            merge(target.second, (*iter).second);
                        }
                        node._data->push_back(std::move(target));
                    }
                }
                node._children.reset(nullptr);
            }
            else
            {
                assert(node._level > 1U);
                for (node_t& child : *(node._children))
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
const node_t<_Tc, _Td, _DIM> * _leftmost(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._children)
        {
            for (auto child = node._children->cbegin(); child != node._children->cend(); ++child)
            {
                if (child->_children)
                    return _leftmost(*child);
                else
                {
                    if (child->_data->size() != 0U)
                        return &(*child);
                }
            }
        }
        else
        {
            if (node._data->size() != 0U)
                return &node;
        }
        assert(false);
        return nullptr;
    }


/*
    Find the rightmost data holding node (with data)
*/
template<class _Tc, class _Td, size_t _DIM>
const node_t<_Tc, _Td, _DIM> * _rightmost(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._children)
        {
            for (auto child = node._children->crbegin(); child != node._children->crend(); ++child)
            {
                if (child->_children)
                    return _rightmost(*child);
                else
                {
                    if (child->_data->size() != 0U)
                        return &(*child);
                }
            }
        }
        else
        {
            if (node._data->size() != 0U)
                return &node;
        }
        assert(false);
        return nullptr;
    }


/*
    Goto the next data holding node from left to right (with data)
*/
template<class _Tc, class _Td, size_t _DIM>
const node_t<_Tc, _Td, _DIM> * _next_from_left(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._parent == nullptr)
            return nullptr;

        // Find node
        auto iter = node._parent->_children->cbegin();
        while (&(*iter) != &node)
            ++iter;

        // Sibling with data
        auto end = node._parent->_children->cend();
        for (++iter; iter != end; ++iter)
        {
            if (iter->_children)
                return _leftmost(*iter); // Search down the sibling branch
            else
            {
                if (iter->_data->size() != 0U)
                    return &(*iter);
            }
        }

        // Siblings of parent
        return _next_from_left(*(node._parent));
    }


/*
    Goto the next data holding node from right to left (with data)
*/
template<class _Tc, class _Td, size_t _DIM>
const node_t<_Tc, _Td, _DIM> * _next_from_right(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._parent == nullptr)
            return nullptr;

        // Find node
        auto iter = node._parent->_children->crbegin();
        while (&(*iter) != &node)
            ++iter;

        // Sibling with data
        auto end = node._parent->_children->crend();
        for (++iter; iter != end; ++iter)
        {
            if (iter->_children)
                return _rightmost(*iter); // Search down the sibling branch
            else
            {
                if (iter->_data->size() != 0U)
                    return &(*iter);
            }
        }

        // Siblings of parent
        return _next_from_right(*(node._parent));
    }


} // End of namespace _cmapbase



template<class _Tc, class _Td, size_t _DIM>
class cmap
{

    public:

        using coord_t = std::array<_Tc, _DIM>;
        using pair_t  = std::pair<coord_t, _Td>;
        using node_t  = _cmapbase::node_t<_Tc, _Td, _DIM>;

    private:

        uint8_t _num_shifts;
        size_t  _size;
        std::unique_ptr<node_t> _root;

    public:

        cmap()
        {
            assert(_cmapbase::_template_checks(static_cast<_Tc>(7U), _DIM));
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
            _cmapbase::_shift(novel.first, _num_shifts);
            _size += _cmapbase::_insert(*_root, novel);
        }

        void resize()
        {
            _size -= _cmapbase::_resize(*_root);
            ++_num_shifts;
        }

        inline uint8_t num_resizes() const { return _num_shifts; }
        inline size_t size() const { return _size; }
        inline bool empty() const { return _size == 0U; }

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
                bool            operator==(const_iterator other) const noexcept { return (_node == other._node) && (_elem == other._elem); }
                bool            operator!=(const_iterator other) const noexcept { return (_node != other._node) || (_elem != other._elem); }

            private:

                void next_iter()
                {
                    if (++_elem == _node->_data->size())
                    {
                        _elem = 0U;
                        _node = _cmapbase::_next_from_left(*_node);
                    }
                }

        };

        const_iterator begin() const noexcept
        {
            if (empty())
                return end();
            return const_iterator(_cmapbase::_leftmost(*_root), 0U);
        }

        const_iterator end() const noexcept
        {
            return const_iterator(nullptr, 0U);
        }

    public:

        class reverse_iterator
        {
            private:

                const node_t * _node;
                size_t         _elem;

            public:

                reverse_iterator(const node_t * node_in, const size_t elem_in) : _node(node_in), _elem(elem_in) {}
                reverse_iterator(const reverse_iterator& in) : _node(in._node), _elem(in._elem) {}
                reverse_iterator& operator++() { this->next_iter(); return *this; }
                reverse_iterator  operator++(int) { reverse_iterator returnval = *this; this->next_iter(); return returnval; }
                const pair_t&     operator*()  { return (*(_node->_data))[_elem]; }
                const pair_t&     operator->() { return (*(_node->_data))[_elem]; }
                bool              operator==(reverse_iterator other) const noexcept { return (_node == other._node) && (_elem == other._elem); }
                bool              operator!=(reverse_iterator other) const noexcept { return (_node != other._node) || (_elem != other._elem); }

            private:

                void next_iter()
                {
                    if (_elem == 0U)
                    {
                        _node = _cmapbase::_next_from_right(*_node);
                        _elem = (_node) ? _node->_data->size() - 1U : 0U;
                    }
                    else
                    {
                        --_elem;
                    }
                }

        };

        reverse_iterator rbegin() const noexcept
        {
            if (empty())
                return rend();
            const node_t * right = _cmapbase::_rightmost(*_root);
            return reverse_iterator(right, right->_data->size() - 1U);
        }

        reverse_iterator rend() const noexcept
        {
            return reverse_iterator(nullptr, 0U);
        }


};





