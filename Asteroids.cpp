#include <cstdlib>
#include <math.h>
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"
#include "Common.h"

Motion initBullet(Motion player) {
    Motion bullet{};
    float dx = cos(player.angle * PI / 180.0);
    float dy = sin(player.angle * PI / 180.0);
    bullet.x = player.x + 2 * dx;
    bullet.y = player.y + 2 * dy;
    bullet.xv = player.xv + dx * 0.04;
    bullet.yv = player.yv + dy * 0.04;
    bullet.angle = player.angle;
    return bullet;
}

World initWorld(){
	World w{};
    w.player.x = ORTHO_MAX / 2;
	w.player.y = ORTHO_MAX / 2;
	w.player.xv = 0;
	w.player.yv = 0;
	w.player.angle = 90;
	w.turning_left = 0;
	w.turning_right = 0;
	w.thrusting = 0;
	w.shooting = false;
	w.alive = true;
	return w;
}

Point randomSpawnPoint() {
    int x, y;
    if (rand() % 2) x = -10;
    else x = ORTHO_MAX + 10;
    y = rand() % (int(ORTHO_MAX) + 1);
    if (rand() % 2) return Point(x, y);
    else return Point(y, x);
}

Asteroid createAsteroid(Point at, double radius){
    Asteroid a;
    a.radius = radius;
    a.motion.x = at.x;
    a.motion.y = at.y;
    a.motion.angle = 0;
    a.motion.angularVelocity = (rand() % 21 - 10) / 100.0;
    int direction = rand() % 360;
    double speed = (rand() % 30 + 100) / 10000.0;
    a.motion.xv = speed * cos(direction * PI / 180);
    a.motion.yv = speed * sin(direction * PI / 180);
    return a;
}

Asteroid spawnAsteroid() {
    return createAsteroid(randomSpawnPoint(), rand() % 3 + 3);
}

bool inBoundary(int x) {
    return x >= 0 && x <= ORTHO_MAX;
}

double calcDistance(Point a, Point b) {
    return sqrt(pow(abs(a.x - b.x), 2) + pow(abs(a.y - b.y), 2));
}

bool collideWith(Point p, Asteroid ast) {
    return calcDistance(p, Point(ast.motion.x, ast.motion.y)) < ast.radius;
}

std::vector<size_t> testBulletsHit(const std::vector<Motion>& bullets, Asteroid ast) {
    std::vector<size_t> hitBullets;
    for (size_t i = 0; i < bullets.size(); ++i) {
        if (collideWith(Point(bullets[i].x, bullets[i].y), ast)) {
            hitBullets.push_back(i);
        }
    }
    return hitBullets;
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

Motion stepMotion(Motion last, double delta, double screenWrap  = 1.0) {
    double upperBound = ORTHO_MAX * screenWrap;
    double lowerBound = -(screenWrap - 1) * ORTHO_MAX;
    Motion next = last;
    next.x = wrapAxis(last.x + last.xv * delta, lowerBound, upperBound);
    next.y = wrapAxis(last.y + last.yv * delta, lowerBound, upperBound);
    next.angle += last.angularVelocity * delta;
    return next;
}

void stepWorld(World& world, int delta) {
    if (world.turning_left){
        world.player.angle = world.player.angle + delta * 0.3;
    }
    if (world.turning_right){
        world.player.angle = world.player.angle - delta * 0.3;
    }
    if (world.thrusting){
        double acceleration = delta * 0.00003;
        world.player.xv = world.player.xv + cos(world.player.angle*PI/180.0) * acceleration;
        world.player.yv = world.player.yv + sin(world.player.angle*PI/180.0) * acceleration;
    }
    if (world.shooting && world.shootTimer >= shootCooldown) {
        world.bullets.push_back(initBullet(world.player));
        world.shootTimer = 0;
    }
    world.shootTimer += delta;
    world.player = stepMotion(world.player, delta);
    
    for (int i = world.bullets.size() - 1; i >= 0; i--) {
        if (inBoundary(world.bullets[i].x) && inBoundary(world.bullets[i].y)) {
            world.bullets[i] = stepMotion(world.bullets[i], delta, 1.2);
        }
        else {
            world.bullets.erase(world.bullets.begin() + i);
        }
    }
    
    world.spawnTimer += delta;
    if (world.spawnTimer >= spawnCooldown) {
        world.spawnTimer = 0;
        world.asteroids.push_back(spawnAsteroid());
    }
    for (size_t i = 0; i < world.asteroids.size(); i++) {
        world.asteroids[i].motion = stepMotion(world.asteroids[i].motion, delta, 1.1);
    }
    
    std::vector<size_t> asteroidsToDestroy;
    for (size_t i = 0; i < world.asteroids.size(); i++) {
        if (world.alive && collideWith(Point(world.player.x, world.player.y), world.asteroids[i])) {
            world.alive = false;
            break;
        }
        std::vector<size_t> hitBullets = testBulletsHit(world.bullets, world.asteroids[i]);
        for (auto bulletId: hitBullets) world.bullets.erase(world.bullets.begin() + bulletId);
        if (hitBullets.size() > 0) {
            asteroidsToDestroy.push_back(i);
        }
    }
    
    for (auto id: asteroidsToDestroy) {
	    Asteroid ast = world.asteroids[id];
    	world.asteroids.erase(world.asteroids.begin() + id);
    	if (ast.radius > 2) {
	    	for (int i = 0; i < 2; i++) {
	            world.asteroids.push_back(
					createAsteroid(Point(ast.motion.x, ast.motion.y), 
								   ast.radius / 2));
	        }	
		}        
	}
	
    world.score += asteroidsToDestroy.size();
}
