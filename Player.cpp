#include <math.h>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

void newBullet(Motion);
void newMissile(Motion);
bool collideWithAsteroid(Point p, Asteroid ast);

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

void kill() {
    alive = false;
}

void initPlayer(){
    motion.at.x = ORTHO_MAX / 2;
    motion.at.y = ORTHO_MAX / 2;
    motion.v.x = 0;
    motion.v.y = 0;
    motion.angle = 90;
    turning_left = 0;
    turning_right = 0;
    thrusting = 0;
    shooting = false;
    alive = true;
}

void drawPlayer(){
    if (!alive) return;
    glLineWidth(2);
    glPushMatrix();
    glTranslatef(motion.at.x, motion.at.y, 0.0);
    glRotatef(motion.angle, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
	    glVertex2f(playerFront.x, playerFront.y);
	    glVertex2f(playerRearLeft.x, playerRearLeft.y);
	    glVertex2f(-0.5, 0.0);
	    glVertex2f(playerRearRight.x, playerRearRight.y);
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

void updatePlayer(int delta){
    if (!alive) return;
    
    if (turning_left) motion.angle = modulate(motion.angle + delta * 0.3);
    if (turning_right) motion.angle = modulate(motion.angle - delta * 0.3);
    if (thrusting) motion = accelerate(motion, delta * 0.00003);
    
    if (shooting && shootTimer >= shootCooldown) {
        newMissile(motion);
        shootTimer = 0;
    }
    shootTimer += delta;
    motion = step(motion, delta);
}

bool testPlayerCollision(Asteroid ast) {
    if (!alive) false;
    return collideWithAsteroid(motion.at + playerFront, ast) ||
           collideWithAsteroid(motion.at + playerRearLeft, ast) ||
           collideWithAsteroid(motion.at + playerRearRight, ast) ||
           collideWithAsteroid(motion.at, ast);
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
