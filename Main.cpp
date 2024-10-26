#include <string>
#include "Common.h"
#define GLUT_DISABLE_ATEXIT_HACK
#include "GL/freeglut.h"

void initShaders();
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

extern std::vector<Asteroid> asteroids;

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
    glClear(GL_COLOR_BUFFER_BIT);
    drawPlayer();
    drawBullets();
    drawAsteroids();
    drawDusts();
    displayScore();
    glutSwapBuffers();
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 800);
    windowNumber = glutCreateWindow("Asteroids Example for C++ Beginners");
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(&onKeyPressed);
    glutKeyboardUpFunc(&onKeyUp);
    glutSpecialFunc(&onSpecialKeyPressed);
    glutSpecialUpFunc(&onSpecialKeyUp);
    glutDisplayFunc(&onDisplay);
    glutIdleFunc(&onUpdate);
    glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initShaders();
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
