default: wctest o3gen wcmulti lc_f lc_test
lc_test: lc_test.c lc_final.h
	gcc -O2 lc_test.c -o lc_test
lc_f: lc_final.c lc_final.h
	gcc -O2 lc_final.c -o lc_f
wcmulti: wcmulti.c
	gcc -O2 wcmulti.c -o wcmulti
wctest: wctest.c
	gcc -O2 wctest.c -o wctest
o3gen: generator.c
	gcc -O3 generator.c -o o3gen
