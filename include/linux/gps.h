#ifndef __LINUX_GPS_H__
#define __LINUX_GPS_H__

#include <linux/spinlock.h>

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

extern spinlock_t gps_lock;
extern struct gps_location systemloc;
extern int LocationCompare(struct gps_location *locA, struct gps_location *locB);
#endif
