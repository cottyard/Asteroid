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
GLdouble modelview[16];
GLdouble projection[16];
GLint viewport[4];

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
    float step = 9.9f;
    glBegin(GL_LINES);
    for (float x = 0.5; x < ORTHO_MAX; x += step) {
        glVertex2f(x, 0.5);
        glVertex2f(x, ORTHO_MAX-0.5);
    }
    for (float y = 0.5; y <= ORTHO_MAX; y += step) {
        glVertex2f(0.5, y);
        glVertex2f(ORTHO_MAX-0.5, y);
    }
    glEnd();
}

extern std::vector<Ripple> ripples;

void view_2d() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void view_3d() {
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(60.0, 1.0, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(ORTHO_MAX / 2, 0, ORTHO_MAX, 
	          ORTHO_MAX / 2, ORTHO_MAX / 2, 0, 
	          0, 1, 0);
}

void onDisplay(){
	view_2d();
	
	glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawGrid();
    drawPlayer();
    drawProjectiles();
    drawAsteroids();
    drawDusts();
    
    swapFrameBuffer();
    
    glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(glowShaderProgram);
    glBindTexture(GL_TEXTURE_2D, lastRenderedTextureBuffer);
    glGenerateMipmap(GL_TEXTURE_2D);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
	    glVertex2f(-1.0f, -1.0f);
		glVertex2f(1.0f, -1.0f);
		glVertex2f(1.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble winX, winY, winZ;
    for (const auto& ripple: ripples) {
    	swapFrameBuffer();
    	
	    glBindFramebuffer(GL_FRAMEBUFFER, activeFrameBuffer);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    glUseProgram(rippleShaderProgram);
	    glBindTexture(GL_TEXTURE_2D, lastRenderedTextureBuffer);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    
	    gluProject(ripple.at.x, ripple.at.y, 0, modelview, projection, viewport, &winX, &winY, &winZ);
		glUniform2f(glGetUniformLocation(rippleShaderProgram, "center"), winX / windowSize, winY / windowSize);
		glUniform1f(glGetUniformLocation(rippleShaderProgram, "progress"), ripple.progress);
	    glBegin(GL_QUADS);
			glVertex2f(-1.0f, -1.0f);
			glVertex2f(1.0f, -1.0f);
			glVertex2f(1.0f, 1.0f);
			glVertex2f(-1.0f, 1.0f);
	    glEnd();
	}
    
    glUseProgram(0);
    
	view_3d();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, activeTextureBuffer);
	glGenerateMipmap(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(0, 0, 0);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(ORTHO_MAX, 0, 0);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(ORTHO_MAX, ORTHO_MAX, 0);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(0, ORTHO_MAX, 0);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    
    view_2d();
    displayScore();
    
    glutSwapBuffers();
}

void initFramebuffer(GLuint& frameBuffer, GLuint& textureBuffer) {
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glGenTextures(1, &textureBuffer);
    glBindTexture(GL_TEXTURE_2D, textureBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowSize, windowSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
	
	glEnable(GL_TEXTURE_2D);
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

