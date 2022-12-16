cc=arm-linux-gnueabi-gcc 

all: gpsupdate file_write file_loc 

gpsupdate: gpsupdate.c
	$(cc) -I../include gpsupdate.c -o gpsupdate

file_write: file_write.c
	$(cc) -I../include file_write.c -o file_write

file_loc: file_loc.c
	$(cc) -I../include file_loc.c -o file_loc

clean: 
	rm -f gpsupdate file_loc file_write
