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


namespace tools {

namespace { namespace _cmapbase {


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
inline bool _template_checks(const _Tc seven, const size_t DIM)
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
inline constexpr void _shift(std::array<_Tc, _DIM>& coordinates, const uint8_t amount) noexcept
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
    Return the node to which coordinates correspond
*/
template<class _Tc, class _Td, size_t _DIM>
inline node_t<_Tc, _Td, _DIM>& _leaf(node_t<_Tc, _Td, _DIM>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        if (node._children)
            return _leaf(_child(node, coordinates), coordinates);
        else
            return node;
    }


/*
    Find a position of coordinates within a node's data
*/
template<class _Tc, class _Td, size_t _DIM>
inline typename std::vector<std::pair<std::array<_Tc, _DIM>, _Td>>::iterator _pair(const node_t<_Tc, _Td, _DIM>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        using _data_iter = typename std::vector<std::pair<std::array<_Tc, _DIM>, _Td>>::iterator;

        assert(node._data);
        _data_iter iter = node._data->begin();
        _data_iter  end = node._data->end();
        while (((*iter).first != coordinates) && (iter != end))
            ++iter;
        return iter;
    }


/*
    Get the number of elements the node and its children hold
*/
template <class _Tc, class _Td, size_t _DIM>
inline size_t _size(const node_t<_Tc, _Td, _DIM>& node)
    {
        using node_t = node_t<_Tc, _Td, _DIM>;

        if (node._data)
        {
            assert(!node._children);
            return node._data->size();
        }
        else
        {
            assert(node._children);
            size_t number = 0U;
            for (const node_t& child : *(node._children))
                number += _size(child);
            return number;
        }
    }


/*
    Collect the data items from a node and its children
*/
template <class _Tc, class _Td, size_t _DIM>
inline void _collect(node_t<_Tc, _Td, _DIM>& node, std::vector<std::pair<std::array<_Tc, _DIM>, _Td>>& result)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

        if (node._data)
        {
            assert(!node._children);
            for (pair_t& item : *(node._data))
                result.push_back(std::move(item));
        }
        else
        {
            assert(node._children);
            for (node_t& child : *(node._children))
                _collect(child, result);
        }
    }


/*
    Insert novel in the node
*/
template<class _Tc, class _Td, size_t _DIM>
inline size_t _insert(node_t<_Tc, _Td, _DIM>& node, const std::pair<std::array<_Tc, _DIM>, _Td>& novel)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

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
            _child(node, item.first)._data->push_back(std::move(item));
        node._data.reset(nullptr);
        return _insert(_child(node, novel.first), novel);
    }


/*
    Resize the nodes recursively: coordinates are divided by two & colliding data is merged
*/
template<class _Tc, class _Td, size_t _DIM>
inline size_t _resize(node_t<_Tc, _Td, _DIM>& node)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

        size_t num_removed = 0U;
        if (node._data)
        {
            assert(!node._children);
            for (pair_t& item : *(node._data))
                _shift(item.first, 1U);
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
                            merge(target.second, (*iter).second);
                        node._data->push_back(std::move(target));
                    }
                }
                node._children.reset(nullptr);
            }
            else
            {
                assert(node._level > 1U);
                for (node_t& child : *(node._children))
                    num_removed += _resize(child);
            }
        }

        assert(node._level != 0U);
        node._level--;
        return num_removed;
    }


template<typename _It, class _Ta>
inline typename std::enable_if<std::is_same<_It, typename _Ta::const_iterator>::value, _It>::type _begin(_Ta& in)
{
    return in.cbegin();
}

template<typename _It, class _Ta>
inline typename std::enable_if<std::is_same<_It, typename _Ta::const_reverse_iterator>::value, _It>::type _begin(_Ta& in)
{
    return in.crbegin();
}

template<typename _It, class _Ta>
inline typename std::enable_if<std::is_same<_It, typename _Ta::const_iterator>::value, _It>::type _end(_Ta& in)
{
    return in.cend();
}

template<typename _It, class _Ta>
inline typename std::enable_if<std::is_same<_It, typename _Ta::const_reverse_iterator>::value, _It>::type _end(_Ta& in)
{
    return in.crend();
}


/*
    Search data down the tree
*/
template<typename _It, class _Tc, class _Td, size_t _DIM>
inline const node_t<_Tc, _Td, _DIM> * _down(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._children)
        {
            _It child = _begin<_It>(*(node._children));
            _It   end =   _end<_It>(*(node._children));
            for (; child != end; ++child)
            {
                if (child->_children)
                    return _down<_It>(*child);
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
    Search following data
*/
template<typename _It, class _Tc, class _Td, size_t _DIM>
inline const node_t<_Tc, _Td, _DIM> * _further(const node_t<_Tc, _Td, _DIM>& node) noexcept
    {
        if (node._parent == nullptr)
            return nullptr;

        // Find node
        _It iter = _begin<_It>(*(node._parent->_children));
        _It  end =   _end<_It>(*(node._parent->_children));
        while (&(*iter) != &node)
            ++iter;

        // Sibling with data
        for (++iter; iter != end; ++iter)
        {
            if (iter->_children)
                return _down<_It>(*iter); // Search down the sibling branch
            else
            {
                if (iter->_data->size() != 0U)
                    return &(*iter);
            }
        }

        // Siblings of parent
        return _further<_It>(*(node._parent));
    }


/*
    Simplify the tree (after erase)
*/
template<class _Tc, class _Td, size_t _DIM>
inline void _simplify(node_t<_Tc, _Td, _DIM>& leaf)
    {
        using pair_t = std::pair<std::array<_Tc, _DIM>, _Td>;
        using node_t = node_t<_Tc, _Td, _DIM>;

        assert(leaf._data);

        if (leaf._parent == nullptr)
            return;

        if (_size(*(leaf._parent)) <= (1U << _DIM))
        {
            node_t& parent = *(leaf._parent); // Need to make a copy for the _simplify call
            parent._data = std::make_unique<std::vector<pair_t>>();
            for (node_t& sibling : *(parent._children))
                _collect(sibling, *(parent._data));
            parent._children.reset(nullptr);
            _simplify(parent);
        }
    }


} } // End of namespaces _cmapbase and {anonymous}



template<class _Tc, class _Td, size_t _DIM>
class cmap {

    public:

        typedef std::array<_Tc, _DIM>             coord_t;
        typedef std::pair<coord_t, _Td>           pair_t;
        typedef _cmapbase::node_t<_Tc, _Td, _DIM> node_t;

    private:

        uint8_t _num_shifts;
        size_t  _size;
        std::unique_ptr<node_t> _root;

        template<class _Type, size_t _Forward>
        class _iterator_base
        {
            private:

                const node_t * _node;
                size_t         _elem;

                template <class T = void>
                inline typename std::enable_if<_Forward == 1, T>::type update()
                {
                    if (++_elem == _node->_data->size())
                    {
                        _node = _cmapbase::_further<typename std::array<node_t, (1U << _DIM)>::const_iterator>(*_node);
                        _elem = 0U;
                    }
                }

                template <class T = void>
                inline typename std::enable_if<_Forward == 0, T>::type update()
                {
                    if (_elem == 0U)
                    {
                        _node = _cmapbase::_further<typename std::array<node_t, (1U << _DIM)>::const_reverse_iterator>(*_node);
                        _elem = (_node) ? _node->_data->size() - 1U : 0U;
                    }
                    else
                        --_elem;
                }

            public:

                _iterator_base(const node_t * node_in, const size_t elem_in) : _node(node_in), _elem(elem_in) {}
                _iterator_base(const _iterator_base& in) : _node(in._node), _elem(in._elem) {}
                _iterator_base& operator++() { this->update(); return *this; }
                _iterator_base  operator++(int) { _iterator_base returnval = *this; this->update(); return returnval; }
                bool            operator==(_iterator_base other) const noexcept { return (_node == other._node) && (_elem == other._elem); }
                bool            operator!=(_iterator_base other) const noexcept { return (_node != other._node) || (_elem != other._elem); }
                const node_t * node() const { return _node; }
                size_t elem() const { return _elem; }

                template <class T = const pair_t&>
                inline typename std::enable_if<std::is_const<_Type>::value, T>::type operator*() { return (*(_node->_data))[_elem]; }

                template <class T = std::pair<const coord_t&, _Td&>>
                inline typename std::enable_if<!std::is_const<_Type>::value, T>::type operator*() { return { (*(_node->_data))[_elem].first, (*(_node->_data))[_elem].second }; }

                template <class T = const pair_t&>
                inline typename std::enable_if<std::is_const<_Type>::value, T>::type operator->() { return (*(_node->_data))[_elem]; }

                template <class T = std::pair<const coord_t&, _Td&>>
                inline typename std::enable_if<!std::is_const<_Type>::value, T>::type operator->() { return { (*(_node->_data))[_elem].first, (*(_node->_data))[_elem].second }; }

                operator _iterator_base<const _Type, _Forward>() const { return _iterator_base<const _Type, _Forward>(_node, _elem); }
        };

    public:

        typedef _iterator_base<      pair_t, 1> iterator;
        typedef _iterator_base<const pair_t, 1> const_iterator;
        typedef _iterator_base<      pair_t, 0> reverse_iterator;
        typedef _iterator_base<const pair_t, 0> const_reverse_iterator;

        cmap() { clear(); }

        ~cmap() {}

        cmap(const cmap&) = delete;
        cmap(cmap&&) = delete;
        cmap& operator=(const cmap&) = delete;
        cmap& operator=(cmap&&) = delete;

        inline void insert(const coord_t& coord_in, const _Td& data_in)
        {
            pair_t novel = { coord_in, data_in };
            _cmapbase::_shift(novel.first, _num_shifts);
            _size += _cmapbase::_insert(_leaf(*_root, novel.first), novel);
        }

        inline void resize()
        {
            _size -= _cmapbase::_resize(*_root);
            ++_num_shifts;
        }

        inline uint8_t num_resizes() const { return _num_shifts; }

        inline size_t size() const { return _size; }

        inline bool empty() const { return _size == 0U; }

        inline void clear()
        {
            assert(_cmapbase::_template_checks(static_cast<_Tc>(7U), _DIM));
            _num_shifts = 0U;
            _size = 0U;
            if (_root){ _root.reset(nullptr); }
            _root = std::make_unique<node_t>();
            _root->_data     = std::make_unique<std::vector<pair_t>>();
            _root->_children = nullptr;
            _root->_parent   = nullptr;
            _root->_level    = 8U * sizeof(_Tc) - 1U;
            _root->_data->reserve(1U << _DIM);
        }

        inline iterator begin() const noexcept
        {
            if (empty())
                return end();
            const node_t * first = _cmapbase::_down<typename std::array<node_t, (1U << _DIM)>::const_iterator>(*_root);
            return iterator(first, 0U);
        }

        inline const_iterator cbegin() const noexcept { return begin(); }
        inline iterator          end() const noexcept { return iterator(nullptr, 0U); }
        inline const_iterator   cend() const noexcept { return end(); }

        inline reverse_iterator rbegin() const noexcept
        {
            if (empty())
                return rend();
            const node_t * last = _cmapbase::_down<typename std::array<node_t, (1U << _DIM)>::const_reverse_iterator>(*_root);
            return reverse_iterator(last, last->_data->size() - 1U);
        }

        inline const_reverse_iterator crbegin() const noexcept { return rbegin(); }
        inline       reverse_iterator    rend() const noexcept { return reverse_iterator(nullptr, 0U); }
        inline const_reverse_iterator   crend() const noexcept { return rend(); }

        inline iterator find(const coord_t& coord_in) const
        {
            coord_t coord = coord_in;
            _cmapbase::_shift(coord, _num_shifts);
            const node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
                return iterator(nullptr, 0U);
            else
                return iterator(&leaf, pos - leaf._data->begin());
        }

        inline _Td& operator[](const coord_t& coord_in)
        {
            coord_t coord = coord_in;
            _cmapbase::_shift(coord, _num_shifts);
            node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
            {
                _size += _cmapbase::_insert(leaf, { coord, _Td() });
                pos = _cmapbase::_pair(_cmapbase::_leaf(leaf, coord), coord);
            }
            return (*pos).second;
        }

        inline bool contains(const coord_t& coord_in) const
        {
            coord_t coord = coord_in;
            _cmapbase::_shift(coord, _num_shifts);
            const node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            return (pos == leaf._data->end()) ? false : true;
        }

        inline size_t erase(const coord_t& coord_in)
        {
            coord_t coord = coord_in;
            _cmapbase::_shift(coord, _num_shifts);
            node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
                return 0U;
            leaf._data->erase(pos);
            --_size;
            _cmapbase::_simplify(leaf);
            assert(_size == _cmapbase::_size(*_root));
            return 1U;
        }


};


} // End of namespace tools



