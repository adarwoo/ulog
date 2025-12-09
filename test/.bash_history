ls
ls ..
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os   -I../include   -c ../src/ulog.c ../src/ulog_avr_none_gnu.cpp test_c.c test.cpp   -o test.elf
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os   -I../include ../src/ulog.c ../src/ulog_avr_none_gnu.cpp test_c.c test.cpp   -o test.elf
avr-g++ -mmcu=atmega328p -DF_CPU=16000000UL -Os   -I../include ../src/ulog.c ../src/ulog_avr_none_gnu.cpp test_c.c test.cpp   -o test.elf
avr-gcc -c -mmcu=atmega328p -DF_CPU=16000000UL -Os -I../include test_c.c -o test.elf
avr-gcc -c -mmcu=atmega328p -DF_CPU=16000000UL -Os -I../include test_c.c -o test_c.o
avr-gcc -c -mmcu=atmega328p -DF_CPU=16000000UL -Os -I../include test_c.c -o test_c.o
avr-objdump -x test_c.o
avr-objdump -a test_c.o
avr-objdump -X test_c.o
avr-objdump -s test_c.o
avr-objdump -S test_c.o
avr-gcc -c -mmcu=atmega328p -DF_CPU=16000000UL -Os -I../include test_c.c -o test_c.o
avr-gcc -c -mmcu=atmega328p -DF_CPU=16000000UL -Os -I../include test_c.c -o test_c.o
avr-objdump -s test_c.o 
avr-objdump -S test_c.o 
cd /mnt/d/eng/dev/ulog/test && make clean && make test && ./test
