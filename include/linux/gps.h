#ifndef GPS_H_
#define GPS_H_

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};
typedef struct _fblock {
  long long int integer;
  long long int fraction;
} fblock;

fblock myadd(fblock num1, fblock num2);
fblock mysub(fblock num1, fblock num2);
fblock mymul(fblock num1, fblock num2);
fblock mydiv(fblock num, long long int div);
fblock mypow(fblock num, int exp);
long long int myfactorial(long long int num);
fblock mycos(fblock deg);
fblock myarccos(fblock deg);
long long int get_dist(struct gps_location* loc1, struct gps_location* loc2);
int LocationCompare(struct gps_location *loc1, struct gps_location *loc2);
long sys_set_gps_location(struct gps_location __user *loc);
long sys_get_gps_location(const char __user *pathname, struct gps_location __user *loc);
extern struct gps_location * latest_loc;

#endif

#ifndef __LINUX_GPS_lock_H__
#define __LINUX_GPS_lock_H__
#include <linux/spinlock.h>

extern spinlock_t gps_lock;

#endif
