#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <asm-generic/errno-base.h>

#define SET_GPS_LOCATION 398

struct gps_location{
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

int main(int argc, char *argv[]) {
    if(argc != 6) {
        printf("Type \"./gpsupdate [lat_int] [lat_frac] [lng_int] [lng_frac] [accuracy]\n");
		return -1;
    }
    struct gps_location loc = {atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5])};
	if (syscall(SET_GPS_LOCATION, &loc)) {
		printf("ERROR");
		return -1;
	}
	return 0;
}


