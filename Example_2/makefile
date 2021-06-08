

all:
	gcc -Wall -O2 -c ../circ_buf.c -o ../outputs/circ_buf.o
	gcc -Wall -O2 -c 02_ex_DMA.c -o ../outputs/02_ex_DMA.o -I..
	gcc ../outputs/circ_buf.o ../outputs/02_ex_DMA.o -o ../outputs/02_ex_DMA -lpthread
	../outputs/02_ex_DMA
	
