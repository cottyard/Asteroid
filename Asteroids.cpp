#include <cstdlib>
#include <math.h>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

const int dustFadingTime = 1500;
const int spawnCooldown = 2000;
int spawnTimer = spawnCooldown;

struct Dust {
    double color[3];
    Motion motion;
    int timeExisted;
};

std::vector<Asteroid> asteroids;
std::vector<Dust> dusts;

Point randomSpawnPoint() {
    int x, y;
    if (rand() % 2) x = -10;
    else x = ORTHO_MAX + 10;
    y = rand() % (int(ORTHO_MAX) + 1);
    if (rand() % 2) return Point(x, y);
    else return Point(y, x);
}

Asteroid createAsteroid(int vertexCount, Point at, double radius){
    Asteroid a;
    a.radius = radius;
    a.motion.at = at;
    a.motion.angle = 0;
    a.motion.angularVelocity = (rand() % 21 - 10) / 100.0;
    int direction = rand() % 360;
    double speed = (rand() % 30 + 100) / 10000.0;
    a.motion.v = Point(speed * cos(direction * PI / 180),
					   speed * sin(direction * PI / 180));
    int angle = 0;
    for (int i = 0; i < vertexCount; i++) {
        a.vertices.push_back(Point(
            a.radius * cos(angle * PI / 180),
            a.radius * sin(angle * PI / 180)));
        angle += (360 / vertexCount + (rand() % 21 - 10));
    }
    return a;
}

Dust createDust(Point at) {
    Dust d{};
    d.color[0] = (double)rand() / RAND_MAX;
    d.color[1] = (double)rand() / RAND_MAX;
    d.color[2] = (double)rand() / RAND_MAX;
    Motion m{};
    m.at = at;
    int direction = rand() % 360;
    double speed = (rand() % 100 + 100) / 10000.0;
    m.v = Point(speed * cos(direction * PI / 180),
    			speed * sin(direction * PI / 180));
    d.motion = m;
    return d;
}

void destroyAsteroid(size_t i) {
    Asteroid ast = asteroids[i];
    asteroids.erase(asteroids.begin() + i);
    int dustCountMin = (int)ast.radius * 2 + 1;
    int dustCount = rand() % dustCountMin + dustCountMin;
    for (int i = 0; i < dustCount; i++) {
        dusts.push_back(createDust(ast.motion.at));
    }
    int vertexCount = ast.vertices.size();
    Point p = ast.motion.at;
    double r = ast.radius / 2;
    if (vertexCount == 7) {
        asteroids.push_back(createAsteroid(4, p, r));
        asteroids.push_back(createAsteroid(5, p, r));
    }
    else if (vertexCount == 6) {
        for (int i = 0; i < 2; i++) {
            asteroids.push_back(createAsteroid(4, p, r));
        }
    }
    else if (vertexCount == 5) {
        for (int i = 0; i < 3; i++) {
            asteroids.push_back(createAsteroid(3, p, r));
        }
    }
    else if (vertexCount == 4) {
        for (int i = 0; i < 2; i++) {
            asteroids.push_back(createAsteroid(3, p, r));
        }
    }
}
 
Asteroid spawnAsteroid() {
    return createAsteroid(rand() % 4 + 4, randomSpawnPoint(), rand() % 3 + 3);
}

void drawDust(Dust dust) {
    glPointSize(2.2);
    glBegin(GL_POINTS);
	    glColor4f(dust.color[0], dust.color[1], dust.color[2],
				  2 - 2 * ((GLfloat)dust.timeExisted / dustFadingTime));
	    glVertex2f(dust.motion.at.x, dust.motion.at.y);
    glEnd();
}

void drawDusts() {
    for (const auto& ds: dusts) drawDust(ds);
}

void updateDusts(int delta) {
    for (int i = dusts.size() - 1; i >= 0; i--) {
        if (dusts[i].timeExisted >= dustFadingTime) {
            dusts.erase(dusts.begin() + i);
        }
        else {
            dusts[i].motion = step(dusts[i].motion, delta, 1.1);
            dusts[i].timeExisted += delta;
        }
    }
}

void drawAsteroid(const Asteroid& asteroid) {
    glPushMatrix();
    glTranslatef(asteroid.motion.at.x, asteroid.motion.at.y, 0.0);
    glRotatef(asteroid.motion.angle, 0.0, 0.0, 1.0);
    glColor3f(0.9, 0.9, 0.9);
    glBegin(GL_LINE_LOOP);
    for (const auto& v : asteroid.vertices) glVertex2f(v.x, v.y);
    glEnd();
    glPopMatrix();
}

void drawAsteroids() {
    for (const auto& ast: asteroids) drawAsteroid(ast);
}

void updateAsteroids(int delta){
    spawnTimer += delta;
    if (spawnTimer >= spawnCooldown) {
        spawnTimer = 0;
        asteroids.push_back(spawnAsteroid());
    }
    for (size_t i = 0; i < asteroids.size(); i++) {
        asteroids[i].motion = step(asteroids[i].motion, delta, 1.1);
    }
}

bool collideWithAsteroid(Point p, Asteroid ast, double radius) {
    return calcDistance(p, ast.motion.at) < ast.radius + radius;
}

const Asteroid* findNearestAsteroid(Point from) {
	const Asteroid* nearest = nullptr;
	double maxDistance = 1e308;
	for (size_t i = 0; i < asteroids.size(); i++) {
		const Asteroid& a = asteroids[i];
		double d = calcDistance(from, a.motion.at);
		if (d < maxDistance) {
			nearest = &a;
			maxDistance = d;
		}
	}
	return nearest;
}
