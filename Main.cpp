#include <string>
#include "Common.h"
#include "GL/glew.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

const int windowSize = 800; 

int windowNumber;
int lastUpdateTime = 0;
void kill();
void drawPlayer();
void drawProjectiles();
void drawAsteroids();
void drawDusts();
void updatePlayer(int);
void updateProjectiles(int);
void updateAsteroids(int);
void updateDusts(int);
void initPlayer();
void onKeyPressed(unsigned char, int, int);
void onKeyUp(unsigned char, int, int);
void onSpecialKeyPressed(int, int, int);
void onSpecialKeyUp(int, int, int);
bool testPlayerCollision(Asteroid);
int destroyHitProjectiles(Asteroid);
void destroyAsteroid(size_t);
void setupShaders();

extern GLuint glowShaderProgram;
extern GLuint rippleShaderProgram;
GLuint frameBuffer1, textureBuffer1;
GLuint frameBuffer2, textureBuffer2;
GLuint activeFrameBuffer, activeTextureBuffer, lastRenderedTextureBuffer;

void swapFrameBuffer() {
	if (activeFrameBuffer == frameBuffer1) {
		activeFrameBuffer = frameBuffer2;
		activeTextureBuffer = textureBuffer2;
		lastRenderedTextureBuffer = textureBuffer1;
	} else {
		activeFrameBuffer = frameBuffer1;
		activeTextureBuffer = textureBuffer1;
		lastRenderedTextureBuffer = textureBuffer2;
	}
}

int score = 0;
void displayScore(){
    glColor3f(0.0, 1.0, 1.0);
    glRasterPos2i(1, ORTHO_MAX - 3);
    std::string s = "Score: ";
    s += std::to_string(score);
    int i = 0;
    while (i < s.size()) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, s[i]);
        i++;
    }
}

extern std::vector<Asteroid> asteroids;
int annihilateAsteroids() {
	std::vector<size_t> asteroidsToDestroy;
    for (size_t i = 0; i < asteroids.size(); i++) {
        if (testPlayerCollision(asteroids[i])) {
            kill();
            return 0;
        }
        int hits = destroyHitProjectiles(asteroids[i]);
        if (hits > 0) asteroidsToDestroy.push_back(i);
    }
    for (auto id: asteroidsToDestroy) destroyAsteroid(id);
    return asteroidsToDestroy.size();
}

void onUpdate() {
    int time = glutGet(GLUT_ELAPSED_TIME);
    int delta = time - lastUpdateTime;
    updatePlayer(delta);
    updateProjectiles(delta);
    updateAsteroids(delta);
    updateDusts(delta);
    score += annihilateAsteroids();
    lastUpdateTime = time;
    glutPostWindowRedisplay(windowNumber);
}

void drawGrid() {
    glColor4f(0.0, 0.0, 0.8, 0.5);
    float step = ORTHO_MAX / 10.0f;
    glBegin(GL_LINES);
    for (float x = 0; x <= ORTHO_MAX; x += step) {
        glVertex2f(x, 0);
        glVertex2f(x, ORTHO_MAX);
    }
    for (float y = 0; y <= ORTHO_MAX; y += step) {
        glVertex2f(0, y);
        glVertex2f(ORTHO_MAX, y);
    }
    glEnd();
}
#include <iostream>
extern std::vector<Ripple> ripples;
void onDisplay(){
	glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    drawGrid();
    drawPlayer();
    drawProjectiles();
    drawAsteroids();
    drawDusts();
    
    swapFrameBuffer();
    
    glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(glowShaderProgram);
    glBindTexture(GL_TEXTURE_2D, lastRenderedTextureBuffer);
    glBegin(GL_QUADS);
	    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
	    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
	    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    for (const auto& ripple: ripples) {
    	swapFrameBuffer();
    	
	    glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glUseProgram(rippleShaderProgram);
	    glBindTexture(GL_TEXTURE_2D, lastRenderedTextureBuffer);
	    std::cout << ripples[0].at.x << " " << ripples[0].at.y << std::endl;
		glUniform2f(glGetUniformLocation(rippleShaderProgram, "center"), ripple.at.x / ORTHO_MAX, ripple.at.y / ORTHO_MAX);
		glUniform1f(glGetUniformLocation(rippleShaderProgram, "progress"), ripple.progress);
	    glBegin(GL_QUADS);
		    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
		    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
		    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
	    glEnd();	
	}
    
    glUseProgram(0);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, activeFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint width, height;
	glBindTexture(GL_TEXTURE_2D, activeTextureBuffer);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    displayScore();
    glutSwapBuffers();
}

void initFramebuffer(GLuint& frameBuffer, GLuint& textureBuffer) {
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glGenTextures(1, &textureBuffer);
    glBindTexture(GL_TEXTURE_2D, textureBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowSize, windowSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    activeFrameBuffer = frameBuffer;
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowSize, windowSize);
    windowNumber = glutCreateWindow("Asteroids Example for C++ Beginners");
	// 2D
	//glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
	// 3D
    glMatrixMode(GL_PROJECTION);
	gluPerspective(60.0, 1.0, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	gluLookAt(ORTHO_MAX / 2, 0, ORTHO_MAX, 
	          ORTHO_MAX / 2, ORTHO_MAX / 2, 0, 
	          0, 1, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glewInit();
    initFramebuffer(frameBuffer1, textureBuffer1);
    initFramebuffer(frameBuffer2, textureBuffer2);
    setupShaders();
    
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(&onKeyPressed);
    glutKeyboardUpFunc(&onKeyUp);
    glutSpecialFunc(&onSpecialKeyPressed);
    glutSpecialUpFunc(&onSpecialKeyUp);
    glutDisplayFunc(&onDisplay);
    glutIdleFunc(&onUpdate);
    
    initPlayer();
    glutMainLoop();
    return 0;
}

