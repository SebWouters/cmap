cmap is a resizable coordinate map
----------------------------------

```cmap<_Tc, _DIM, _Td>``` provides an implementation in the style of
```std::map``` associating:

* keys = coordinates (```coord_t = std::array<_Tc, _DIM>```), and
* values = data (```_Td```).

cmap automatically merges values associated with coordinate collisions
according to a user-implemented function
```void merge(_Td& left, const _Td& right)```. cmap furthermore provides
a function ```void resize()```, which

* divides each coordinate by two, and
* merges values associated with adjusted coordinate collisions.

After ```resize()```, the **resized** coordinates should be used
w.r.t. cmap.

cmap provides the following functionality:

* ```void insert(const coord_t& coord, const _Td& data)```
* ```void emplace(const coord_t& coord, _Ts&& ... args)```
* ```void resize()```
* ```uint8_t num_resizes() const```
* ```size_t size() const```
* ```bool empty() const```
* ```void prune() const```
* ```void clear()```
* ```iterator begin() const```
* ```iterator end() const```
* ```const_iterator cbegin() const```
* ```const_iterator cend() const```
* ```reverse_iterator rbegin() const```
* ```reverse_iterator rend() const```
* ```const_reverse_iterator crbegin() const```
* ```const_reverse_iterator crend() const```
* ```iterator find(const coord_t& coord) const```
* ```_Td& operator[](const coord_t& coord)```
* ```bool contains(const coord_t& coord) const```
* ```size_t erase(const coord_t& coord)```
* ```size_t erase(const const_iterator& iter)```
* ```size_t erase(const const_iterator& first, const const_iterator& stop)```

Examples can be found in ```tests/test{2,3,4,5}.cpp```.

Bugs, remarks & questions
-------------------------

--> sebastianwouters [at] gmail [dot] com

Copyright
---------

Copyright (c) 2020, Sebastian Wouters

All rights reserved.

cmap is licensed under the BSD 3-Clause License. A copy of the License
can be found in the file LICENSE in the root folder of this project.

