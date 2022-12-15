cc=arm-linux-gnueabi-gcc 


all: gpsupdate file_loc  file_write 

gpsupdate: gpsupdate.c
	$(cc) -I../include gpsupdate.c -o gpsupdate

file_loc: file_loc.c
	$(cc) -I../include file_loc.c -o file_loc

file_write: file_write.c
	$(cc) -I../include file_write.c -o file_write




clean: 
	rm -f gpsupdate file_loc file_write

