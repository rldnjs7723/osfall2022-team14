#ifndef GPS_H_
#define GPS_H_

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

long sys_set_gps_location(struct gps_location __user *loc);
long sys_get_gps_location(const char __user *pathname, struct gps_location __user *loc);

extern struct gps_location * latest_loc;

#endif

#ifndef __LINUX_GPS_lock_H__
#define __LINUX_GPS_lock_H__
#include <linux/spinlock.h>

extern spinlock_t gps_lock;

#endif
