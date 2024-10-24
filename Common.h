#pragma once
#include <vector>

const double ORTHO_MAX = 100.0;
const double PI = 3.14159265358979323846;

struct Motion {
    double x;
    double y;
    double xv;
    double yv;
    double angle;
    double angularVelocity;
};

struct Point {
    Point() :x(0), y(0) {}
    Point(double x, double y) : x(x), y(y) {}
    double x;
    double y;
};

struct Asteroid {
    double radius;
    Motion motion{};
    std::vector<Point> vertices;
};

Motion step(Motion last, double delta, double screenWrap = 1.0);