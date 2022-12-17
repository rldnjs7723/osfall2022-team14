#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <string.h>
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
	if (argc != 2) {
		printf("Type \"./file_loc [filename]\"\n");
		return -1;
	}
	char *tmp = malloc(64);
	strcpy(tmp, "/root/proj4/");
	strcat(tmp, argv[1]);
	if (syscall(GET_GPS_LOCATION, tmp, &loc)) {
		return -1;
	}
	printf("Location information: (latitude, longitude) / accuracy = (%d.%d, %d.%d) / %d\n", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lng_fractional, loc.accuracy);
	printf("Google Maps link: https://www.google.com/maps/place/%d.%d,%d.%d,%d", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lng_fractional, loc.accuracy);
	return 0;
}
