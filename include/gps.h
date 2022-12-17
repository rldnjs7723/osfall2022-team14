#include <linux/spinlock.h>

extern spinlock_t gps_lock;

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};
extern struct gps_location systemloc;
extern int LocationCompare(struct gps_location *locA, struct gps_location *locB);
//long sys_set_gps_location(struct gps_location __user *loc);
//long sys_get_gps_location(const char __user *pathname, struct gps_location __user *loc);
