#include <linux/syscalls.h>
#include <linux/gps.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fcntl.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/fs.h>

DEFINE_SPINLOCK(gps_lock);

struct gps_location * latest_loc;

fblock PI = {3, 141592};
fblock R = {6371000, 0};
fblock DtoR = {0, 17453};
fblock RtoD = {57, 295779};

fblock myadd(fblock num1, fblock num2) {
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

fblock mysub(fblock num1, fblock num2) {
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

fblock mymul(fblock num1, fblock num2) {
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

fblock mydiv(fblock num, long long int mydiv) {
	fblock result;
  long long int temp = (1000000LL * num.integer + num.fraction) / mydiv;
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

fblock mypow(fblock num, int exp) {
	if (exp == 0) return (fblock){1, 0};
  else return mymul(num, mypow(num, exp - 1));
}

long long int myfactorial(long long int num) {
	if (num == 0) return 1;
  else return num * myfactorial(num - 1);
}

fblock mycos(fblock deg) {
  fblock rad, temp, result = {1, 0};
  rad = mymul(deg, DtoR);
  int i;
  for(i = 1; i < 10; ++i) {
    temp = mydiv(mypow(rad, 2 * i), myfactorial((long long int)(2 * i)));
    if (i % 2) result = mysub(result, temp);
    else result = myadd(result, temp);
  }
  return result;
}

fblock myarccos(fblock deg) {
  fblock rad, temp, result;
  rad = mydiv(PI, 2);
  int i;
  for(i = 0; i < 10; ++i) {
    temp = mydiv(mydiv(mymul(mypow(deg, 2 * i), (fblock){myfactorial((long long int)(2 * i)), 0}),
                       myfactorial((long long int)(i)) * myfactorial((long long int)(i)) * (long long int)(2 * i + 1)),
                   mypow((fblock){4, 0}, i).integer);
    rad = mysub(rad, temp);
  }
  result = mymul(rad, RtoD);
  return result;
}

long long int get_dist(struct gps_location* loc1, struct gps_location* loc2) {
  fblock lat1, lng1, lat2, lng2, difflat, difflng, distance;

  lat1.integer = (long long int)loc1->lat_integer;
  lat1.fraction = (long long int)loc1->lat_fractional;
  lng1.integer = (long long int)loc1->lng_integer;
  lng1.fraction = (long long int)loc1->lng_fractional;
  lat2.integer = (long long int)loc2->lat_integer;
  lat2.fraction = (long long int)loc2->lat_fractional;
  lng2.integer = (long long int)loc2->lng_integer;
  lng2.fraction = (long long int)loc2->lng_fractional;

  difflat = mysub(lat1, lat2);
  difflng = mysub(lng1, lng2);

  distance = mymul(R, myarccos(mysub(mycos(difflat), mymul(mymul(mycos(lat1), mycos(lat2)), mysub((fblock){1, 0}, mycos(difflng))))));
	return distance.fraction > 0 ? distance.integer + 1 : distance.integer; // round-up if fraction is not zero
}

int LocationCompare(struct gps_location *loc1, struct gps_location *loc2) {
  long long int accuracy_sum, distance;
  accuracy_sum = (long long int)loc1->accuracy + (long long int)loc2->accuracy;
  distance = get_dist(loc1, loc2);
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
	latest_loc->lat_integer = path_buf.lat_integer;
	latest_loc->lat_fractional = path_buf.lat_fractional;
	latest_loc->lng_integer = path_buf.lng_integer;
	latest_loc->lng_fractional = path_buf.lng_fractional;
	latest_loc->accuracy = path_buf.accuracy;
	spin_unlock(&gps_lock);
	return 0;
}

SYSCALL_DEFINE2(get_gps_location, const char __user *, pathname, struct gps_location __user *, loc) {
	struct path path;
  unsigned int lookup_flags = LOOKUP_FOLLOW;
	struct gps_location loc_buf;
	struct inode *inode;

	if (user_path_at_empty(AT_FDCWD, pathname, lookup_flags, &path, NULL)) {
		printk("ERROR\n");
		return -EFAULT;	
	}
  inode = path.dentry->d_inode;
	if (!inode->i_op->get_gps_location) {
		printk("ERROR\n");
		return -ENODEV;
	}
	inode->i_op->get_gps_location(inode, &loc_buf);
	spin_lock(&gps_lock);
	if (!LocationCompare(&loc_buf, latest_loc)) {
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