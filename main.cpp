#include <stdio.h>

#include "pool.h"

int main(int argc, char const *argv[])
{
	string_pool* pool = make_string_pool(1024);
	printf("=== Init used: %lu ===\n", pool->used);
	string_ref* p1 = put_string_in_pool(pool, "first string");
	string_ref* p2 = put_string_in_pool(pool, "second string");
	string_ref* p3 = put_string_in_pool(pool, "third string");
	string_ref* p4 = put_string_in_pool(pool, "fourth string");
	string_ref* p5 = put_string_in_pool(pool, "fifth string");
	printf("=== After putting some stuff: %lu/%lu ===\n", get_data_size(pool), pool->used);
	dump_pool(pool);
	release_string_in_pool(pool, p2);
	release_string_in_pool(pool, p4);
	printf("=== After releasing couple strings: %lu/%lu ===\n", get_data_size(pool), pool->used);
	dump_pool(pool);
	compact_pool(pool);
	printf("=== After compacting pool: %lu/%lu ===\n", get_data_size(pool), pool->used);
	dump_pool(pool);

	string_ref* p6 = put_string_in_pool(pool, "sixth string");
	string_ref* p7 = put_string_in_pool(pool, "seventh string");
	string_ref* p8 = put_string_in_pool(pool, "eight string");
	printf("=== After pushing more stuff ===\n");
	dump_pool(pool);
	release_string_in_pool(pool, p5);
	release_string_in_pool(pool, p7);
	printf("=== After releasing: %lu/%lu ===\n", get_data_size(pool), pool->used);
	dump_pool(pool);
	compact_pool(pool);
	printf("=== After compacting: %lu/%lu ===\n", get_data_size(pool), pool->used);
	dump_pool(pool);
	release_pool(pool);
	return 0;
}