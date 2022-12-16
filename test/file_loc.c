#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <asm-generic/errno-base.h>

#define GET_GPS_LOCATION 399

struct gps_location{
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

int main(int argc, char *argv[]) {
	struct gps_location loc;
	if(argc != 2) {
		printf("Type \"./file_write [filename]\"\n");
		return -1;
	}
	if (syscall(GET_GPS_LOCATION, argv[1], &loc)) {
		printf("ERROR");
		return -1;
	}
	printf("----------File info---------\n");
	printf("GPS coordinate:	(latitude, longitude) / accuracy = (%d.%d, %d.%d) / %d\n", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lat_fractional, loc.accuracy);
	printf("Google map link: https://www.google.co.kr/maps/place/@%d.%d,%d.%d,%dz/\n", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lat_fractional, loc.accuracy);
	return 0;
}
