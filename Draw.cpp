#include <math.h>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

Point playerFront(1.5, 0);
Point playerRearLeft(-1.0, -1.1);
Point playerRearRight(-1.0, 1.1);

void drawCircle(double radius) {
	glBegin(GL_LINE_LOOP);
		for(int i=0; i<10; i++){
			glVertex2f(radius * cos(2*PI*i/10),
					   radius * sin(2*PI*i/10));
		}
	glEnd();
}

void drawWorld(World world){
    if (world.alive) {
    	glPushMatrix();
	    glTranslatef(world.player.x, world.player.y, 0.0);
	    glRotatef(world.player.angle, 0.0, 0.0, 1.0);
	    glColor3f(1.0, 1.0, 0.0);
	    glBegin(GL_LINE_LOOP);
	    glVertex2f(playerFront.x, playerFront.y);
	    glVertex2f(playerRearLeft.x, playerRearLeft.y);
	    glVertex2f(-0.5, 0.0);
	    glVertex2f(playerRearRight.x, playerRearRight.y);
	    glVertex2f(playerFront.x, playerFront.y);
	    glEnd();
	    if (world.thrusting) {
	        glColor3f(1.0, 0.0, 0.0);
	        glBegin(GL_LINE_STRIP);
	        glVertex2f(-0.75, -0.5);
	        glVertex2f(-1.75, 0);
	        glVertex2f(-0.75, 0.5);
	        glEnd();
	    }
	    glPopMatrix();
	}
    
    for (auto& b: world.bullets) {
		glPushMatrix();
		glTranslatef(b.x, b.y, 0.0);
		glColor3f(0.8,0.2,0.5);
		drawCircle(0.3);
		glPopMatrix();
	}
	
    for (auto& asteroid: world.asteroids) {
    	glPushMatrix();
	    glTranslatef(asteroid.motion.x, asteroid.motion.y, 0.0);
	    glRotatef(asteroid.motion.angle, 0.0, 0.0, 1.0);
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    glColor3f(0.9, 0.9, 0.9);
	    drawCircle(asteroid.radius);
	    glPopMatrix();
	}
}

