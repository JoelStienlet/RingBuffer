all:
	make test_random
	make test_threads
	make valgrind
	
test_random:
	gcc -Wall -O2 -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -Wall -O2 -c TEST_random_ins_del.c -o ../outputs/TEST_random_ins_del.o -I..
	gcc ../outputs/circ_buf.o ../outputs/TEST_random_ins_del.o -o ../outputs/TEST_random_ins_del
	../outputs/TEST_random_ins_del 5 100000

test_threads:
	gcc -Wall -O2 -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -Wall -O2 -c TEST_concurrent_threads.c -o ../outputs/TEST_concurrent_threads.o -I..
	gcc ../outputs/circ_buf.o ../outputs/TEST_concurrent_threads.o -o ../outputs/TEST_concurrent_threads -lpthread
	../outputs/TEST_concurrent_threads 10 10000000

valgrind:
	gcc -g -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -g -c TEST_random_ins_del.c -o ../outputs/TEST_random_ins_del.o -I..
	gcc -g ../outputs/circ_buf.o ../outputs/TEST_random_ins_del.o -o ../outputs/TEST_random_ins_del
	valgrind ../outputs/TEST_random_ins_del 5 100000

coverage:
	gcc -fprofile-arcs -ftest-coverage -O0 -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -O2 -c TEST_random_ins_del.c -o ../outputs/TEST_random_ins_del.o -I..
	gcc -fprofile-arcs -ftest-coverage ../outputs/circ_buf.o ../outputs/TEST_random_ins_del.o -o ../outputs/TEST_random_ins_del
	../outputs/TEST_random_ins_del 5 100000
	gcc -O2 -g -c TEST_bad_pointers.c -o ../outputs/TEST_bad_pointers.o -I..
	gcc -fprofile-arcs -ftest-coverage ../outputs/circ_buf.o ../outputs/TEST_bad_pointers.o -o ../outputs/TEST_bad_pointers
	../outputs/TEST_bad_pointers
	gcc -O2 -g -c TEST_bad_buf_size.c -o ../outputs/TEST_bad_buf_size.o -I..
	gcc -fprofile-arcs -ftest-coverage ../outputs/circ_buf.o ../outputs/TEST_bad_buf_size.o -o ../outputs/TEST_bad_buf_size
	../outputs/TEST_bad_buf_size
