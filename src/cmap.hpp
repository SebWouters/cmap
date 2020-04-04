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


template<class _Tc, size_t _DIM, class _Td>
struct node_t;

template<class _Tc, size_t _DIM, class _Td>
using data_vec = std::vector<std::pair<std::array<_Tc, _DIM>, _Td>>;

template<class _Tc, size_t _DIM, class _Td>
using node_arr = std::array<node_t<_Tc, _DIM, _Td>, (1U << _DIM)>;


/*
    node_t<_Tc, _DIM, _Td>:
        * holds one of _children or _data, but not both
        * key = coordinates (std::array<_Tc, _DIM>)
        * value = data (_Td)
        * _Tc of type uint{8,16,32,64,128,256}_t
        * _DIM <= 8U
        * _level indicates which bit of _Tc to check in _child(...)
*/
template<class _Tc, size_t _DIM,  class _Td>
struct node_t
    {
        node_t<_Tc, _DIM, _Td> *                    _parent;
        std::unique_ptr<data_vec<_Tc, _DIM, _Td>>   _data;
        std::unique_ptr<node_arr<_Tc, _DIM, _Td>>   _children;
        uint8_t                                     _level;
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
    Divide the elements of coordinates by 2
    Assumes _Tc to be of type uint{8,16,32,64,128,256}_t
*/
template<class _Tc, size_t _DIM>
inline constexpr void _shift(std::array<_Tc, _DIM>& coordinates) noexcept
    {
        for (_Tc& element : coordinates)
            element = element >> 1U;
    }


/*
    Return the child of node to which coordinates correspond
*/
template<class _Tc, size_t _DIM, class _Td>
inline node_t<_Tc, _DIM, _Td>& _child(const node_t<_Tc, _DIM, _Td>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        assert(node._children);
        uint32_t child_idx = 0U;
        for (const _Tc& element : coordinates)
            child_idx = (child_idx << 1U) | ((element >> node._level) & 1U);
        return (*(node._children))[child_idx];
    }


/*
    Return the node to which coordinates correspond
*/
template<class _Tc, size_t _DIM, class _Td>
inline node_t<_Tc, _DIM, _Td>& _leaf(node_t<_Tc, _DIM, _Td>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        if (node._children)
            return _leaf(_child(node, coordinates), coordinates);
        else
            return node;
    }


/*
    Find a position of coordinates within a node's data
*/
template<class _Tc, size_t _DIM, class _Td>
inline typename data_vec<_Tc, _DIM, _Td>::iterator _pair(const node_t<_Tc, _DIM, _Td>& node, const std::array<_Tc, _DIM>& coordinates)
    {
        assert(node._data);
        auto iter = node._data->begin();
        auto  end = node._data->end();
        while ((iter != end) && ((*iter).first != coordinates))
            ++iter;
        return iter;
    }


/*
    Get the number of elements in the node or its leafs
*/
template<class _Tc, size_t _DIM, class _Td>
inline size_t _size(const node_t<_Tc, _DIM, _Td>& node)
    {
        if (node._data)
        {
            assert(!node._children);
            return node._data->size();
        }
        else
        {
            assert(node._children);
            size_t number = 0U;
            for (const auto& child : *(node._children))
                number += _size(child);
            return number;
        }
    }


/*
    Collect the data items from a node and its children
*/
template<class _Tc, size_t _DIM, class _Td>
inline void _collect(const node_t<_Tc, _DIM, _Td>& node, data_vec<_Tc, _DIM, _Td>& result)
    {
        if (node._data)
        {
            assert(!node._children);
            for (auto& item : *(node._data))
                result.push_back(std::move(item)); // TODO: insert with iterator range?
        }
        else
        {
            assert(node._children);
            for (auto& child : *(node._children))
                _collect(child, result);
        }
    }


/*
    Simplify the tree (after erase; top-down)
*/
template<class _Tc, size_t _DIM, class _Td>
inline void _prune(node_t<_Tc, _DIM, _Td>& node)
    {
        if (node._children)
        {
            const size_t number = _size(node); // TODO: Child holds its size to avoid recomputation
            if (number <= (1U << _DIM))
            {
                node._data = std::make_unique<data_vec<_Tc, _DIM, _Td>>();
                node._data->reserve(number);
                for (auto& child : *(node._children))
                    _collect(child, *(node._data));
                node._children.reset(nullptr);
                assert(number == node._data->size());
            }
            else
            {
                for (auto& child : *(node._children))
                    _prune(child);
            }
        }
    }


/*
    Split a node into children and distribute data
*/
template<class _Tc, size_t _DIM, class _Td>
inline void _split(node_t<_Tc, _DIM, _Td>& node)
    {
        assert(node._level != 0U);
        const uint8_t child_level = node._level - 1U;
        node._children = std::make_unique<node_arr<_Tc, _DIM, _Td>>();
        for (auto& newchild : *(node._children))
        {
            newchild._data     = std::make_unique<data_vec<_Tc, _DIM, _Td>>();
          //newchild._data->reserve(1U << _DIM);
            newchild._children = nullptr;
            newchild._parent   = &node;
            newchild._level    = child_level;
        }
        for (const auto& item : *(node._data))
            _child(node, item.first)._data->push_back(std::move(item));
        node._data.reset(nullptr);
    }


/*
    Insert (coord, data) in the node
*/
template<class _Tc, size_t _DIM, class _Td>
inline size_t _insert(node_t<_Tc, _DIM, _Td>& node, const std::array<_Tc, _DIM>& coord, const _Td& data)
    {
        assert(node._data);
        for (auto& target : *(node._data))
        {
            if (target.first == coord)
            {
                merge(target.second, data);
                return 0U;
            }
        }
        if (node._data->size() < (1U << _DIM))
        {
            node._data->push_back(std::make_pair(coord, data));
            return 1U;
        }
        _split(node);
        return _insert(_child(node, coord), coord, data);
    }


/*
    Emplace (coord, args) in the node
*/
template<class _Tc, size_t _DIM, class _Td, class ... _Ts>
inline size_t _emplace(node_t<_Tc, _DIM, _Td>& node, const std::array<_Tc, _DIM>& coord, _Ts&& ... args)
    {
        assert(node._data);
        for (auto& target : *(node._data))
        {
            if (target.first == coord)
            {
                merge(target.second, {args ...});
                return 0U;
            }
        }
        if (node._data->size() < (1U << _DIM))
        {
            node._data->emplace_back(std::piecewise_construct, std::forward_as_tuple(coord), std::forward_as_tuple(args ...));
            return 1U;
        }
        _split(node);
        return _emplace(_child(node, coord), coord, args ...);
    }


/*
    Merge pairs with identical coordinates
*/
template<class _Tc, size_t _DIM, class _Td>
inline size_t _merge(data_vec<_Tc, _DIM, _Td>& data)
    {
        size_t num_merged = 0U;
        if (data.size() > 1U)
        {
            auto head = data.begin();
            auto end  = data.end();
            while (head != end)
            {
                auto& target = *head;
                ++head;
                auto iter   = head;
                auto result = head;
                while (iter != end)
                {
                    if (target.first == (*iter).first)
                    {
                        merge(target.second, (*iter).second);
                        ++num_merged;
                    }
                    else
                    {
                        *result = std::move(*iter);
                        ++result;
                    }
                    ++iter;
                }
                end = result;
            }
            data.erase(end, data.end());
        }
        return num_merged;
    }


/*
    Resize the nodes recursively: coordinates are divided by two & colliding data is merged
*/
template<class _Tc, size_t _DIM, class _Td>
inline size_t _resize(node_t<_Tc, _DIM, _Td>& node)
    {
        size_t num_removed = 0U;
        if (node._data)
        {
            assert(!node._children);
            for (auto& item : *(node._data))
                _shift(item.first);
            num_removed = _merge(*(node._data));
        }
        else
        {
            assert(node._children);
            if (node._level == 1U)
            {
                node._data = std::make_unique<data_vec<_Tc, _DIM, _Td>>();
                for (auto& child : *(node._children))
                {
                    assert( child._data);
                    assert(!child._children);
                    if (child._data->size() != 0U)
                    {
                        num_removed += child._data->size() - 1U;
                        auto& target = (*(child._data))[0U];
                        _shift(target.first);
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
                for (auto& child : *(node._children))
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
template<typename _It, class _Tc, size_t _DIM, class _Td>
inline const node_t<_Tc, _DIM, _Td> * _down(const node_t<_Tc, _DIM, _Td>& node) noexcept
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
    Search the next node with data
*/
template<typename _It, class _Tc, size_t _DIM, class _Td>
inline const node_t<_Tc, _DIM, _Td> * _next(const node_t<_Tc, _DIM, _Td>& node) noexcept
    {
        if (node._parent == nullptr)
            return nullptr;

        _It iter = _begin<_It>(*(node._parent->_children));
        _It  end =   _end<_It>(*(node._parent->_children));
        while (&(*iter) != &node)
            ++iter;

        for (++iter; iter != end; ++iter)
        {
            if (iter->_children)
                return _down<_It>(*iter);
            else
            {
                if (iter->_data->size() != 0U)
                    return &(*iter);
            }
        }
        return _next<_It>(*(node._parent));
    }


} } // End of namespaces _cmapbase and {anonymous}



template<class _Tc, size_t _DIM, class _Td>
class cmap {

    public:

        typedef std::array<_Tc, _DIM>             coord_t;
        typedef std::pair<coord_t, _Td>           pair_t;
        typedef _cmapbase::node_t<_Tc, _DIM, _Td> node_t;

    private:

        typedef _cmapbase::data_vec<_Tc, _DIM, _Td> data_vec;
        typedef _cmapbase::node_arr<_Tc, _DIM, _Td> node_arr;

        uint8_t _num_resizes;
        size_t  _size;
        std::unique_ptr<node_t> _root;

        template<class _Type, typename _vIt>
        class _iterator_base
        {
            private:

                const node_t * _node;
                _vIt           _vitr;

                template <class T = void>
                inline typename std::enable_if<std::is_same<_vIt, typename data_vec::iterator>::value, T>::type update()
                {
                    assert(_node);
                    if (++_vitr == _node->_data->end())
                    {
                        _node = _cmapbase::_next<typename node_arr::const_iterator>(*_node);
                        _vitr = (_node) ? _node->_data->begin() : vvoid();
                    }
                }

                template <class T = void>
                inline typename std::enable_if<std::is_same<_vIt, typename data_vec::reverse_iterator>::value, T>::type update()
                {
                    assert(_node);
                    if (++_vitr == _node->_data->rend())
                    {
                        _node = _cmapbase::_next<typename node_arr::const_reverse_iterator>(*_node);
                        _vitr = (_node) ? _node->_data->rbegin() : vvoid();
                    }
                }

                static _vIt vvoid() { return _vIt(); }

            public:

                _iterator_base() : _node(nullptr), _vitr(vvoid()) {}
                _iterator_base(const node_t * node_in, _vIt vitr_in) : _node(node_in), _vitr(vitr_in) {}
                _iterator_base(const _iterator_base& in) : _node(in._node), _vitr(in._vitr) {}
                _iterator_base& operator++() { this->update(); return *this; }
                _iterator_base  operator++(int) { _iterator_base returnval = *this; this->update(); return returnval; }
                bool operator==(_iterator_base other) const noexcept { return (_node == other._node) && (_vitr == other._vitr); }
                bool operator!=(_iterator_base other) const noexcept { return (_node != other._node) || (_vitr != other._vitr); }
                const node_t * node() const { return _node; }
                _vIt viter() const { return _vitr; }

                template <class T = const pair_t&>
                inline typename std::enable_if<std::is_const<_Type>::value, T>::type operator*() const { return *_vitr; }

                template <class T = std::pair<const coord_t&, _Td&>>
                inline typename std::enable_if<!std::is_const<_Type>::value, T>::type operator*() const { return { (*_vitr).first, (*_vitr).second }; }

                template <class T = const pair_t&>
                inline typename std::enable_if<std::is_const<_Type>::value, T>::type operator->() const { return *_vitr; }

                template <class T = std::pair<const coord_t&, _Td&>>
                inline typename std::enable_if<!std::is_const<_Type>::value, T>::type operator->() const { return { (*_vitr).first, (*_vitr).second }; }

                operator _iterator_base<const _Type, _vIt>() const { return _iterator_base<const _Type, _vIt>(_node, _vitr); }
        };

    public:

        typedef _iterator_base<      pair_t, typename data_vec::iterator> iterator;
        typedef _iterator_base<const pair_t, typename data_vec::iterator> const_iterator;
        typedef _iterator_base<      pair_t, typename data_vec::reverse_iterator> reverse_iterator;
        typedef _iterator_base<const pair_t, typename data_vec::reverse_iterator> const_reverse_iterator;

        cmap() { clear(); }

        ~cmap() {}

        cmap(const cmap&) = delete;
        cmap(cmap&&) = delete;
        cmap& operator=(const cmap&) = delete;
        cmap& operator=(cmap&&) = delete;

        inline void insert(const coord_t& coord, const _Td& data)
        {
            _size += _cmapbase::_insert(_leaf(*_root, coord), coord, data);
        }

        template<class ... _Ts>
        inline void emplace(const coord_t& coord, _Ts&& ... args)
        {
            _size += _cmapbase::_emplace(_leaf(*_root, coord), coord, args ...);
        }

        inline void resize()
        {
            _size -= _cmapbase::_resize(*_root);
            ++_num_resizes;
        }

        inline uint8_t num_resizes() const { return _num_resizes; }

        inline size_t size() const { return _size; }

        inline bool empty() const { return _size == 0U; }

        inline void prune() const { _prune(*_root); }

        inline void clear()
        {
            assert(_cmapbase::_template_checks(static_cast<_Tc>(7U), _DIM));
            _num_resizes = 0U;
            _size = 0U;
            if (_root){ _root.reset(nullptr); }
            _root = std::make_unique<node_t>();
            _root->_data     = std::make_unique<data_vec>();
            _root->_children = nullptr;
            _root->_parent   = nullptr;
            _root->_level    = 8U * sizeof(_Tc) - 1U;
            _root->_data->reserve(1U << _DIM);
        }

        inline iterator begin() const noexcept
        {
            if (empty())
                return end();
            const node_t * first = _cmapbase::_down<typename node_arr::const_iterator>(*_root);
            return iterator(first, first->_data->begin());
        }

        inline const_iterator cbegin() const noexcept { return begin(); }
        inline iterator          end() const noexcept { return iterator(); }
        inline const_iterator   cend() const noexcept { return end(); }

        inline reverse_iterator rbegin() const noexcept
        {
            if (empty())
                return rend();
            const node_t * last = _cmapbase::_down<typename node_arr::const_reverse_iterator>(*_root);
            return reverse_iterator(last, last->_data->rbegin());
        }

        inline const_reverse_iterator crbegin() const noexcept { return rbegin(); }
        inline       reverse_iterator    rend() const noexcept { return reverse_iterator(); }
        inline const_reverse_iterator   crend() const noexcept { return rend(); }

        inline iterator find(const coord_t& coord) const
        {
            const node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
                return end();
            else
                return iterator(&leaf, pos);
        }

        inline _Td& operator[](const coord_t& coord)
        {
            node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
            {
                _size += _cmapbase::_insert(leaf, coord, _Td());
                pos = _cmapbase::_pair(_cmapbase::_leaf(leaf, coord), coord);
            }
            return (*pos).second;
        }

        inline bool contains(const coord_t& coord) const
        {
            const node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            return pos != leaf._data->end();
        }

        inline size_t erase(const coord_t& coord)
        {
            node_t& leaf = _cmapbase::_leaf(*_root, coord);
            auto pos = _cmapbase::_pair(leaf, coord);
            if (pos == leaf._data->end())
                return 0U;
            leaf._data->erase(pos);
            --_size;
            _cmapbase::_prune(*_root);
            assert(_size == _cmapbase::_size(*_root));
            return 1U;
        }

        inline size_t erase(const const_iterator& iter)
        {
            if (iter.node() == nullptr)
                return 0U;
            iter.node()->_data->erase(iter.viter());
            --_size;
            _cmapbase::_prune(*_root);
            assert(_size == _cmapbase::_size(*_root));
            return 1U;
        }

        inline size_t erase(const const_iterator& first, const const_iterator& stop)
        {
            const_iterator end  = cend();
            const_iterator iter = const_iterator(first);
            size_t number = 0U;
            while ((iter != end) && (iter != stop))
            {
                auto dbegin = iter.viter();
                auto dend   = (iter.node() == stop.node()) ? stop.viter() : iter.node()->_data->end();
                number += dend - dbegin;
                iter.node()->_data->erase(dbegin, dend);
                const node_t * next = _cmapbase::_next<typename node_arr::const_iterator>(*iter.node());
                iter = ((iter.node() != stop.node()) && next) ? const_iterator(next, next->_data->begin()) : end;
            }
            _size -= number;
            _cmapbase::_prune(*_root);
            assert(_size == _cmapbase::_size(*_root));
            return number;
        }


};


} // End of namespace tools



