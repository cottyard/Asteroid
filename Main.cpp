#include <string>
#define GLUT_DISABLE_ATEXIT_HACK
#include "glut.h"
#include "Common.h"

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

void onVisibilityChange(int vis){
    if (vis == GLUT_VISIBLE) glutIdleFunc(&onUpdate);
    else glutIdleFunc(nullptr);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 800);
    windowNumber = glutCreateWindow("Asteroids Example for C++ Beginners");
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(&onKeyPressed);
    glutKeyboardUpFunc(&onKeyUp);
    glutSpecialFunc(&onSpecialKeyPressed);
    glutSpecialUpFunc(&onSpecialKeyUp);
    glutDisplayFunc(&onDisplay);
    glutVisibilityFunc(&onVisibilityChange);
    glOrtho(0, ORTHO_MAX, 0, ORTHO_MAX, -1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initPlayer();
    glutMainLoop();
    return 0;
}

double wrap_axis(double x, double lower_bound, double upper_bound) {
    if (x < lower_bound) {
        return x + upper_bound - lower_bound;
    }
    else if (x > upper_bound) {
        return x - upper_bound + lower_bound;
    }
    return x;
}

Motion step(Motion last, double delta, double screenWrap) {
    double upper_bound = ORTHO_MAX * screenWrap;
    double lower_bound = -(screenWrap - 1) * ORTHO_MAX;
    Motion next = last;
    next.x = wrap_axis(last.x + last.xv * delta, lower_bound, upper_bound);
    next.y = wrap_axis(last.y + last.yv * delta, lower_bound, upper_bound);
    next.angle += last.angularVelocity * delta;
    return next;
}
