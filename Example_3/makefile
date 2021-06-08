
all:
	gcc -Wall -O2 -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -Wall -O2 -c 03_ex_parser.c -o ../outputs/03_ex_parser.o
	gcc -Wall -O2 -c 03_ex_serial_CRC.c -o ../outputs/03_ex_serial_CRC.o -I..
	gcc ../outputs/circ_buf.o ../outputs/03_ex_serial_CRC.o ../outputs/03_ex_parser.o -o ../outputs/03_ex_serial_CRC -lpthread -lz
	../outputs/03_ex_serial_CRC


test_parser:
	gcc -Wall -c 03_ex_parser.c -o ../outputs/03_ex_parser.o
	gcc -Wall -c 03_ex_test_parser.c -o ../outputs/03_ex_test_parser.o
	gcc ../outputs/03_ex_parser.o ../outputs/03_ex_test_parser.o -o ../outputs/03_ex_test_parser -lz
	valgrind ../outputs/03_ex_test_parser
