#pragma once
#include <vector>

const double ORTHO_MAX = 100.0;
const double PI = 3.14159265358979323846;

const int spawnCooldown = 1000;
const int shootCooldown = 300;

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
};

struct World {
	int score = 0;
	int spawnTimer = 0;	
	int shootTimer = 0;
	bool alive;
	bool shooting;
	bool turning_left;
	bool turning_right;
	bool thrusting;
	Motion player;
	std::vector<Motion> bullets;
	std::vector<Asteroid> asteroids;
};
