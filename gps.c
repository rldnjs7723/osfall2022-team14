#include <linux/syscalls.h>
#include <linux/gps.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fcntl.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/fs.h>

DEFINE_SPINLOCK(gps_lock);

struct gps_location systemloc;

typedef struct _fblock {
  long long int integer;
  long long int fraction;
} fblock;

fblock PI = {3, 141592};
fblock R = {6371000, 0};
fblock DtoR = {0, 17453};
fblock RtoD = {57, 295779};

fblock neg(fblock val) {
  fblock result;
  result.integer = -val.integer - 1;
  result.fraction = 1000000LL -val.fraction;
  return result;
}

fblock add(fblock num1, fblock num2) {
  fblock result;
  long long int temp = 1000000LL * (num1.integer + num2.integer) + num1.fraction + num2.fraction;
  if (temp >= 0) {
    result.integer = temp / 1000000LL;
    result.fraction = temp % 1000000LL;
  }
  else {
    result.integer = temp / 1000000LL - 1;
    result.fraction = 1000000LL + temp % 1000000LL;
  }
  return result;
}

fblock sub(fblock num1, fblock num2) {
  fblock result;
  long long int temp = 1000000LL * (num1.integer - num2.integer) + num1.fraction - num2.fraction;
  if (temp >= 0) {
    result.integer = temp / 1000000LL;
    result.fraction = temp % 1000000LL;
  }
  else {
    result.integer = temp / 1000000LL - 1;
    result.fraction = 1000000LL + temp % 1000000LL;
  }
	return result;
}

fblock mul(fblock num1, fblock num2) {
	fblock result;
  long long temp = num1.integer * num2.fraction \
                 + num2.integer * num1.fraction \
                 + num1.fraction * num2.fraction / 1000000LL;
  if (temp >= 0) {
    result.integer = num1.integer * num2.integer + temp / 1000000LL;
    result.fraction = temp % 1000000LL;
  }
  else {
    result.integer = num1.integer * num2.integer + temp / 1000000LL - 1;
    result.fraction = 1000000LL + temp % 1000000LL;
  }
  return result;
}

fblock div(fblock num, long long int div) {
	fblock result;
  long long int temp = (1000000LL * num.integer + num.fraction) / div;
  if (temp >= 0) {
    result.integer = temp / 1000000LL;
    result.fraction = temp % 1000000LL;
  }
  else {
    result.integer = temp / 1000000LL - 1;
    result.fraction = 1000000LL + temp % 1000000LL;
  }
  return result;
}

fblock pow(fblock num, int exp) {
	if (exp == 0) return (fblock){1, 0};
  else return mul(num, pow(num, exp - 1));
}

long long int factorial(long long int num) {
	if (num == 0) return 1;
  else return num * factorial(num - 1);
}

fblock cos(fblock deg) {
  fblock rad, temp, result = {1, 0};
  rad = mul(deg, DtoR);
  int i;
  for(i = 1; i < 10; ++i) {
    temp = div(pow(rad, 2 * i), factorial((long long int)(2 * i)));
    if (i % 2) result = sub(result, temp);
    else result = add(result, temp);
  }
  return result;
}

fblock arccos(fblock deg) {
  fblock rad, temp, result;
  rad = div(PI, 2);
  int i;
  for(i = 0; i < 10; ++i) {
    temp = div(div(mul(pow(deg, 2 * i), (fblock){factorial((long long int)(2 * i)), 0}),
                       factorial((long long int)(i)) * factorial((long long int)(i)) * (long long int)(2 * i + 1)),
                   pow((fblock){4, 0}, i).integer);
    rad = sub(rad, temp);
  }
  result = mul(rad, RtoD);
  return result;
}

long long int get_dist(struct gps_location* loc1, struct gps_location* loc2) {
  fblock lat1, lng1, lat2, lng2, difflat, difflng, distance;
  //fblock dlat, dlng;
  //fblock latavg;
  //fblock x, y, d;

  lat1.integer = (long long int)loc1->lat_integer;
  lat1.fraction = (long long int)loc1->lat_fractional;
  lng1.integer = (long long int)loc1->lng_integer;
  lng1.fraction = (long long int)loc1->lng_fractional;
  lat2.integer = (long long int)loc2->lat_integer;
  lat2.fraction = (long long int)loc2->lat_fractional;
  lng2.integer = (long long int)loc2->lng_integer;
  lng2.fraction = (long long int)loc2->lng_fractional;

  difflat = sub(lat1, lat2);
  difflng = sub(lng1, lng2);

  /*dlat = (difflat.integer < 0) ? neg(difflat) : difflat;
  dlng = (difflng.integer < 0) ? neg(difflng) : difflng;
  latavg = avg(lat1, lat2);
  y = mul(DIST, dlat);
  x = mul(cos(latavg), mul(DIST, dlng));
  d = add(mul(x, x), mul(y, y));
  dist = d.integer;*/

  distance = mul(R, arccos(sub(cos(difflat), mul(mul(cos(lat1), cos(lat2)), sub((fblock){1, 0}, cos(difflng))))));
	return distance.fraction > 0 ? distance.integer + 1 : distance.integer; // round-up if fraction is not zero
}

int LocationCompare(struct gps_location *locA, struct gps_location *locB) {
  long long int accuracy_sum, distance;
  accuracy_sum = (long long int)locA->accuracy + (long long int)locB->accuracy;
  //accuracy_square = accuracy_sum * accuracy_sum;
  /*printk("locA : (%d.%d, %d.%d) / %d\nlocB : (%d.%d, %d.%d) / %d\n",
  locA->lat_integer, locA->lat_fractional, locA->lng_integer, locA->lng_fractional, locA->accuracy,
  locB->lat_integer, locB->lat_fractional, locB->lng_integer, locB->lng_fractional, locB->accuracy);*/
  distance = get_dist(locA, locB);
	return (distance <= accuracy_sum);
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc) {
  struct gps_location path_buf;
	if (copy_from_user(&path_buf, loc, sizeof(struct gps_location))) {
		printk("ERROR\n");
		return -EFAULT;
	}
	if (path_buf.lat_integer < -90 || path_buf.lat_integer > 90
  || path_buf.lng_integer < -180 || path_buf.lng_integer > 180
  || path_buf.lat_fractional < 0 || path_buf.lat_fractional > 999999
  || path_buf.lng_fractional < 0 || path_buf.lng_fractional > 999999) {
    printk("Invalid location\n");
		return -EINVAL;
  }
	spin_lock(&gps_lock);
	systemloc.lat_integer = path_buf.lat_integer;
	systemloc.lat_fractional = path_buf.lat_fractional;
	systemloc.lng_integer = path_buf.lng_integer;
	systemloc.lng_fractional = path_buf.lng_fractional;
	systemloc.accuracy = path_buf.accuracy;
	spin_unlock(&gps_lock);
	return 0;
}

SYSCALL_DEFINE2(get_gps_location, const char __user *, pathname, struct gps_location __user *, loc) {
	//char *path_buf;
	struct path path;
  unsigned int lookup_flags = LOOKUP_FOLLOW;
	struct gps_location loc_buf;
	struct inode *inode;

  /*if (strncpy_from_user(path_buf, pathname, 64) < 0) {
    printk("ERROR\n");
    return -EFAULT;
  }*/
	if (user_path_at_empty(AT_FDCWD, pathname, lookup_flags, &path, NULL)) {
		printk("ERROR\n");
		return -EFAULT;	
	}
  /*
  if (kern_path(path_buf, LOOKUP_FOLLOW, &path) < 0) {
		printk("ERROR\n");
		return -EFAULT;
	}*/
  inode = path.dentry->d_inode;
	if (!inode->i_op->get_gps_location) {
		printk("ERROR\n");
		return -ENODEV;
	}
	inode->i_op->get_gps_location(inode, &loc_buf);

	spin_lock(&gps_lock);
	if (!LocationCompare(&loc_buf, &systemloc)) {
		spin_unlock(&gps_lock);
    printk("Can't access that location\n");
		return -EACCES;
	}
	spin_unlock(&gps_lock);

	if (copy_to_user(loc, &loc_buf, sizeof(struct gps_location))) {
		printk("ERROR\n");
		return -EFAULT;
	}
	return 0;
}