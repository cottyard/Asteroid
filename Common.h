#pragma once
#include <vector>

const double ORTHO_MAX = 100.0;
const double PI = 3.14159265358979323846;

struct Point {
    Point() :x(0), y(0) {}
    Point(double x, double y) : x(x), y(y) {}
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    double x;
    double y;
};

struct Motion {
    Point at;
    Point v;
    double angle;
    double angularVelocity;
};

struct Asteroid {
    double radius;
    Motion motion{};
    std::vector<Point> vertices;
};

double velocity(Motion m);
double bearing(Motion m);
double angleTo(Point from, Point to);
double calcDistance(Point a, Point b);
double modulate(double angle);

Motion accelerate(Motion motion, double acceleration);
Motion step(Motion last, double delta, double screenWrap = 1.0);

