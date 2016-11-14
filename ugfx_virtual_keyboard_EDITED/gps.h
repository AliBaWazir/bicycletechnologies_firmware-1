#ifndef _GPS_H_
#define _GPS_H_

#define ZOOM_LEVEL 15

int long2tilex(double lon, int z);
int lat2tiley(double lat, int z);
double tilex2long(int x, int z);
double tiley2lat(int y, int z);
void runGPS();

#endif /* _GPS_H_ */