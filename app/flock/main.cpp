
#include <raylib.h>

#include <mather/mather.hpp>
#include <mather/vector2d.hpp>
#include <mather/vector2d_util.hpp>
#include <mather/point2d.hpp>
#include <mather/point2d_util.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <numbers>
#include <print>
#include <map>
#include <cmath>
#include <random>
#include <utility>
#include <tuple>
#include <execution>

constexpr double initWinW = 1000;
constexpr double initWinH = 800;

constexpr float maxSpeedK = 2.0;
constexpr float maxForceK = 0.03;
constexpr float neighborDist = 65.0;
constexpr float SeparateDist = 24.0;  // has to be smaller than neighborDist
constexpr float separateK = 1.5;
constexpr float alignK = 1.0;
constexpr float cohereK = 0.8;

constexpr int numberOfBirds = 250;
constexpr int fps = 60;


struct AppWindow {
    AppWindow(int winWidth = 1280, int winHeight = 720) {
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

        InitWindow(winWidth, winHeight, "Flocking");

        SetWindowMinSize(400, 600);
        SetTargetFPS(fps);
    }

    ~AppWindow() {
        CloseWindow();
    }

    AppWindow(const AppWindow& other) = delete;
    AppWindow& operator=(const AppWindow& other) = delete;

    auto isOpen() -> bool {
        return !WindowShouldClose();
    }
};

struct Bird {
    Bird(float x_, float y_, float velX_, float velY_)  //
        : pos(x_, y_), vel(velX_, velY_) {
    }

    mathER::Vector2d pos;
    mathER::Vector2d vel;
    mathER::Vector2d acc;

    Color color{GREEN};
    bool range{false};
};

auto rr2vv(const mathER::Point2d& v) -> Vector2 {
    return {(float)v.x, (float)v.y};
}

auto rr2vv(const mathER::Vector2d& v) -> Vector2 {
    return {(float)v.x, (float)v.y};
}

auto vv2rr(const Vector2& v) -> mathER::Vector2d {
    return {(float)v.x, (float)v.y};
}

auto limit(mathER::Vector2d& v, const float maximum) -> void {
    const double magnitude = mathER::length(v);

    if (maximum > 0.0f && magnitude > maximum) {
        v *= (maximum / magnitude);
    }
}

auto target(Bird& bird, const mathER::Vector2d& targetDirection, const float k) {
    auto x = mathER::normEx(targetDirection);
    if (!x.has_value()) {
        return;
    }

    auto y = x.value() - bird.vel;
    y *= maxSpeedK;

    limit(y, maxForceK);

    bird.acc += (y * k);
}

template <typename T>
auto inv(const T t) -> T {
    return T{1} / t;
}

auto steer(Bird& bird, const std::vector<Bird>& flock) -> void {

    mathER::Vector2d sumSeparate;
    mathER::Vector2d sumAlign;
    mathER::Vector2d sumCohere;

    int countN = 0;
    int countS = 0;

    for (auto& other : flock) {
        if (bird.pos == other.pos) {
            continue;
        }

        mathER::Vector2d diff = bird.pos - other.pos;
        float d = mathER::length(diff);

        if (d < neighborDist) {
            sumAlign += other.vel;
            sumCohere += other.pos;
            countN++;
        }

        if (d < SeparateDist) {
            auto norm = mathER::normEx(diff);
            if (!norm.has_value()) {
                throw;
            }

            sumSeparate += (norm.value() * inv(d));
            countS++;
        }
    }

    if (countS > 0) {
        target(bird, sumSeparate, separateK);
    }

    if (countN > 0) {
        target(bird, sumAlign, alignK);
        sumCohere *= inv(static_cast<float>(countN));
        sumCohere -= bird.pos;
        target(bird, sumCohere, cohereK);
    }
}

void enemy(Bird& bird) {
    mathER::Vector2d mouse = vv2rr(GetMousePosition());
    mathER::Vector2d diff = mouse - bird.pos;

    const double d = mathER::length(diff);

    target(bird, diff, (d > 100.0) ? 1.0 : -1.0);
}

struct Config {
    int w{0};
    int h{0};
};

void walls(const Config& config, Bird& bird, const int wallNumber) {

    mathER::Vector2d wall(0.0, bird.pos.y);

    switch (wallNumber) {
        case 0:
            wall.x = 0.0;
            wall.y = bird.pos.y;
            break;
        case 1:
            wall.x = config.w;
            wall.y = bird.pos.y;
            break;
        case 2:
            wall.x = bird.pos.x;
            wall.y = 0.0;
            break;
        case 3:
            wall.x = bird.pos.x;
            wall.y = config.h;
            break;
    }

    const mathER::Vector2d diff = wall - bird.pos;

    const double d = mathER::length(wall - bird.pos);

    if (d <= 20.0) {
        target(bird, diff, -17.0);
    }

    if (d <= 40.0) {
        target(bird, diff, -5.0);
    }

    if (d <= 80.0) {
        target(bird, diff, -1.0);
    }
}

auto guide(const Config& config, const Bird& birdIn, const std::vector<Bird>& flock) {
    Bird bird{birdIn};

    bird.acc.x = 0.0;
    bird.acc.y = 0.0;

    steer(bird, flock);

    // enemy(bird);

    for (int i = 0; i < 4; i++) {
        walls(config, bird, i);
    }

    return bird;
}

auto update(const Config& config, const Bird& birdIn) -> Bird {
    Bird bird(birdIn);

    bird.vel += bird.acc;

    limit(bird.vel, maxSpeedK);

    bird.pos += bird.vel;

    bird.pos.x = std::fmod(bird.pos.x + config.w, config.w);
    bird.pos.y = std::fmod(bird.pos.y + config.h, config.h);

    return bird;
}

auto calculateTallTriangle(const mathER::Vector2d& cv,
                           const float width,
                           const float height,
                           const float angleRad)
    -> std::tuple<mathER::Point2d, mathER::Point2d, mathER::Point2d> {

    const mathER::Point2d c(cv.x, cv.y);

    const mathER::Point2d p1(0.0f, -height / 2.0f);
    const mathER::Point2d p2(-width / 2.0f, height / 2.0f);
    const mathER::Point2d p3(width / 2.0f, height / 2.0f);

    auto t1 = mathER::rotate(p1, angleRad);
    if (!t1.has_value()) {
        throw;
    }

    auto t2 = mathER::rotate(p2, angleRad);
    if (!t2.has_value()) {
        throw;
    }

    auto t3 = mathER::rotate(p3, angleRad);
    if (!t3.has_value()) {
        throw;
    }

    return {t1.value() + c, t2.value() + c, t3.value() + c};
}

auto drawBird(const Bird& bird) {

    auto vr = mathER::angle(bird.vel);
    if (!vr.has_value()) {
        throw;
    }

    const float v = vr.value() + mathER::deg2rad(90.0);

    if (bird.range) {
        float r0 = neighborDist;
        float r1 = SeparateDist;

        DrawRing(rr2vv(bird.pos), r1, r0 + 1.0f, 0.0f, 360.0f, 100, DARKGRAY);
        DrawRing(rr2vv(bird.pos), r0, r0 + 1.0f, 0.0f, 360.0f, 100, ORANGE);
        DrawRing(rr2vv(bird.pos), r1, r1 + 1.0f, 0.0f, 360.0f, 100, ORANGE);
    }

    auto [p1, p2, p3] = calculateTallTriangle(bird.pos, 4, 10, v);

    DrawTriangle(rr2vv(p1), rr2vv(p2), rr2vv(p3), bird.color);
}

auto generateRandomBirds(const int numberOfBrids) -> std::vector<Bird> {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> distribX(0, initWinW);
    std::uniform_int_distribution<int> distribY(0, initWinH);
    std::uniform_int_distribution<int> distribI(-100, 100);

    std::vector<Bird> flock;

    for (int i = 0; i < numberOfBirds; i++) {
        float rw = (float)distribX(gen);
        float rh = (float)distribY(gen);

        float rx = (float)distribI(gen) / 10000.0;
        float ry = (float)distribI(gen) / 10000.0;

        flock.push_back(Bird(rw, rh, rx, ry));
    }

    return flock;
}

int main(void) {
    AppWindow appWindow(initWinW, initWinH);

    std::vector<Bird> flock = generateRandomBirds(numberOfBirds);

    if (flock.size() > 2) {
        flock.front().color = RED;
        flock.front().range = true;

        flock.back().color = BLUE;
    }

    Config config;

    while (appWindow.isOpen()) {

        config.w = static_cast<float>(GetScreenWidth());
        config.h = static_cast<float>(GetScreenHeight());

        std::transform(std::execution::par,                 //
                       flock.cbegin(),                      //
                       flock.cend(),                        //
                       flock.begin(),                       //
                       [config, flock](const Bird& b) {     //
                           return guide(config, b, flock);  //
                       });

        std::transform(std::execution::par,           //
                       flock.cbegin(),                //
                       flock.cend(),                  //
                       flock.begin(),                 //
                       [config](const Bird& b) {      //
                           return update(config, b);  //
                       });

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleLines(80, 80, config.w - (2 * 80), config.h - (2 * 80), BLUE);

        for (auto& b : flock) {
            drawBird(b);
        }

        EndDrawing();
    }

    return 0;
}
