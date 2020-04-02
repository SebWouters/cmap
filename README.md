cmap is a resizable coordinate map
----------------------------------

```cmap<_Tc, _DIM, _Td>``` provides an implementation in the style of
```std::map``` holding:

* keys = coordinates (```std::array<_Tc, _DIM>```), associated with
* values = data (```_Td```).

cmap automatically merges values associated with coordinate collisions
according to a user-implemented function
```void merge(_Td& left, const _Td& right)```. cmap furthermore provides
a function ```void resize()```, which

* divides each coordinate by two, and
* merges values associated with adjusted coordinate collisions.

Examples can be found in ```tests/test{2,3,4,5}.cpp```.

Work in progess.

Copyright
---------

Copyright (c) 2020, Sebastian Wouters

All rights reserved.

cmap is licensed under the BSD 3-Clause License. A copy of the License
can be found in the file LICENSE in the root folder of this project.

