# C Data Structures

Simple and convenient data structure single-file public domain libraries for C/C++

* [TinyHashMap](#tinyhashmap---simple-hash-map) - Simple Hash Map (in 111 lines of code)
* [TinyBuf](#tinybuf---simple-dynamic-array) - Simple Dynamic Array (in 50 lines of code)


## TinyHashMap - Simple Hash Map

Implements a hash map with 32-bit or 64-bit (see [TinyHashMap64](#64-bit-keys)) keys.  
Based on the implementation from the public domain Bitwise project by Per Vognsen - https://github.com/pervognsen/bitwise

It's a super simple type safe hash map for C with no need to predeclare any type or anything.  
Will always allocate memory for twice the amount of max elements so larger structs should be stored as pointers or indices to an array.  
Can be used in C++ with POD types (without any constructor/destructor).

### Usage
```c
#include "tinyhashmap.h"
```
See [tinyhashmap.h](tinyhashmap.h)

#### Set 2 elements with string keys and mytype_t values
```c
mytype_t* map = NULL;
HMAP_SET_STR(map, "foo", foo_element);
HMAP_SET_STR(map, "bar", bar_element);
```
Now `HMAP_LEN(map) == 2`, `HMAP_GET_STR(map, "foo") == foo_element`, `HMAP_GET_STR(map, "bar") == bar_element`

#### Check if keys exist
```c
bool has_foo = HMAP_HAS_STR(map, "foo");
bool has_baz = HMAP_HAS_STR(map, "baz");
```
Now `has_foo == true`, `has_baz == false`

#### Removing a key
```c
bool removed = HMAP_DEL_STR(map, "bar");
bool removed_again = HMAP_DEL_STR(map, "bar");
```
Now `HMAP_LEN(map) == 1`, `removed == true`, `removed_again == false`

#### Add/modify via pointer
```c
mytype_t* p_elem = HMAP_PTR_STR(map, "qux");
p_elem->a = 123;
```
New keys initially have memory uninitialized  
Pointers can get invalidated when a key is added/removed

#### Looking up the index for a given key
```c
ptrdiff_t idx_foo = HMAP_IDX_STR(map, "foo");
ptrdiff_t idx_invalid = HMAP_IDX_STR(map, "invalid");
```
Now `idx_foo >= 0`, `idx_invalid == -1`, `map[idx_foo] == foo_element`  
Indices can change when a key is added/removed

#### Clear all elements (keep memory allocated)
```c
HMAP_CLEAR(map);
```
Now `HMAP_LEN(map) == 0`, `HMAP_CAP(map) == 16`

#### Reserve memory for at least N elements
```c
HMAP_FIT(map, 30);
```
Now `HMAP_LEN(map) == 0`, `HMAP_CAP(map) == 64`

#### Add elements with custom hash keys
```c
HMAP_SET(map, my_uint32_hash(key1), some_element);
HMAP_SET(map, my_uint32_hash(key2), other_element);
```
Now `HMAP_LEN(map) == 2`

#### Iterate elements (random order, order can change on insert)
```c
for (size_t i = 0, cap = HMAP_CAP(map); i != cap, i++)
    if (HMAP_KEY(map, i))
```
Inside that if, `map[i]` is the value of key `HMAP_KEY(map, i)`

#### Set a custom null value (is zeroed by default)
```c
HMAP_SETNULLVAL(map, map_null);
```
Now `HMAP_GET_STR(map, "invalid") == map_null`

#### Free allocated memory
```c
HMAP_FREE(map);
```
Now `map == NULL`, `HMAP_LEN(map) == 0`, `HMAP_CAP(map) == 0`

#### To handle running out of memory
```c
bool ran_out_of_memory = !HMAP_TRYFIT(map, 1000);
```
You can check memory availability with this before setting an element (with `SET`, `PTR` or `NULLVAL`).  
When out of memory, `map` will stay unmodified.

### Filtered string keys
A neat trick with string based keys is to filter the strings while hashing.  
For example, this hash function ignores white space and case:
```c
static uint32_t hash_nocase_nospace(const char* str)
{
	unsigned char c;
	uint32_t hash = (uint32_t)0x811c9dc5;
	while ((c = (unsigned char)*(str++)) != '\0')
		if (c > ' ')
			hash = ((hash * (uint32_t)0x01000193) ^ (uint32_t)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c));
	return (hash ? hash : 1);
}
```
Then `HMAP_GET(map, hash_nocase_nospace("TEST A"))` and `HMAP_PTR(map, hash_nocase_nospace("testa"))` return the same.

### Notes
Be careful not to supply modifying statements to the macro arguments.  
Something like `HMAP_FIT(map, i++);` would have unintended results.

### 64-bit Keys
Include `tinyhashmap64.h` instead and use the prefix `HMAP64_` instead of `HMAP_`. See [tinyhashmap64.h](tinyhashmap64.h)


## TinyBuf - Simple Dynamic Array

Implements stretchy buffers as invented (?) by Sean Barrett.  
Based on the implementation from the public domain Bitwise project by Per Vognsen - https://github.com/pervognsen/bitwise

It's a super simple type safe dynamic array for C with no need to predeclare any type or anything.  
The first time an element is added, memory for 16 elements are allocated. Then every time length is about to exceed capacity, capacity is doubled.  
Can be used in C++ with POD types (without any constructor/destructor).

### Usage
```c
#include "tinybuf.h"
```
See [tinybuf.h](tinybuf.h)

#### Setup and adding 2 elements
```c
mytype_t* buf = NULL;
BUF_PUSH(buf, some_element);
BUF_PUSH(buf, other_element);
```
Now `BUF_LEN(buf) == 2`, `buf[0] == some_element`, `buf[1] == other_element`

#### Free allocated memory
```c
BUF_FREE(buf);
```
Now `buf == NULL`, `BUF_LEN(buf) == 0`, `BUF_CAP(buf) == 0`

#### Explicitly increase allocated memory and set capacity
```c
BUF_FIT(buf, 100);
```
Now `BUF_LEN(buf) == 0`, `BUF_CAP(buf) == 100`

#### Resize buffer (does not initialize or zero memory!)
```c
BUF_RESIZE(buf, 200);
```
Now `BUF_LEN(buf) == 200`, `BUF_CAP(buf) == 200`

#### Remove an element in the middle, keeping order
```c
BUF_REMOVE(buf, 30);
```
Now `BUF_LEN(buf) == 199`, everything past 30 shifted 1 up

#### Remove an element in the middle, swapping the last element into it
```c
BUF_SWAPREMOVE(buf, 10);
```
Now `BUF_LEN(buf) == 198`, element 198 was copied into 10

#### Insert an element into the middle of the array
```c
BUF_INSERT(buf, 100, some_element);
```
Now `BUF_LEN(buf) == 199`, everything past 99 shifted 1 down

#### Make a gap of a given size in the middle of the array
```c
mytype_t* ptr_gap = BUF_MAKEGAP(buf, 20, 11);
```
Now `BUF_LEN(buf) == 210`, everything past 19 shifted 11 down

#### Add multiple elements at the end (uninitialized)
```c
mytype_t* ptr1 = BUF_ADD(buf, 10);
```
Now `BUF_LEN(buf) == 220`, `ptr1` points to `buf[210]`

#### Add multiple elements at the end (zeroing memory)
```c
mytype_t* ptr2 = BUF_ADDZEROED(buf, 10);
```
Now `BUF_LEN(buf) == 230`, `ptr2` points to `buf[220]`

#### To handle running out of memory
```c
bool ran_out_of_memory = !BUF_TRYFIT(buf, 1000);
```
before `RESIZE or `PUSH.  
When out of memory, buf will stay unmodified.

### Notes
Be careful not to supply modifying statements to the macro arguments.  
Something like `BUF_REMOVE(buf, i--);` would have unintended results.


# Public Domain (Unlicense)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
