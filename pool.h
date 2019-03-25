#ifndef POOL_H
#define POOL_H

#include <cstddef>

const size_t end_of_str = 1;
const size_t additional_bytes = 2;
const size_t length_bytes = 1;


struct string_ref{
	char* data = nullptr;
	bool valid = false;
};

struct string_pool{
	char* str = nullptr;
	string_ref* str_ref = nullptr;
	size_t size = 0;
	size_t used = 0;
	size_t max_ref = 0;
	size_t used_refs = 0;
};

string_pool* make_string_pool(size_t size, size_t max_strings_count = 0);
void release_pool(string_pool* pool);
string_ref* put_string_in_pool(string_pool* pool, const char* string);
string_ref* reserve_string_space_in_pool(string_pool* pool, size_t max_length);
void copy_to_string_ref(string_ref* ref, const char* string, size_t len);
void release_string_in_pool(string_pool* pool, string_ref* ref);
size_t get_data_size(string_pool* pool);
void dump_pool(string_pool* pool);
void compact_pool(string_pool* pool);

#endif // POOL_H