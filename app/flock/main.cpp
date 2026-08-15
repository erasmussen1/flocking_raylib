
#include <raylib.h>

#include <mather/mather.hpp>
#include <mather/vector2d.hpp>
#include <mather/vector2d_util.hpp>


#include <vector>
#include <string>
// #include <memory>
// #include <algorithm>
#include <filesystem>
#include <unistd.h>
#include <stdexcept>
#include <numbers>
#include <print>
#include <map>
#include <cmath>
#include <random>
#include <utility>

namespace fs = std::filesystem;

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


struct AppFonts {
    AppFonts() = default;

    ~AppFonts() {
        UnloadFont(fonts);
    }

    AppFonts(const AppFonts& other) = delete;
    AppFonts& operator=(const AppFonts& other) = delete;

    auto initFonts(const fs::path& directorName) -> int {

        int glyphs[20];
        for (int i = 0; i < 20; ++i) {
            glyphs[i] = 0x1D2E0 + i;
        }

        auto filename = directorName / "resources/BabelStoneMayanNumerals.ttf";

        if (!fs::exists(filename)) {
            return -1;
        }

        fonts = LoadFontEx(filename.string().c_str(), 64, glyphs, 20);
        SetTextureFilter(fonts.texture, TEXTURE_FILTER_BILINEAR);

        return 0;
    }

    Font fonts;
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

auto rr2vv(const mathER::Vector2d& v) -> Vector2 {
    return {(float)v.x, (float)v.y};
}

auto vv2rr(const Vector2& v) -> mathER::Vector2d {
    return {(float)v.x, (float)v.y};
}

auto limit(mathER::Vector2d& v, const float maximum) -> void {
    const double magnitude = mathER::pythag(v.x, v.y);

    if (maximum > 0.0f && magnitude > maximum) {
        v = v * (maximum / magnitude);
    }
}

auto birdDist(const mathER::Vector2d& p1, const mathER::Vector2d& p2) -> float {
    auto diff = p1 - p2;

    return mathER::pythag(diff.x, diff.y);
}

auto target(Bird& bird, mathER::Vector2d& targetDirection, const float k) {

    auto x = mathER::normEx(targetDirection);
    if (!x.has_value()) {
        return;
    }

    auto y = x.value();
    y -= bird.vel;

    y = y * maxSpeedK;

    limit(y, maxForceK);

    y = y * k;

    bird.acc += y;
}

auto steer(Bird& bird, const std::vector<Bird>& flock) -> void {

    mathER::Vector2d sumSeparate;
    mathER::Vector2d sumAlign;
    mathER::Vector2d sumCohere;

    int countN = 0;
    int countS = 0;

    for (auto& other : flock) {
        float d = birdDist(bird.pos, other.pos);

        if (d <= 0.0) {
            continue;
        }

        if (d < neighborDist) {
            sumAlign += other.vel;
            sumCohere += other.pos;
            countN++;
        }

        if (d < SeparateDist) {
            mathER::Vector2d diff = bird.pos - other.pos;

            auto norm = mathER::normEx(diff);
            if (!norm.has_value()) {
                throw;
            }

            sumSeparate += (norm.value() * (1.0 / d));
            countS++;
        }
    }

    if (countS > 0) {
        target(bird, sumSeparate, separateK);
    }

    if (countN > 0) {
        target(bird, sumAlign, alignK);
        sumCohere = sumCohere * (1.0 / countN);
        sumCohere -= bird.pos;
        target(bird, sumCohere, cohereK);
    }
}

void enemy(Bird& bird) {
    mathER::Vector2d mouse = vv2rr(GetMousePosition());
    mathER::Vector2d diff = mouse - bird.pos;

    double d = birdDist(mouse, bird.pos);

    if (d > 100) {
        target(bird, diff, 1.0);
    }

    if (d < 100) {
        target(bird, diff, -1.0);
    }
}

struct Config {
    int w{0};
    int h{0};
};

void wall(const Config &config, Bird& bird, const int wallNumber) {

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
            wall.x =bird.pos.x;
            wall.y = config.h;
            break;
    }

    mathER::Vector2d diff = wall - bird.pos;

    double d = birdDist(wall, bird.pos);

    if (d <= 80) {
        target(bird, diff, -1.0);
    }

    if (d <= 40) {
        target(bird, diff, -5.0);
    }

    if (d <= 20) {
        target(bird, diff, -17.0);
    }


}

auto guide(const Config& config, Bird& bird, const std::vector<Bird>& flock) {

    bird.acc.x = 0.0;
    bird.acc.y = 0.0;

    steer(bird, flock);
    // enemy(bird);
    for (int i = 0; i < 4; i++) {
        wall(config, bird, i);
    }
}

auto update(const Config &config, Bird& bird) {
    bird.vel += bird.acc;

    limit(bird.vel, maxSpeedK);

    bird.pos += bird.vel;

    bird.pos.x = std::fmod(bird.pos.x + config.w, config.w);
    bird.pos.y = std::fmod(bird.pos.y + config.h, config.h);
}

void DrawTallTriangle(
    const float x, const float y, float width, float height, float angleRad, Color color) {
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);

    Vector2 p1 = {0.0f, -height / 2.0f};
    Vector2 p2 = {-width / 2.0f, height / 2.0f};
    Vector2 p3 = {width / 2.0f, height / 2.0f};

    auto rotate = [&](Vector2 pp) -> Vector2 {
        return {x + pp.x * c - pp.y * s, y + pp.x * s + pp.y * c};
    };

    p1 = rotate(p1);
    p2 = rotate(p2);
    p3 = rotate(p3);

    DrawTriangle(p1, p2, p3, color);
}

auto drawBird(Bird& bird) {

    auto vr = mathER::angle(bird.vel);
    if (!vr.has_value()) {
        throw;
    }

    float v = vr.value() + mathER::deg2rad(90.0);

    if (bird.range) {
        float r0 = neighborDist;
        float r1 = SeparateDist;

        DrawRing(rr2vv(bird.pos), r1, r0 + 1.0f, 0.0f, 360.0f, 100, DARKGRAY);
        DrawRing(rr2vv(bird.pos), r0, r0 + 1.0f, 0.0f, 360.0f, 100, ORANGE);
        DrawRing(rr2vv(bird.pos), r1, r1 + 1.0f, 0.0f, 360.0f, 100, ORANGE);
    }

    DrawTallTriangle(bird.pos.x, bird.pos.y, 4, 10, v, bird.color);
}

int main(void) {
    AppWindow appWindow(initWinW, initWinH);

    std::vector<Bird> flock;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> distribX(0, initWinW);
    std::uniform_int_distribution<int> distribY(0, initWinH);
    std::uniform_int_distribution<int> distribI(-100, 100);

    for (int i = 0; i < numberOfBirds; i++) {
        float rw = (float)distribX(gen);
        float rh = (float)distribY(gen);

        float rx = (float)distribI(gen) / 10000.0;
        float ry = (float)distribI(gen) / 10000.0;

        flock.push_back(Bird(rw, rh, rx, ry));
    }

    if (flock.size() > 2)
    {
        flock.front().color = RED;
        flock.front().range = true;

        flock.back().color = BLUE;
    }

    Config config;

    while (appWindow.isOpen()) {

        config.w = static_cast<float>(GetScreenWidth());
        config.h = static_cast<float>(GetScreenHeight());

        for (auto& b : flock) {
            guide(config, b, flock);
        }
        for (auto& b : flock) {
            update(config, b);
        }


        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleLines(80, 80, config.w-(2*80), config.h-(2*80), BLUE);

        for (auto& b : flock) {
            drawBird(b);
        }

        EndDrawing();
    }

    return 0;
}
