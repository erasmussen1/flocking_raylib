
#include <raylib.h>
// #include <raymath.h>

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
constexpr float neighborDist = 90.0;
constexpr float SeparateDist = 30.0;  // has to be smaller than neighborDist
constexpr float separateK = 1.8;
constexpr float alignK = 1.0;
constexpr float cohereK = 1.0;

constexpr int numberOfBirds = 70;
constexpr int fps = 60;

auto getGlyph(const int id) -> std::string {
    static std::map<int, std::string> kMayaGlyphs = {
        {0, "\U0001D2E0"},  // Mayan numeral zero (shell)
        {1, "\U0001D2E1"},  {2, "\U0001D2E2"},  {3, "\U0001D2E3"},  {4, "\U0001D2E4"},
        {5, "\U0001D2E5"},  {6, "\U0001D2E6"},  {7, "\U0001D2E7"},  {8, "\U0001D2E8"},
        {9, "\U0001D2E9"},  {10, "\U0001D2EA"}, {11, "\U0001D2EB"}, {12, "\U0001D2EC"},
        {13, "\U0001D2ED"}, {14, "\U0001D2EE"}, {15, "\U0001D2EF"}, {16, "\U0001D2F0"},
        {17, "\U0001D2F1"}, {18, "\U0001D2F2"}, {19, "\U0001D2F3"}};

    auto it = kMayaGlyphs.find(id % 20);
    if (it != kMayaGlyphs.end()) {
        return it->second;
    }

    return {"\U0001D2E0"};
}

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

auto executableDirectory() -> std::filesystem::path {
    std::vector<char> buffer(4096);

    ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (len <= -1) {
        throw std::runtime_error("ERR: readlink failed");
    }

    return fs::path(std::string(buffer.data(), (size_t)len)).parent_path();
}

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

template <typename T>
auto limit(T& v, float mx) {
    if (v.x > mx) {
        v.x = mx;
    }

    if (v.y > mx) {
        v.y = mx;
    }
}

auto limit2(mathER::Vector2d& vel, float mx) {

    double mag = mathER::pythag(vel.x, vel.y);

    if (mx > 0 && mag > mx) {
        vel = vel * (1.0 / mag);
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
    y = y - bird.vel ;

    y = y * maxSpeedK;

    limit(y, maxForceK);

    y = y * k;

    bird.acc = bird.acc + y;
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
            sumAlign = sumAlign + other.vel;
            sumCohere = sumCohere + other.pos;
            countN++;
        }

        if (d < SeparateDist) {
            mathER::Vector2d diff = bird.pos - other.pos;

            auto norm = mathER::normEx(diff);
            if (!norm.has_value()) {
                throw;
            }

            sumSeparate = sumSeparate + (norm.value() * (1.0 / d));
            countS++;
        }
    }

    if (countS > 0) {
        target(bird, sumSeparate, separateK);
    }

    if (countN > 0) {
        target(bird, sumAlign, alignK);
        sumCohere = sumCohere * (1.0 / countN);
        sumCohere = sumCohere - bird.pos;
        target(bird, sumCohere, cohereK);
    }
}

void enemy(Bird &bird) {

    mathER::Vector2d mouse=vv2rr(GetMousePosition());

    mathER::Vector2d diff = mouse - bird.pos;

    target(bird, diff, 1.0);
}

auto guide(Bird& bird, const std::vector<Bird>& flock) {

    bird.acc.x = 0.0;
    bird.acc.y = 0.0;

    steer(bird, flock);
    enemy(bird);
}

auto update(const float w, const float h, Bird& bird) {
    bird.vel = bird.vel + bird.acc;

    limit(bird.vel, maxSpeedK);

    bird.pos = bird.pos + bird.vel;

    bird.pos.x = std::fmod(bird.pos.x + w, w);
    bird.pos.y = std::fmod(bird.pos.y + h, h);
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

    DrawTallTriangle(bird.pos.x, bird.pos.y, 7, 14, v, bird.color);
}

int main(void) {
    AppWindow appWindow(initWinW, initWinH);

    // AppFonts font16;

    std::vector<Bird> flock;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> distribX(0, initWinW);
    std::uniform_int_distribution<int> distribY(0, initWinH);
    std::uniform_int_distribution<int> distribI(-100, 100);

    for (int i = 0; i < numberOfBirds; i++) {
        float rw = (float)distribX(gen);
        float rh = (float)distribY(gen);

        float rx = (float)distribI(gen)/10000.0;
        float ry = (float)distribI(gen)/10000.0;

        flock.push_back(Bird(rw, rh, rx, ry));
    }

    {
        flock[0].color = RED;
        flock[0].range = true;
    }

    std::println("============================");
    int noAngle = 0;
    int p1 = 0;
    int p2 = 0;
    int p3 = 0;
    int p4 = 0;
    for (auto& x : flock) {
        auto v = mathER::angle(x.vel);
        if (!v.has_value()) {
            noAngle++;
            continue;
        }

       const  double d = mathER::rad2deg(v.value());

        if (d > 0.0 && d <= 90.0) { p1++; }
        if (d > 90.0 && d <= 180.0) { p2++; }
        if (d > 180.0 && d <= 270.0) { p3++; }
        if (d > 270.0 && d <= 360.0) { p4++; }
    }
    std::println("No angle {}", noAngle);
    std::println("{} {} {} {}", p1, p2, p3, p4);
    std::println("============================");


#if 0
    auto exeDir = executableDirectory();
    int rc = font16.initFonts(exeDir);
    if (rc != 0) {
        std::println("ERR: ===> Font file could not be found! <===");
        return -1;
    }
#endif

    int w = 0;
    int h = 0;

    while (appWindow.isOpen()) {

        w = GetScreenWidth();
        h = GetScreenHeight();

        /*
        Vector2 mouse = GetMousePosition();

        flock[0].pos.x = mouse.x;
        flock[0].pos.y = mouse.y;
        */

        BeginDrawing();
        ClearBackground(BLACK);

        for (auto& b : flock) {
            guide(b, flock);
            update(w, h, b);
            drawBird(b);
        }

        EndDrawing();
    }

    return 0;
}
