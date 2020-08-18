/* C Data Structures Test - public domain - Bernhard Schelling 2020
   https://github.com/schellingb/c-data-structures
         no warranty implied; use at your own risk

   This is some test code for this project.
   Simple and convenient data structure single-file public domain libraries for C/C++

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

#include "tinybuf.h"
#include "tinyhashmap.h"
#include "tinyhashmap64.h"

#include <stdio.h>
#define CDS_ASSERT(cond) (void)((cond) ? ((int)0) : (*(volatile int*)0 = 0xbad|fprintf(stderr, "FAILED ASSERT (%s)\n", #cond )))

typedef struct { int a, b, c; } mytype_t;

static void test_buf()
{
	mytype_t *buf = NULL, *ptr;
	mytype_t some_element = { 1, 2, 3 };
	mytype_t other_element = { 500, 10, 99 };
	int i, ran_out_of_memory;

	/* Add elements: */
	BUF_PUSH(buf, some_element);
	BUF_PUSH(buf, other_element);
	CDS_ASSERT(BUF_LEN(buf) == 2);
	CDS_ASSERT(!memcmp(&buf[0], &some_element, sizeof(some_element)));
	CDS_ASSERT(!memcmp(&buf[1], &other_element, sizeof(other_element)));

	/* Free allocated memory: */
	BUF_FREE(buf);
	CDS_ASSERT(buf == NULL);
	CDS_ASSERT(BUF_LEN(buf) == 0);
	CDS_ASSERT(BUF_CAP(buf) == 0);

	/* Explicitly increase allocated memory and set capacity: */
	BUF_FIT(buf, 100);
	CDS_ASSERT(BUF_LEN(buf) == 0);
	CDS_ASSERT(BUF_CAP(buf) == 100);

	/* Resize buffer (does not initialize or zero memory!): */
	BUF_RESIZE(buf, 200);
	CDS_ASSERT(BUF_LEN(buf) == 200);
	CDS_ASSERT(BUF_CAP(buf) == 200);

	/* Remove an element in the middle, keeping order: */
	for (i = 0; i != (int)BUF_LEN(buf); i++) buf[i].a = i;
	BUF_REMOVE(buf, 30);
	CDS_ASSERT(BUF_LEN(buf) == 199);
	CDS_ASSERT(buf[29].a == 29);
	CDS_ASSERT(buf[30].a == 31);

	/* Remove an element in the middle, swapping the last element into it: */
	for (i = 0; i != (int)BUF_LEN(buf); i++) buf[i].a = i;
	BUF_SWAPREMOVE(buf, 10);
	CDS_ASSERT(BUF_LEN(buf) == 198);
	CDS_ASSERT(buf[0].a == 0 && buf[9].a == 9 && buf[10].a == 198 && buf[11].a == 11 && buf[197].a == 197);

	/* Insert an element into the middle of the array: */
	for (i = 0; i != (int)BUF_LEN(buf); i++) buf[i].a = i;
	BUF_INSERT(buf, 100, some_element);
	CDS_ASSERT(BUF_LEN(buf) == 199);
	CDS_ASSERT(!memcmp(&buf[100], &some_element, sizeof(some_element)));
	CDS_ASSERT(buf[0].a == 0 && buf[99].a == 99 && buf[101].a == 100 && buf[198].a == 197);

	/* Make a gap of a given size in the middle of the array: */
	for (i = 0; i != (int)BUF_LEN(buf); i++) buf[i].a = i;
	ptr = BUF_MAKEGAP(buf, 20, 11);
	CDS_ASSERT(BUF_LEN(buf) == 210 && ptr == &buf[20]);
	CDS_ASSERT(buf[0].a == 0 && buf[19].a == 19 && buf[31].a == 20 && buf[199].a == 188 && buf[209].a == 198);

	/* Add multiple elements at the end (uninitialized): */
	BUF_ADD(buf, 10)[5] = some_element;
	CDS_ASSERT(BUF_LEN(buf) == 220);
	CDS_ASSERT(!memcmp(&buf[215], &some_element, sizeof(some_element)));

	/* Add multiple elements at the end (zero memory): */
	ptr = BUF_ADDZEROED(buf, 10);
	CDS_ASSERT(BUF_LEN(buf) == 230 && ptr == &buf[220]);
	CDS_ASSERT(*(char*)&buf[220] == 0 && !memcmp(&buf[220], (char*)&buf[220] + 1, sizeof(*buf)*10-1));

	/* Handle running out of memory: */
	ran_out_of_memory = !BUF_TRYFIT(buf, 1000);
	CDS_ASSERT(!ran_out_of_memory);
	CDS_ASSERT(BUF_LEN(buf) == 230);
	CDS_ASSERT(BUF_CAP(buf) == 1000);
	ran_out_of_memory = !BUF_TRYFIT(buf, (sizeof(void*) > 4 ? 0xFFFF000000000000 : 0xFFFF0000) / sizeof(mytype_t));
	CDS_ASSERT(ran_out_of_memory);
	CDS_ASSERT(BUF_LEN(buf) == 230);
	CDS_ASSERT(BUF_CAP(buf) == 1000);

	BUF_FREE(buf);
}

static uint32_t hash_nocase_nospace(const char* str)
{
	unsigned char c;
	uint32_t hash = (uint32_t)0x811c9dc5;
	while ((c = (unsigned char)*(str++)) != '\0')
		if (c > ' ')
			hash = ((hash * (uint32_t)0x01000193) ^ (uint32_t)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c));
	return (hash ? hash : 1);
}

static void test_hmap()
{
	mytype_t* map = NULL;
	mytype_t some_element = { 1, 2, 3 };
	mytype_t other_element = { 500, 10, 99 };
	mytype_t map_null = { -1, -1, -1 };
	mytype_t* p_elem;
	int has_foo, has_baz, removed, removed_again, it_found_count, ran_out_of_memory;
	size_t i, cap;
	ptrdiff_t idx_foo, idx_invalid;

	/* Set 2 elements with string keys and mytype_t values: */
	HMAP_SET_STR(map, "foo", some_element);
	HMAP_SET_STR(map, "bar", other_element);
	CDS_ASSERT(HMAP_LEN(map) == 2);
	CDS_ASSERT(!memcmp(HMAP_PTR_STR(map, "foo"), &some_element, sizeof(some_element)));
	CDS_ASSERT(!memcmp(HMAP_PTR_STR(map, "bar"), &other_element, sizeof(other_element)));

	/* Check if keys exist: */
	has_foo = HMAP_HAS_STR(map, "foo");
	has_baz = HMAP_HAS_STR(map, "baz");
	CDS_ASSERT(has_foo);
	CDS_ASSERT(!has_baz);

	/* Removing a key: */
	removed = HMAP_DEL_STR(map, "bar");
	removed_again = HMAP_DEL_STR(map, "bar");
	CDS_ASSERT(HMAP_LEN(map) == 1);
	CDS_ASSERT(removed);
	CDS_ASSERT(!removed_again);

	/* Add/modify via pointer: */
	p_elem = HMAP_PTR_STR(map, "qux");
	p_elem->a = 123;
	CDS_ASSERT(HMAP_GET_STR(map, "qux").a == 123);

	/* Looking up an index of a key: */
	idx_foo = HMAP_IDX_STR(map, "foo");
	idx_invalid = HMAP_IDX_STR(map, "invalid");
	CDS_ASSERT(idx_foo >= 0 && idx_invalid == -1);

	/* Clear all elements (keep memory allocated): */
	HMAP_CLEAR(map);
	CDS_ASSERT(HMAP_LEN(map) == 0);
	CDS_ASSERT(HMAP_CAP(map) == 16);

	/* Reserve memory for at least N elements: */
	HMAP_FIT(map, 30);
	CDS_ASSERT(HMAP_LEN(map) == 0);
	CDS_ASSERT(HMAP_CAP(map) == 64);

	/* Add elements with custom hash keys: */
	HMAP_SET(map, /*my_uint32_hash(key1)*/ 0x11111111, some_element);
	HMAP_SET(map, /*my_uint32_hash(key2)*/ 0x22222222, other_element);
	CDS_ASSERT(HMAP_LEN(map) == 2);

	/* Iterate elements (random order, order can change on insert): */
	it_found_count = 0;
	for (i = 0, cap = HMAP_CAP(map); i != cap; i++)
	{
		uint32_t key;
		mytype_t val;
		if (!HMAP_KEY(map, i)) continue;
		key = HMAP_KEY(map, i);
		val = map[i];
		CDS_ASSERT(key == 0x11111111 || key == 0x22222222);
		CDS_ASSERT(key != 0x11111111 || !memcmp(&val, &some_element, sizeof(some_element)));
		CDS_ASSERT(key != 0x22222222 || !memcmp(&val, &other_element, sizeof(other_element)));
		it_found_count++;
	}
	CDS_ASSERT(it_found_count == 2);

	/* Set a custom null value (is zeroed by default): */
	HMAP_SETNULLVAL(map, map_null);
	CDS_ASSERT(!memcmp(&map[HMAP_IDX_STR(map, "invalid")], &map_null, sizeof(map_null)));

	/* Free allocated memory: */
	HMAP_FREE(map);
	CDS_ASSERT(map == NULL);
	CDS_ASSERT(HMAP_LEN(map) == 0);
	CDS_ASSERT(HMAP_CAP(map) == 0);

	/* Handle running out of memory: */
	ran_out_of_memory = !HMAP_TRYFIT(map, 1000);
	CDS_ASSERT(!ran_out_of_memory);
	CDS_ASSERT(HMAP_LEN(map) == 0);
	CDS_ASSERT(HMAP_CAP(map) == 2048);
	ran_out_of_memory = !HMAP_TRYFIT(map, (sizeof(void*) > 4 ? 0x7FFF000000000000 : 0x7FFF0000) / sizeof(mytype_t));
	CDS_ASSERT(ran_out_of_memory);
	CDS_ASSERT(HMAP_LEN(map) == 0);
	CDS_ASSERT(HMAP_CAP(map) == 2048);

	/* Filtered string keys: */
	HMAP_SET(map, hash_nocase_nospace("TEST A"), some_element);
	CDS_ASSERT(HMAP_IDX(map, hash_nocase_nospace("TEST A")) == HMAP_IDX(map, hash_nocase_nospace("testa")));
	CDS_ASSERT(HMAP_IDX(map, hash_nocase_nospace("TEST A")) != HMAP_IDX(map, hash_nocase_nospace("TEST B")));

	HMAP_FREE(map);
}

static void test_hmap64()
{
	mytype_t* map = NULL;
	mytype_t some_element = { 1, 2, 3 };
	mytype_t other_element = { 500, 10, 99 };
	mytype_t map_null = { -1, -1, -1 };
	mytype_t* p_elem;
	int has_foo, has_baz, removed, removed_again, it_found_count, ran_out_of_memory;
	size_t i, cap;
	ptrdiff_t idx_foo, idx_invalid;

	/* Set 2 elements with string keys and mytype_t values: */
	HMAP64_SET_STR(map, "foo", some_element);
	HMAP64_SET_STR(map, "bar", other_element);
	CDS_ASSERT(HMAP64_LEN(map) == 2);
	CDS_ASSERT(!memcmp(HMAP64_PTR_STR(map, "foo"), &some_element, sizeof(some_element)));
	CDS_ASSERT(!memcmp(HMAP64_PTR_STR(map, "bar"), &other_element, sizeof(other_element)));

	/* Check if keys exist: */
	has_foo = HMAP64_HAS_STR(map, "foo");
	has_baz = HMAP64_HAS_STR(map, "baz");
	CDS_ASSERT(has_foo);
	CDS_ASSERT(!has_baz);

	/* Removing a key: */
	removed = HMAP64_DEL_STR(map, "bar");
	removed_again = HMAP64_DEL_STR(map, "bar");
	CDS_ASSERT(HMAP64_LEN(map) == 1);
	CDS_ASSERT(removed);
	CDS_ASSERT(!removed_again);

	/* Add/modify via pointer: */
	p_elem = HMAP64_PTR_STR(map, "qux");
	p_elem->a = 123;
	CDS_ASSERT(HMAP64_GET_STR(map, "qux").a == 123);

	/* Looking up an index of a key: */
	idx_foo = HMAP64_IDX_STR(map, "foo");
	idx_invalid = HMAP64_IDX_STR(map, "invalid");
	CDS_ASSERT(idx_foo >= 0 && idx_invalid == -1);

	/* Clear all elements (keep memory allocated): */
	HMAP64_CLEAR(map);
	CDS_ASSERT(HMAP64_LEN(map) == 0);
	CDS_ASSERT(HMAP64_CAP(map) == 16);

	/* Reserve memory for at least N elements: */
	HMAP64_FIT(map, 30);
	CDS_ASSERT(HMAP64_LEN(map) == 0);
	CDS_ASSERT(HMAP64_CAP(map) == 64);

	/* Add elements with custom hash keys: */
	HMAP64_SET(map, /*my_uint64_hash(key1)*/ 0x1111111111111111, some_element);
	HMAP64_SET(map, /*my_uint64_hash(key2)*/ 0x2222222222222222, other_element);
	CDS_ASSERT(HMAP64_LEN(map) == 2);

	/* Iterate elements (random order, order can change on insert): */
	it_found_count = 0;
	for (i = 0, cap = HMAP64_CAP(map); i != cap; i++)
	{
		uint64_t key;
		mytype_t val;
		if (!HMAP64_KEY(map, i)) continue;
		key = HMAP64_KEY(map, i);
		val = map[i];
		CDS_ASSERT(key == 0x1111111111111111 || key == 0x2222222222222222);
		CDS_ASSERT(key != 0x1111111111111111 || !memcmp(&val, &some_element, sizeof(some_element)));
		CDS_ASSERT(key != 0x2222222222222222 || !memcmp(&val, &other_element, sizeof(other_element)));
		it_found_count++;
	}
	CDS_ASSERT(it_found_count == 2);

	/* Set a custom null value (is zeroed by default): */
	HMAP64_SETNULLVAL(map, map_null);
	CDS_ASSERT(!memcmp(&map[HMAP64_IDX_STR(map, "invalid")], &map_null, sizeof(map_null)));

	/* Free allocated memory: */
	HMAP64_FREE(map);
	CDS_ASSERT(map == NULL);
	CDS_ASSERT(HMAP64_LEN(map) == 0);
	CDS_ASSERT(HMAP64_CAP(map) == 0);

	/* Handle running out of memory: */
	ran_out_of_memory = !HMAP64_TRYFIT(map, 1000);
	CDS_ASSERT(!ran_out_of_memory);
	CDS_ASSERT(HMAP64_LEN(map) == 0);
	CDS_ASSERT(HMAP64_CAP(map) == 2048);
	ran_out_of_memory = !HMAP64_TRYFIT(map, (sizeof(void*) > 4 ? 0x7FFF000000000000 : 0x7FFF0000) / sizeof(mytype_t));
	CDS_ASSERT(ran_out_of_memory);
	CDS_ASSERT(HMAP64_LEN(map) == 0);
	CDS_ASSERT(HMAP64_CAP(map) == 2048);

	HMAP64_FREE(map);
}

int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
	printf("Testing buf...\n");
	test_buf();
	printf("Testing hmap...\n");
	test_hmap();
	printf("Testing hmap64...\n");
	test_hmap64();
	printf("Done!\n");
	return 0;
}
