/* TinyHashMap64 - simple hash map - public domain - Bernhard Schelling 2020
   https://github.com/schellingb/c-data-structures
         no warranty implied; use at your own risk

   This file implements a hash map with 64-bit keys.
   Based on the implementation from the public domain Bitwise project
   by Per Vognsen - https://github.com/pervognsen/bitwise

   It's a super simple type safe hash map for C with no need
   to predeclare any type or anything.
   Will always allocate memory for twice the amount of max elements
   so larger structs should be stored as pointers or indices to an array.
   Can be used in C++ with POD types (without any constructor/destructor).

   Be careful not to supply modifying statements to the macro arguments.
   Something like HMAP64_FIT(map, i++); would have unintended results.

   Sample usage:

   -- Set 2 elements with string keys and mytype_t values:
   mytype_t* map = NULL;
   HMAP64_SET_STR(map, "foo", foo_element);
   HMAP64_SET_STR(map, "bar", bar_element);
   -- now HMAP64_LEN(map) == 2, HMAP64_GET_STR(map, "foo") == foo_element

   -- Check if keys exist:
   bool has_foo = HMAP64_HAS_STR(map, "foo");
   bool has_baz = HMAP64_HAS_STR(map, "baz");
   -- now has_foo == true, has_baz == false

   -- Removing a key:
   bool removed = HMAP64_DEL_STR(map, "bar");
   bool removed_again = HMAP64_DEL_STR(map, "bar");
   -- now HMAP64_LEN(map) == 1, removed == true, removed_again == false

   -- Add/modify via pointer:
   mytype_t* p_elem = HMAP64_PTR_STR(map, "qux");
   p_elem->a = 123;
   -- New keys initially have memory uninitialized
   -- Pointers can get invalidated when a new key is added

   -- Looking up the index for a given key:
   ptrdiff_t idx_foo = HMAP64_IDX_STR(map, "foo");
   ptrdiff_t idx_invalid = HMAP64_IDX_STR(map, "invalid");
   -- now idx_foo >= 0, idx_invalid == -1, map[idx_foo] == foo_element
   -- Indices can change when a new key is added

   -- Clear all elements (keep memory allocated):
   HMAP64_CLEAR(map);
   -- now HMAP64_LEN(map) == 0, HMAP64_CAP(map) == 16

   -- Reserve memory for at least N elements:
   HMAP64_FIT(map, 30);
   -- now HMAP64_LEN(map) == 0, HMAP64_CAP(map) == 64

   -- Add elements with custom hash keys:
   HMAP64_SET(map, my_uint64_hash(key1), some_element);
   HMAP64_SET(map, my_uint64_hash(key2), other_element);
   -- now HMAP64_LEN(map) == 2, _GET/_HAS/_DEL/_PTR/_IDX also exist

   -- Iterate elements (random order, order can change on insert):
   for (size_t i = 0, cap = HMAP64_CAP(map); i != cap, i++)
     if (HMAP64_KEY(map, i))
   ------ here map[i] is the value of key HMAP64_KEY(map, i)

   -- Set a custom null value (is zeroed by default):
   HMAP64_SETNULLVAL(map, map_null);
   -- now HMAP64_GET_STR(map, "invalid") == map_null

   -- Free allocated memory:
   HMAP64_FREE(map);
   -- now map == NULL, HMAP64_LEN(map) == 0, HMAP64_CAP(map) == 0

   -- To handle running out of memory:
   bool ran_out_of_memory = !HMAP64_TRYFIT(map, 1000);
   -- before setting an element (with SET, PTR or NULLVAL).
   -- When out of memory, map will stay unmodified.

   PUBLIC DOMAIN (UNLICENSE)

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
*/

#ifndef TINYHASHMAP64_H
#define TINYHASHMAP64_H

#include <stdlib.h> /* for malloc, realloc */
#include <string.h> /* for memcpy, memset */
#include <stddef.h> /* for ptrdiff_t, size_t */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h> /* for uint64_t */
#endif

#define HMAP64_LEN(b) ((b) ? HMAP64__HDR(b)->len : 0)
#define HMAP64_CAP(b) ((b) ? HMAP64__HDR(b)->cap : 0)
#define HMAP64_KEY(b, idx) (HMAP64__HDR(b)->keys[idx])
#define HMAP64_SETNULLVAL(b, val) (HMAP64__FIT1(b), b[-1] = (val))
#define HMAP64_CLEAR(b) ((b) ? (memset(HMAP64__HDR(b)->keys, 0, HMAP64_CAP(b) * sizeof(uint64_t)), HMAP64__HDR(b)->len = 0) : 0)
#define HMAP64_FREE(b) ((b) ? (free(HMAP64__HDR(b)->keys), free(HMAP64__HDR(b)), (b) = NULL) : 0)
#define HMAP64_FIT(b, n) ((!(n) || ((b) && (size_t)(n) * 2 < HMAP64_CAP(b))) ? 0 : HMAP64__GROW(b, n))
#define HMAP64_TRYFIT(b, n) (HMAP64_FIT((b), (n)), (!(n) || ((b) && (size_t)(n) * 2 < HMAP64_CAP(b))))

#define HMAP64_SET(b, key, val) (HMAP64__FIT1(b), b[hmap64__idx(HMAP64__HDR(b), (key), 1, 0)] = (val))
#define HMAP64_GET(b, key) (HMAP64__FIT1(b), b[hmap64__idx(HMAP64__HDR(b), (key), 0, 0)])
#define HMAP64_HAS(b, key) ((b) ? hmap64__idx(HMAP64__HDR(b), (key), 0, 0) != -1 : 0)
#define HMAP64_DEL(b, key) ((b) ? hmap64__idx(HMAP64__HDR(b), (key), 0, 1) != -1 : 0)
#define HMAP64_PTR(b, key) (HMAP64__FIT1(b), &b[hmap64__idx(HMAP64__HDR(b), (key), 1, 0)])
#define HMAP64_IDX(b, key) ((b) ? hmap64__idx(HMAP64__HDR(b), (key), 0, 0) : -1)

#ifndef TINYHASHMAP64_WITHOUT_STRINGS
#define HMAP64_SET_STR(b, string_key, val) HMAP64_SET(b, hash64_string(string_key), val)
#define HMAP64_GET_STR(b, string_key)      HMAP64_GET(b, hash64_string(string_key))
#define HMAP64_HAS_STR(b, string_key)      HMAP64_HAS(b, hash64_string(string_key))
#define HMAP64_DEL_STR(b, string_key)      HMAP64_DEL(b, hash64_string(string_key))
#define HMAP64_PTR_STR(b, string_key)      HMAP64_PTR(b, hash64_string(string_key))
#define HMAP64_IDX_STR(b, string_key)      HMAP64_IDX(b, hash64_string(string_key))

static uint64_t hash64_string(const char* str)
{
	unsigned char c;
	uint64_t hash = (uint64_t)0xcbf29ce484222325;
	while ((c = (unsigned char)*(str++)) != '\0')
		hash = ((hash * (uint64_t)0x100000001b3) ^ (uint64_t)c);
	return (hash ? hash : 1);
}
#endif

struct hmap64__hdr { size_t len, cap; uint64_t *keys; };
#define HMAP64__HDR(b) (((struct hmap64__hdr *)&(b)[-1])-1)
#define HMAP64__GROW(b, n) (*(void**)(&(b)) = hmap64__grow(HMAP64__HDR(b), (void*)(b), sizeof(*(b)), (size_t)(n)))
#define HMAP64__FIT1(b) ((b) && HMAP64_LEN(b) * 2 < HMAP64_CAP(b) ? 0 : HMAP64__GROW(b, 0))

static void* hmap64__grow(struct hmap64__hdr *old_hdr, void* old_ptr, size_t elem_size, size_t res)
{
	struct hmap64__hdr *new_hdr;
	char *new_vals;
	size_t new_cap = (old_ptr ? old_hdr->cap * 2 : 16);
	while (new_cap && new_cap / 2 < res)
		if (!(new_cap *= 2))
			return old_ptr; /* overflow */

	new_hdr = (struct hmap64__hdr *)malloc(sizeof(struct hmap64__hdr) + (new_cap + 1) * elem_size);
	if (!new_hdr)
		return old_ptr; /* out of memory */

	new_hdr->cap = new_cap;
	new_hdr->keys = (uint64_t *)calloc(new_cap, sizeof(uint64_t));
	if (!new_hdr->keys)
		return (free(new_hdr), old_ptr); /* out of memory */

	new_vals = ((char*)(new_hdr + 1)) + elem_size;
	if (old_ptr)
	{
		size_t i;
		char* old_vals = ((char*)(old_hdr + 1)) + elem_size;
		for (i = 0; i < old_hdr->cap; i++)
		{
			uint64_t key, j;
			if (!old_hdr->keys[i])
				continue;
			for (key = old_hdr->keys[i], j = key;; j++)
			{
				if (!new_hdr->keys[j &= new_hdr->cap - 1])
				{
					new_hdr->keys[j] = key;
					memcpy(new_vals + j * elem_size, old_vals + i * elem_size, elem_size);
					break;
				}
			}
		}
		memcpy(new_vals - elem_size, old_vals - elem_size, elem_size);
		new_hdr->len = old_hdr->len;
		free(old_hdr->keys);
		free(old_hdr);
	}
	else
	{
		memset(new_vals - elem_size, 0, elem_size);
		new_hdr->len = 0;
	}
	return new_vals;
}

static ptrdiff_t hmap64__idx(struct hmap64__hdr* hdr, uint64_t key, int add, int del)
{
	uint64_t i;

	if (!key)
		return (ptrdiff_t)-1;

	for (i = key;; i++)
	{
		if (hdr->keys[i &= hdr->cap - 1] == key)
		{
			if (del) { hdr->len --; hdr->keys[i] = 0; }
			return (ptrdiff_t)i;
		}
		if (!hdr->keys[i])
		{
			if (add) { hdr->len++; hdr->keys[i] = key; return (ptrdiff_t)i; }
			return (ptrdiff_t)-1;
		}
	}
}

#endif
