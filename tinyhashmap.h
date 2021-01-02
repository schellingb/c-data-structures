/* TinyHashMap - simple hash map - public domain - Bernhard Schelling 2020
   https://github.com/schellingb/c-data-structures
         no warranty implied; use at your own risk

   This file implements a hash map with 32-bit keys.
   Based on the implementation from the public domain Bitwise project
   by Per Vognsen - https://github.com/pervognsen/bitwise

   It's a super simple type safe hash map for C with no need
   to predeclare any type or anything.
   Will always allocate memory for twice the amount of max elements
   so larger structs should be stored as pointers or indices to an array.
   Can be used in C++ with POD types (without any constructor/destructor).

   Be careful not to supply modifying statements to the macro arguments.
   Something like HMAP_FIT(map, i++); would have unintended results.

   Sample usage:

   -- Set 2 elements with string keys and mytype_t values:
   mytype_t* map = NULL;
   HMAP_SET_STR(map, "foo", foo_element);
   HMAP_SET_STR(map, "bar", bar_element);
   -- now HMAP_LEN(map) == 2, HMAP_GET_STR(map, "foo") == foo_element

   -- Check if keys exist:
   bool has_foo = HMAP_HAS_STR(map, "foo");
   bool has_baz = HMAP_HAS_STR(map, "baz");
   -- now has_foo == true, has_baz == false

   -- Removing a key:
   bool removed = HMAP_DEL_STR(map, "bar");
   bool removed_again = HMAP_DEL_STR(map, "bar");
   -- now HMAP_LEN(map) == 1, removed == true, removed_again == false

   -- Add/modify via pointer:
   mytype_t* p_elem = HMAP_PTR_STR(map, "qux");
   p_elem->a = 123;
   -- New keys initially have memory uninitialized
   -- Pointers can get invalidated when a key is added/removed

   -- Looking up the index for a given key:
   ptrdiff_t idx_foo = HMAP_IDX_STR(map, "foo");
   ptrdiff_t idx_invalid = HMAP_IDX_STR(map, "invalid");
   -- now idx_foo >= 0, idx_invalid == -1, map[idx_foo] == foo_element
   -- Indices can change when a key is added/removed

   -- Clear all elements (keep memory allocated):
   HMAP_CLEAR(map);
   -- now HMAP_LEN(map) == 0, HMAP_CAP(map) == 16

   -- Reserve memory for at least N elements:
   HMAP_FIT(map, 30);
   -- now HMAP_LEN(map) == 0, HMAP_CAP(map) == 64

   -- Add elements with custom hash keys:
   HMAP_SET(map, my_uint32_hash(key1), some_element);
   HMAP_SET(map, my_uint32_hash(key2), other_element);
   -- now HMAP_LEN(map) == 2, _GET/_HAS/_DEL/_PTR/_IDX also exist

   -- Iterate elements (random order, order can change on insert):
   for (size_t i = 0, cap = HMAP_CAP(map); i != cap, i++)
     if (HMAP_KEY(map, i))
   ------ here map[i] is the value of key HMAP_KEY(map, i)

   -- Set a custom null value (is zeroed by default):
   HMAP_SETNULLVAL(map, map_null);
   -- now HMAP_GET_STR(map, "invalid") == map_null

   -- Free allocated memory:
   HMAP_FREE(map);
   -- now map == NULL, HMAP_LEN(map) == 0, HMAP_CAP(map) == 0

   -- To handle running out of memory:
   bool ran_out_of_memory = !HMAP_TRYFIT(map, 1000);
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

#ifndef TINYHASHMAP_H
#define TINYHASHMAP_H

#include <stdlib.h> /* for malloc, realloc */
#include <string.h> /* for memcpy, memset */
#include <stddef.h> /* for ptrdiff_t, size_t */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h> /* for uint32_t */
#endif

#define HMAP_LEN(b) ((b) ? HMAP__HDR(b)->len : 0)
#define HMAP_MAX(b) ((b) ? HMAP__HDR(b)->maxlen : 0)
#define HMAP_CAP(b) ((b) ? HMAP__HDR(b)->maxlen + 1 : 0)
#define HMAP_KEY(b, idx) (HMAP__HDR(b)->keys[idx])
#define HMAP_SETNULLVAL(b, val) (HMAP__FIT1(b), b[-1] = (val))
#define HMAP_CLEAR(b) ((b) ? (memset(HMAP__HDR(b)->keys, 0, HMAP_CAP(b) * sizeof(uint32_t)), HMAP__HDR(b)->len = 0) : 0)
#define HMAP_FREE(b) ((b) ? (free(HMAP__HDR(b)->keys), free(HMAP__HDR(b)), (b) = NULL) : 0)
#define HMAP_FIT(b, n) ((!(n) || ((b) && (size_t)(n) * 2 <= HMAP_MAX(b))) ? 0 : HMAP__GROW(b, n))
#define HMAP_TRYFIT(b, n) (HMAP_FIT((b), (n)), (!(n) || ((b) && (size_t)(n) * 2 <= HMAP_MAX(b))))

#define HMAP_SET(b, key, val) (HMAP__FIT1(b), b[hmap__idx(HMAP__HDR(b), (key), 1, 0)] = (val))
#define HMAP_GET(b, key) (HMAP__FIT1(b), b[hmap__idx(HMAP__HDR(b), (key), 0, 0)])
#define HMAP_HAS(b, key) ((b) ? hmap__idx(HMAP__HDR(b), (key), 0, 0) != -1 : 0)
#define HMAP_DEL(b, key) ((b) ? hmap__idx(HMAP__HDR(b), (key), 0, sizeof(*(b))) != -1 : 0)
#define HMAP_PTR(b, key) (HMAP__FIT1(b), &b[hmap__idx(HMAP__HDR(b), (key), 1, 0)])
#define HMAP_IDX(b, key) ((b) ? hmap__idx(HMAP__HDR(b), (key), 0, 0) : -1)

#ifndef TINYHASHMAP_WITHOUT_STRINGS
#define HMAP_SET_STR(b, string_key, val) HMAP_SET(b, hash_string(string_key), val)
#define HMAP_GET_STR(b, string_key)      HMAP_GET(b, hash_string(string_key))
#define HMAP_HAS_STR(b, string_key)      HMAP_HAS(b, hash_string(string_key))
#define HMAP_DEL_STR(b, string_key)      HMAP_DEL(b, hash_string(string_key))
#define HMAP_PTR_STR(b, string_key)      HMAP_PTR(b, hash_string(string_key))
#define HMAP_IDX_STR(b, string_key)      HMAP_IDX(b, hash_string(string_key))

static uint32_t hash_string(const char* str)
{
	unsigned char c;
	uint32_t hash = (uint32_t)0x811c9dc5;
	while ((c = (unsigned char)*(str++)) != '\0')
		hash = ((hash * (uint32_t)0x01000193) ^ (uint32_t)c);
	return (hash ? hash : 1);
}
#endif

struct hmap__hdr { size_t len, maxlen; uint32_t *keys; };
#define HMAP__HDR(b) (((struct hmap__hdr *)&(b)[-1])-1)
#define HMAP__GROW(b, n) (*(void**)(&(b)) = hmap__grow(HMAP__HDR(b), (void*)(b), sizeof(*(b)), (size_t)(n)))
#define HMAP__FIT1(b) ((b) && HMAP_LEN(b) * 2 <= HMAP_MAX(b) ? 0 : HMAP__GROW(b, 0))

static void* hmap__grow(struct hmap__hdr *old_hdr, void* old_ptr, size_t elem_size, size_t reserve)
{
	struct hmap__hdr *new_hdr;
	char *new_vals;
	size_t new_max = (old_ptr ? old_hdr->maxlen * 2 + 1 : 15);
	while (new_max && new_max / 2 <= reserve)
		if (!(new_max = new_max * 2 + 1))
			return old_ptr; /* overflow */

	new_hdr = (struct hmap__hdr *)malloc(sizeof(struct hmap__hdr) + (new_max + 2) * elem_size);
	if (!new_hdr)
		return old_ptr; /* out of memory */

	new_hdr->maxlen = new_max;
	new_hdr->keys = (uint32_t *)calloc(new_max + 1, sizeof(uint32_t));
	if (!new_hdr->keys)
		return (free(new_hdr), old_ptr); /* out of memory */

	new_vals = ((char*)(new_hdr + 1)) + elem_size;
	if (old_ptr)
	{
		size_t i;
		char* old_vals = ((char*)(old_hdr + 1)) + elem_size;
		for (i = 0; i <= old_hdr->maxlen; i++)
		{
			uint32_t key, j;
			if (!old_hdr->keys[i])
				continue;
			for (key = old_hdr->keys[i], j = key;; j++)
			{
				if (!new_hdr->keys[j &= new_hdr->maxlen])
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

static ptrdiff_t hmap__idx(struct hmap__hdr* hdr, uint32_t key, int add, int del)
{
	uint32_t i;

	if (!key)
		return (ptrdiff_t)-1;

	for (i = key;; i++)
	{
		if (hdr->keys[i &= hdr->maxlen] == key)
		{
			if (del)
			{
				hdr->len--;
				hdr->keys[i] = 0;
				while ((key = hdr->keys[i = (i + 1) & hdr->maxlen]) != 0)
				{
					if ((key = (uint32_t)hmap__idx(hdr, key, 1, 0)) == i) continue;
					hdr->len--;
					hdr->keys[i] = 0;
					memcpy(((char*)(hdr + 1)) + (key + 1) * del,
						((char*)(hdr + 1)) + (i + 1) * del, del);
				}
			}
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
