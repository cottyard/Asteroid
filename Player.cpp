#include <math.h>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

const double bulletSize = 0.3;
const int shootCooldown = 180;
Point playerFront(1.5, 0);
Point playerRearLeft(-1.0, -1.1);
Point playerRearRight(-1.0, 1.1);

int shootTimer = 0;

bool alive;
bool shooting;
bool turning_left;
bool turning_right;
bool thrusting;
Motion motion;

std::vector<Motion> bullets;

void kill() {
    alive = false;
}

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

void destroyBullet(size_t i) {
    bullets.erase(bullets.begin() + i);
}

void initPlayer(){
    motion.x = ORTHO_MAX / 2;
    motion.y = ORTHO_MAX / 2;
    motion.xv = 0;
    motion.yv = 0;
    motion.angle = 90;
    turning_left = 0;
    turning_right = 0;
    thrusting = 0;
    shooting = false;
    alive = true;
}

void drawBullet(int i) {
	glPushMatrix();
	glTranslatef(bullets[i].x, bullets[i].y, 0.0);
	glBegin(GL_LINE_LOOP);
		for(int i=0;i<10;i++){
			glVertex2f(bulletSize * cos(2*PI*i/10),
					   bulletSize * sin(2*PI*i/10));
		}
	glEnd();
	glPopMatrix();
}

void drawBullets() {
	glColor3f(0.8,0.2,0.5);
    for (size_t i = 0; i < bullets.size(); i++) drawBullet(i);
}

void drawPlayer(){
    if (!alive) return;
    glLineWidth(2);
    glPushMatrix();
    glTranslatef(motion.x, motion.y, 0.0);
    glRotatef(motion.angle, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
	    glVertex2f(playerFront.x, playerFront.y);
	    glVertex2f(playerRearLeft.x, playerRearLeft.y);
	    glVertex2f(-0.5, 0.0);
	    glVertex2f(playerRearRight.x, playerRearRight.y);
	    glVertex2f(playerFront.x, playerFront.y);
    glEnd();
    if (thrusting) {
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
	        glVertex2f(-0.75, -0.5);
	        glVertex2f(-1.75, 0);
	        glVertex2f(-0.75, 0.5);
        glEnd();
    }
    glPopMatrix();
    glLineWidth(1);
}

bool inBoundary(int x) {
    return x >= 0 && x <= ORTHO_MAX;
}

void updatePlayer(int delta){
    if (!alive) return;
    if (turning_left){
        motion.angle = motion.angle + delta * 0.3;
    }
    if (turning_right){
        motion.angle = motion.angle - delta * 0.3;
    }
    if (thrusting){
        double acceleration = delta * 0.00003;
        motion.xv = motion.xv + cos(motion.angle*PI/180.0) * acceleration;
        motion.yv = motion.yv + sin(motion.angle*PI/180.0) * acceleration;
    }
    if (shooting && shootTimer >= shootCooldown) {
        bullets.push_back(initBullet(motion));
        shootTimer = 0;
    }
    shootTimer += delta;
    motion = step(motion, delta);
}

void updateBullets(int delta) {
    for (int i = bullets.size() - 1; i >= 0; i--) {
        if (inBoundary(bullets[i].x) && inBoundary(bullets[i].y)) {
            bullets[i] = step(bullets[i], delta, 1.2);
        }
        else {
            destroyBullet(i);
        }
    }
}

double calcDistance(Point a, Point b) {
    return sqrt(pow(abs(a.x - b.x), 2) + pow(abs(a.y - b.y), 2));
}

bool collideWith(Point p, Asteroid ast) {
    return calcDistance(p, Point(ast.motion.x, ast.motion.y)) < ast.radius;
}

bool testPlayerCollision(Asteroid ast) {
    if (!alive) false;
    return collideWith(Point(motion.x + playerFront.x, motion.y + playerFront.y), ast) ||
           collideWith(Point(motion.x + playerRearLeft.x, motion.y + playerRearLeft.y), ast) ||
           collideWith(Point(motion.x + playerRearRight.x, motion.y + playerRearRight.y), ast) ||
           collideWith(Point(motion.x, motion.y), ast);
}

std::vector<size_t> testBulletsHit(Asteroid ast) {
    std::vector<size_t> hitBullets;
    for (size_t i = 0; i < bullets.size(); ++i) {
        if (collideWith(Point(bullets[i].x, bullets[i].y), ast)) {
            hitBullets.push_back(i);
        }
    }
    return hitBullets;
}

void onKeyPressed(unsigned char key, int px, int py) {
    switch (key) {
    case 27:
        exit(0);
        break;
    case ' ':
        if (alive) shooting = true;
        else initPlayer();
        break;        
    }
}

void onKeyUp(unsigned char key, int px, int py) {
    switch (key) {
    case ' ':
        shooting = false;
        break;
    }
}

void onSpecialKeyPressed(int key, int x, int y){
    int time = glutGet(GLUT_ELAPSED_TIME);
    switch (key){
    case GLUT_KEY_UP:
        thrusting = true;
        break;
    case GLUT_KEY_LEFT:
        turning_left = true;
        break;
    case GLUT_KEY_RIGHT:
        turning_right = true;
        break;
    }
}

void onSpecialKeyUp(int key, int x, int y){
    switch (key){
    case GLUT_KEY_UP:
        thrusting = false;
        break;
    case GLUT_KEY_LEFT:
        turning_left = false;
        break;
    case GLUT_KEY_RIGHT:
        turning_right = false;
        break;
    }
}
