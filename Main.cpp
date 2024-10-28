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
void drawBullets();
void drawAsteroids();
void drawDusts();
void updatePlayer(int);
void updateBullets(int);
void updateAsteroids(int);
void updateDusts(int);
void initPlayer();
void onKeyPressed(unsigned char, int, int);
void onKeyUp(unsigned char, int, int);
void onSpecialKeyPressed(int, int, int);
void onSpecialKeyUp(int, int, int);
bool testPlayerCollision(Asteroid);
std::vector<size_t> testBulletsHit(Asteroid);
void destroyBullet(size_t);
void destroyAsteroid(size_t);
void setupShaders();

extern std::vector<Asteroid> asteroids;
extern GLuint glowShader;
GLuint framebuffer, textureColorbuffer;
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

void onUpdate() {
    int time = glutGet(GLUT_ELAPSED_TIME);
    int delta = time - lastUpdateTime;
    updatePlayer(delta);
    updateBullets(delta);
    updateAsteroids(delta);
    updateDusts(delta);
    std::vector<size_t> asteroidsToDestroy;
    for (size_t i = 0; i < asteroids.size(); i++) {
        if (testPlayerCollision(asteroids[i])) {
            kill();
            break;
        }
        std::vector<size_t> hitBullets = testBulletsHit(asteroids[i]);
        for (auto bulletId: hitBullets) destroyBullet(bulletId);
        if (hitBullets.size() > 0) {
            asteroidsToDestroy.push_back(i);
        }
    }
    for (auto id: asteroidsToDestroy) destroyAsteroid(id);
    score += asteroidsToDestroy.size();
    lastUpdateTime = time;
    glutPostWindowRedisplay(windowNumber);
}

void onDisplay(){
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawPlayer();
    drawBullets();
    drawAsteroids();
    drawDusts();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(glowShader);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glBegin(GL_QUADS);
	    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
	    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
	    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();
    glUseProgram(0);
    displayScore();
    glutSwapBuffers();
}

void initFramebuffer() {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowSize, windowSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowSize, windowSize);
    windowNumber = glutCreateWindow("Asteroids Example for C++ Beginners");
    glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glewInit();
    initFramebuffer();
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

double wrapAxis(double x, double lowerBound, double upperBound) {
    if (x < lowerBound) {
        return x + upperBound - lowerBound;
    }
    else if (x > upperBound) {
        return x - upperBound + lowerBound;
    }
    return x;
}

Motion step(Motion last, double delta, double screenWrap) {
    double upperBound = ORTHO_MAX * screenWrap;
    double lowerBound = -(screenWrap - 1) * ORTHO_MAX;
    Motion next = last;
    next.x = wrapAxis(last.x + last.xv * delta, lowerBound, upperBound);
    next.y = wrapAxis(last.y + last.yv * delta, lowerBound, upperBound);
    next.angle += last.angularVelocity * delta;
    return next;
}
