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

#endif