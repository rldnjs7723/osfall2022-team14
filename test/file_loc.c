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
<<<<<<< HEAD
		printf("Type \"./file_write [filename]\"\n");
		return -1;
	}
	char *tmp = malloc(64);
	strcpy(tmp, "/root/proj4/");
	strcat(tmp, argv[1]);
	if (syscall(GET_GPS_LOCATION, tmp, &loc)) {
		printf("ERROR");
=======
		printf("Type \"./file_loc [filename]\"\n");
		return -1;
	}
	char *tmp = malloc(32);
	strcpy(tmp, "/root/proj4/");
	strcat(tmp, argv[1]);
	if (syscall(GET_GPS_LOCATION, tmp, &loc)) {
		printf("ERROR\n");
>>>>>>> 94b4f7c138b8ef0df0c164df19688240e029ebf1
		return -1;
	}
	printf("Location information: (latitude, longitude) / accuracy = (%d.%d, %d.%d) / %d\n", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lat_fractional, loc.accuracy);
	return 0;
}
