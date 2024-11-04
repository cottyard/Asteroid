#include "Common.h"
#include <math.h>

double velocity(Motion m) {
	return sqrt(pow(m.v.x, 2) + pow(m.v.y, 2));
}

double bearing(Motion m) {
	return atan2(m.v.y, m.v.x) * 180.0 / PI;
}

double angleTo(Point from, Point to) {
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    return atan2(dy, dx) * 180.0 / PI;
}

double calcDistance(Point a, Point b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

double wrapAxis(double x, double lowerBound, double upperBound) {
    if (x < lowerBound) {
        return x + upperBound - lowerBound;
    }
    else if (x > upperBound) {
        return x - upperBound + lowerBound;
    }
    return x;
}

Motion accelerate(Motion motion, double acceleration) {
	motion.v = Point(motion.v.x + cos(motion.angle*PI/180.0) * acceleration,
	                 motion.v.y + sin(motion.angle*PI/180.0) * acceleration);
    return motion;
}

double modulate(double angle) {
	while (angle >= 180) angle -= 360;
	while (angle <= -180) angle += 360;
	return angle;
}

Motion step(Motion last, double delta, double screenWrap) {
    double upperBound = ORTHO_MAX * screenWrap;
    double lowerBound = -(screenWrap - 1) * ORTHO_MAX;
    Motion next = last;
    next.at.x = wrapAxis(last.at.x + last.v.x * delta, lowerBound, upperBound);
    next.at.y = wrapAxis(last.at.y + last.v.y * delta, lowerBound, upperBound);
    next.angle += modulate(last.angularVelocity * delta);
    return next;
}

