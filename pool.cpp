#include "pool.h"

#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <cstdio>

#ifndef DEBUG_DUMPS
#define DEBUG_DUMPS 0
#endif

enum string_status_flag{
	USED,
	FREED
};

void clean_mem(char* ptr, size_t length, char pattern = 0x00)
{
	char* end = ptr + length;
	while(ptr != end)
		*ptr++ = pattern;
}

string_pool* make_string_pool(size_t size, size_t max_strings_count)
{
	string_pool* pool = (string_pool*)malloc(sizeof(string_pool));
	if(pool)
	{
		if(max_strings_count == 0)
			max_strings_count = (size >> 4);
		
		size_t alloc_bytes = sizeof(string_ref) * max_strings_count;
		pool->str = (char*)malloc(alloc_bytes + size);

		if(pool->str)
		{
			pool->str_ref = (string_ref*)(pool->str + size);
			pool->max_ref = max_strings_count;
			pool->size = size;
			clean_mem((char*)pool->str_ref, alloc_bytes);
		}
		else
		{
			free(pool);
			pool = nullptr;
		}
	}
	return pool;
}

void release_pool(string_pool* pool)
{
	if(pool)
	{
		if(pool->str)
			free(pool->str);
		free(pool);
	}
}

char* allocate_string_space(string_pool* pool, size_t string_characters)
{
	char* ptr = nullptr;
	if(pool != nullptr && pool->used + string_characters < pool->size)
	{
		ptr = pool->str + pool->used;
		*(ptr + length_bytes) = (0xFF & USED);
		pool->used += (string_characters + end_of_str + additional_bytes);
	}
	return ptr;
}

void free_string_space(char* ptr)
{
	if(ptr != nullptr)
		*(ptr + length_bytes) = (0xFF & FREED);
}

string_ref* first_free_ref(string_pool* pool)
{
	if(pool == nullptr || pool->str_ref == nullptr)
		return nullptr;
	string_ref* ref = pool->str_ref;
	while(ref < pool->str_ref + pool->max_ref)
	{
		if(ref->valid == false)
		{
			size_t first_free = ref - pool->str_ref + 1;
			pool->used_refs = pool->used_refs < first_free ? first_free : pool->used_refs;
#if DEBUG_DUMPS
			printf("First free at %lu\n", first_free);
#endif
			return ref;
		}
		++ref;
	}
	return nullptr;
}

string_ref* reserve_string_space_in_pool(string_pool* pool, size_t max_length)
{
	if(pool == nullptr)
		return nullptr;
	if(max_length > 255)
		return nullptr;
	char* ptr = allocate_string_space(pool, max_length);
	if(ptr == nullptr)
		return nullptr;
	string_ref* ref = first_free_ref(pool);
	if(ref == nullptr)
	{
		free_string_space(ptr);
		return nullptr;
	}
	*ptr = 0x00;
	*(ptr+length_bytes) = (USED & 0xFF);
	ptr += additional_bytes;

	ref->data = ptr;
	ref->valid = true;
	return ref;
}

void copy_to_string_ref(string_ref* ref, const char* string, size_t len)
{
	if(ref == nullptr || string == nullptr)
		return;
	strncpy(ref->data, string, len + end_of_str);
	*(ref->data - additional_bytes) = (len & 0xFF);
}

string_ref* put_string_in_pool(string_pool* pool, const char* string)
{
	if(string == nullptr || pool == nullptr)
		return nullptr;
	size_t len = strlen(string);
	if(len > 255)
		return nullptr;
	
	string_ref* ref = reserve_string_space_in_pool(pool, len);
	if(ref == nullptr)
		return nullptr;
	copy_to_string_ref(ref, string, len);
	
	return ref;
}

void release_string_in_pool(string_pool* pool, string_ref* ref)
{
	if(pool == nullptr || pool->str == nullptr || ref == nullptr || ref->data == nullptr)
		return;
	if(ref->data < pool->str + additional_bytes || ref->data >= pool->str + pool->used - end_of_str)
		return;
#if DEBUG_DUMPS
	printf("Releasing %p, %s\n", ref->data, ref->data);
#endif
	*(ref->data - additional_bytes + length_bytes) = (0xFF & FREED);
	ref->data = nullptr;
	ref->valid = false;
}

void dump_pool(string_pool* pool)
{
	string_ref* ref = pool->str_ref;
	while(ref != pool->str_ref + pool->used_refs)
	{
		if(ref->valid)
			printf("%s\n", ref->data);
		++ref;
	}
}

void update_ref_in_pool(string_pool* pool, char* start_pool, char* end_pool, int offset)
{
	if(pool == nullptr || pool->str_ref == nullptr)
		return;
	string_ref* ref = pool->str_ref;
	while(ref < pool->str_ref + pool->max_ref)
	{
		if(ref->data >= start_pool && ref->data < end_pool)
			ref->data += offset;
		ref++;
	}
}

size_t string_ref_size(string_ref* ref)
{
#if DEBUG_DUMPS
	printf("valid %d\n", ref->valid);
#endif
	if(ref == nullptr || ref->valid == false)
		return 0;
	return (size_t)(*(ref->data - additional_bytes) & 0xFF);
}

size_t get_data_size(string_pool* pool)
{
#if DEBUG_DUMPS
	printf("Refs used %lu\n", pool->used_refs);
#endif
	size_t total_size = 0;
	if(pool == nullptr || pool->str_ref == nullptr)
		return total_size;
	string_ref* ref = pool->str_ref;
	string_ref* ref_end = pool->str_ref + pool->used_refs;
	while(ref != ref_end)
	{
		total_size += string_ref_size(ref);
		++ref;
	}
	return total_size;
}

void compact_pool(string_pool* pool)
{
	if(pool == nullptr || pool->str == nullptr)
		return;
	char* free_space_begin = nullptr;
	char* occupied_space_begin = nullptr;
	char* ptr = pool->str;
	
	while(ptr < pool->str + pool->used + additional_bytes)
	{
		// for last slice without any used space after it, there will be no valid flags, etc
		bool last_slice = ptr >= pool->str + pool->used;
		// first check, then deref - UB
		if(last_slice == true || *(ptr+length_bytes) == (0xFF & FREED))
		{
			// current block is begin of a first free slice)
			if(last_slice == false && free_space_begin == nullptr) 
			{
				free_space_begin = ptr;
			}
			// occupied slice ends here and we can move it somwhere
			else if(occupied_space_begin != nullptr && free_space_begin != nullptr) 
			{
				size_t slice_size = ptr - occupied_space_begin;
				// move occupied memory
				memmove(free_space_begin, occupied_space_begin, slice_size);

				update_ref_in_pool(pool, occupied_space_begin, occupied_space_begin + slice_size, 
					free_space_begin - occupied_space_begin);

				// clear occupied ptr
				occupied_space_begin = nullptr;
				// mark begin of a new free slice that begins right after occupied slice
				free_space_begin += slice_size;
			}
		}
		// space is begin of occupied slice and there is some free space before it
		else if(free_space_begin != nullptr && occupied_space_begin == nullptr)
		{
			occupied_space_begin = ptr;
		}
		ptr += ((size_t)*ptr + end_of_str + additional_bytes);
	}
	pool->used = free_space_begin - pool->str;
}
