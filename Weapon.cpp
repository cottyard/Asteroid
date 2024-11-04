#include <math.h>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

bool collideWithAsteroid(Point, Asteroid);
const Asteroid* findNearestAsteroid(Point);

const double bulletSize = 0.3;
const double missileTurnRate = 0.3;
const double missileFule = 0.5;
const double missileVelocityCap = 0.05;

struct Missile {
	Missile(double fuel, Point target, Motion motion):
		fuel(fuel), target(target), motion(motion), acceleration(0) {}
	double fuel;
	double acceleration;
	Point target;
	Motion motion{};
};

std::vector<Motion> bullets;
std::vector<Missile> missiles;

Motion initProjectile(Motion player, double velocityBoost) {
    Motion p{};
    float dx = cos(player.angle * PI / 180.0);
    float dy = sin(player.angle * PI / 180.0);
    p.at.x = player.at.x + 2 * dx;
    p.at.y = player.at.y + 2 * dy;
    p.v.x = player.v.x + dx * velocityBoost;
    p.v.y = player.v.y + dy * velocityBoost;
    p.angle = player.angle;
    return p;
}

void newBullet(Motion player) {
	bullets.push_back(initProjectile(player, 0.04));
}

void newMissile(Motion player) {
	const Asteroid* a = findNearestAsteroid(player.at);
	Point target = a ? a->motion.at : player.at;
	missiles.push_back(Missile(
		missileFule,
		target,
		initProjectile(player, 0)
	));
}

void drawBullet(Motion b) {
	glPushMatrix();
	glTranslatef(b.at.x, b.at.y, 0.0);
	glBegin(GL_LINE_LOOP);
		for(int i=0;i<10;i++){
			glVertex2f(bulletSize * cos(2*PI*i/10),
					   bulletSize * sin(2*PI*i/10));
		}
	glEnd();
	glPopMatrix();
}

void drawMissile(Missile missile) {
	Motion m = missile.motion;
	glPushMatrix();
	glTranslatef(m.at.x, m.at.y, 0.0);
	glRotatef(m.angle, 0.0, 0.0, 1.0);
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINE_LOOP);
	    glVertex2f(0.7, 0);
	    glVertex2f(-0.7, -0.4);
	    glVertex2f(-0.7, 0.4);
	glEnd();
	if (missile.acceleration > 0) {
		double size = pow(missile.acceleration / 0.001, 0.3);
		glColor3f(1.0, 0.0, 0.0);
	    glBegin(GL_LINE_STRIP);
	        glVertex2f(-0.7*size, -0.3*size);
	        glVertex2f(-1.5*size, 0);
	        glVertex2f(-0.7*size, 0.3*size);
	    glEnd();	
	}
	glPopMatrix();
}

void drawProjectiles() {
	glColor3f(0.8,0.2,0.5);
    for (size_t i = 0; i < bullets.size(); i++) drawBullet(bullets[i]);
    for (size_t i = 0; i < missiles.size(); i++) drawMissile(missiles[i]);
}

bool inBoundary(int x) {
    return x >= 0 && x <= ORTHO_MAX;
}

double lorentzian(double x, double peek, double span, double base) {
	return peek / (x*x + span) + base;
}

void updateProjectiles(int delta) {
    for (int i = bullets.size() - 1; i >= 0; i--) {
        if (inBoundary(bullets[i].at.x) && inBoundary(bullets[i].at.y)) {
            bullets[i] = step(bullets[i], delta, 1.2);
        }
        else {
            bullets.erase(bullets.begin() + i);
        }
    }
    for (int i = missiles.size() - 1; i >= 0; i--) {
    	Motion m = missiles[i].motion;
    	Point target = missiles[i].target;
    	const Asteroid* a = findNearestAsteroid(missiles[i].target);
    	if (a) {
    		missiles[i].target = a->motion.at;
    		double distance = calcDistance(m.at, a->motion.at);
    		double estimatedDelta = distance / missileVelocityCap;
    		Motion predicted = step(a->motion, estimatedDelta, 1.1);
    		target = predicted.at;
		}
    	if (missiles[i].fuel > 0.001) {
	    	double desiredAngle = angleTo(m.at, target);
	    	double correctionAngle = modulate(desiredAngle - m.angle);
	    	double correctionBearing = modulate(bearing(m) - desiredAngle);
	    	double da = correctionAngle;
	    	if (da > missileTurnRate) da = missileTurnRate;
	    	if (da < -missileTurnRate) da = -missileTurnRate;
	    	m.angle = modulate(m.angle + da * delta);
    		double v = velocity(m);
    		if (v > missileVelocityCap) v = missileVelocityCap;
			double a = delta * 0.0001 * pow((1 - v / missileVelocityCap), 2) * 
					   lorentzian(correctionAngle, 100, 100, 0) *
			           lorentzian(modulate(correctionBearing + 180), 100000, 2000, 1);
			m = accelerate(m, a);
    		missiles[i].fuel -= a;
    		missiles[i].acceleration = a;
		} else {
			missiles[i].acceleration = 0;
		}
        missiles[i].motion = step(m, delta);
    }
}

int destroyHitProjectiles(Asteroid ast) {
	int hits = 0;
    for (int i = bullets.size() - 1; i >= 0; i--) {
        if (collideWithAsteroid(bullets[i].at, ast)) {
        	bullets.erase(bullets.begin() + i);
            hits++;
        }
    }
    for (int i = missiles.size() - 1; i >= 0; i--) {
        if (collideWithAsteroid(missiles[i].motion.at, ast)) {
        	missiles.erase(missiles.begin() + i);
            hits++;
        }
    }
    return hits;
}

