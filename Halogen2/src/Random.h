#pragma once

void init_genrand64(unsigned long long seed);
void init_by_array64(unsigned long long init_key[], unsigned long long key_length);
unsigned long long genrand64_int64(void);