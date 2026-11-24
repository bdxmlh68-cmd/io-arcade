#include <eadk.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

extern "C" {
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "IO Arcade";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
uint16_t eadk_app_icon[55 * 56] __attribute__((section(".rodata.eadk_app_icon")));
}

static constexpr int W = 320;
static constexpr int H = 240;
static constexpr int BAND_H = 20;

static constexpr eadk_color_t C_BG_DARK    = 0x10A3;
static constexpr eadk_color_t C_GRID       = 0x2125;
static constexpr eadk_color_t C_PANEL      = 0x18E4;
static constexpr eadk_color_t C_BORDER     = 0x4A69;
static constexpr eadk_color_t C_WHITE      = 0xFFFF;
static constexpr eadk_color_t C_BLACK      = 0x0000;
static constexpr eadk_color_t C_CYAN       = 0x07FF;
static constexpr eadk_color_t C_BLUE       = 0x24FF;
static constexpr eadk_color_t C_GREEN      = 0x2FE8;
static constexpr eadk_color_t C_YELLOW     = 0xFFE0;
static constexpr eadk_color_t C_ORANGE     = 0xFD20;
static constexpr eadk_color_t C_RED        = 0xF8A2;
static constexpr eadk_color_t C_PURPLE     = 0xB19F;
static constexpr eadk_color_t C_PINK       = 0xF996;
static constexpr eadk_color_t C_ASPHALT    = 0x2966;
static constexpr eadk_color_t C_ROAD_LINE  = 0x6B6D;

static const eadk_color_t PALETTE[] = {
    C_CYAN, C_GREEN, C_ORANGE, C_PINK, C_PURPLE, C_YELLOW, C_BLUE, C_RED
};

static uint16_t bandBuf[W * BAND_H];

static void fillRectBand(int x, int y, int w, int h, int by0, eadk_color_t col) {
    int x0 = x < 0 ? 0 : x;
    int x1 = (x + w) > W ? W : (x + w);
    int y0 = y < by0 ? by0 : y;
    int y1 = (y + h) > (by0 + BAND_H) ? (by0 + BAND_H) : (y + h);
    if (x0 >= x1 || y0 >= y1) return;
    for (int py = y0; py < y1; py++) {
        uint16_t* row = &bandBuf[(py - by0) * W + x0];
        for (int px = x0; px < x1; px++) {
            *row++ = col;
        }
    }
}

static void fillCircleBand(int cx, int cy, int r, int by0, eadk_color_t col) {
    if (r <= 0) return;
    int y0 = (cy - r) < by0 ? by0 : (cy - r);
    int y1 = (cy + r) >= (by0 + BAND_H) ? (by0 + BAND_H - 1) : (cy + r);
    int r2 = r * r;
    for (int py = y0; py <= y1; py++) {
        int dy = py - cy;
        int dxMax = (int)sqrtf((float)(r2 - dy * dy));
        int x0 = (cx - dxMax) < 0 ? 0 : (cx - dxMax);
        int x1 = (cx + dxMax) >= W ? (W - 1) : (cx + dxMax);
        uint16_t* row = &bandBuf[(py - by0) * W + x0];
        for (int px = x0; px <= x1; px++) {
            *row++ = col;
        }
    }
}

static void drawRingBand(int cx, int cy, int rOuter, int rInner, int by0, eadk_color_t col) {
    if (rOuter <= 0) return;
    int y0 = (cy - rOuter) < by0 ? by0 : (cy - rOuter);
    int y1 = (cy + rOuter) >= (by0 + BAND_H) ? (by0 + BAND_H - 1) : (cy + rOuter);
    int ro2 = rOuter * rOuter;
    int ri2 = rInner > 0 ? rInner * rInner : 0;
    for (int py = y0; py <= y1; py++) {
        int dy = py - cy;
        int dy2 = dy * dy;
        for (int px = cx - rOuter; px <= cx + rOuter; px++) {
            if (px < 0 || px >= W) continue;
            int dx = px - cx;
            int d2 = dx * dx + dy2;
            if (d2 <= ro2 && d2 >= ri2) {
                bandBuf[(py - by0) * W + px] = col;
            }
        }
    }
}

static void drawLineBand(int x0, int y0, int x1, int y1, int thick, int by0, eadk_color_t col) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        fillCircleBand(x0, y0, thick, by0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void flushBand(int by0) {
    eadk_display_push_rect((eadk_rect_t){0, (uint16_t)by0, W, BAND_H}, bandBuf);
}

static uint32_t rngState = 0x12345678u;
static inline uint32_t fastRand() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}
static inline float randRange(float a, float b) {
    return a + (float)(fastRand() & 0xFFFF) / 65535.0f * (b - a);
}
static inline float dist2(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2, dy = y1 - y2;
    return dx * dx + dy * dy;
}

enum GameMode {
    MODE_MENU = 0,
    MODE_AGAR = 1,
    MODE_DIEP = 2,
    MODE_HOLE = 3,
    MODE_GAMEOVER = 4
};

static GameMode currentMode = MODE_MENU;
static GameMode lastPlayedMode = MODE_AGAR;
static int menuSelection = 0;
static int bestScores[3] = {0, 0, 0};
static int lastScore = 0;
static bool lastVictory = false;

struct Particle {
    float x, y, vx, vy;
    int life;
    eadk_color_t col;
};
static constexpr int MAX_PARTICLES = 36;
static Particle particles[MAX_PARTICLES];

static void spawnParticles(float x, float y, eadk_color_t col, int count) {
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (particles[i].life <= 0) {
            float ang = randRange(0.0f, 6.28318f);
            float spd = randRange(1.2f, 4.5f);
            particles[i] = {x, y, cosf(ang) * spd, sinf(ang) * spd, (int)randRange(10, 22), col};
            count--;
        }
    }
}

static void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vx *= 0.92f;
            particles[i].vy *= 0.92f;
            particles[i].life--;
        }
    }
}

static constexpr float AGAR_WORLD = 1200.0f;
static constexpr int AGAR_PELLETS = 75;
static constexpr int AGAR_BOTS = 10;
static constexpr int AGAR_VIRUS = 5;

struct AgarPellet { float x, y; eadk_color_t col; };
struct AgarBot { float x, y, vx, vy, mass; eadk_color_t col; bool alive; };
struct AgarVirus { float x, y, r; };

static AgarPellet agarPellets[AGAR_PELLETS];
static AgarBot agarBots[AGAR_BOTS];
static AgarVirus agarViruses[AGAR_VIRUS];
static float agarPx, agarPy, agarMass, agarBoostTimer;
static int agarScore;

static inline float massToRadius(float m) { return sqrtf(m) * 2.2f; }

static void initAgar() {
    agarPx = AGAR_WORLD * 0.5f;
    agarPy = AGAR_WORLD * 0.5f;
    agarMass = 36.0f;
    agarBoostTimer = 0.0f;
    agarScore = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < AGAR_PELLETS; i++) {
        agarPellets[i] = {randRange(30, AGAR_WORLD - 30), randRange(30, AGAR_WORLD - 30), PALETTE[i % 8]};
    }
    for (int i = 0; i < AGAR_BOTS; i++) {
        agarBots[i] = {randRange(80, AGAR_WORLD - 80), randRange(80, AGAR_WORLD - 80), 0, 0, randRange(24.0f, 110.0f), PALETTE[(i + 2) % 8], true};
    }
    for (int i = 0; i < AGAR_VIRUS; i++) {
        agarViruses[i] = {randRange(140, AGAR_WORLD - 140), randRange(140, AGAR_WORLD - 140), 22.0f};
    }
}

static void stepAgar(eadk_keyboard_state_t kb) {
    float dx = 0.0f, dy = 0.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_left))  dx -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_right)) dx += 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_up))    dy -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_down))  dy += 1.0f;

    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.01f) { dx /= len; dy /= len; }

    if (eadk_keyboard_key_down(kb, eadk_key_ok) && agarMass > 45.0f && agarBoostTimer <= 0.0f) {
        agarBoostTimer = 12.0f;
        agarMass -= 4.0f;
        spawnParticles(agarPx, agarPy, C_CYAN, 8);
    }
    if (agarBoostTimer > 0.0f) agarBoostTimer -= 1.0f;

    float speed = (3.4f / (1.0f + agarMass * 0.0045f)) * (agarBoostTimer > 0.0f ? 1.85f : 1.0f);
    agarPx += dx * speed;
    agarPy += dy * speed;
    float pr = massToRadius(agarMass);
    if (agarPx < pr) agarPx = pr;
    if (agarPx > AGAR_WORLD - pr) agarPx = AGAR_WORLD - pr;
    if (agarPy < pr) agarPy = pr;
    if (agarPy > AGAR_WORLD - pr) agarPy = AGAR_WORLD - pr;

    for (int i = 0; i < AGAR_PELLETS; i++) {
        if (dist2(agarPx, agarPy, agarPellets[i].x, agarPellets[i].y) < (pr + 4.0f) * (pr + 4.0f)) {
            agarMass += 2.2f;
            agarScore += 10;
            spawnParticles(agarPellets[i].x, agarPellets[i].y, agarPellets[i].col, 3);
            agarPellets[i].x = randRange(30, AGAR_WORLD - 30);
            agarPellets[i].y = randRange(30, AGAR_WORLD - 30);
            pr = massToRadius(agarMass);
        }
    }

    for (int i = 0; i < AGAR_VIRUS; i++) {
        if (agarMass > 140.0f && dist2(agarPx, agarPy, agarViruses[i].x, agarViruses[i].y) < pr * pr) {
            agarMass *= 0.65f;
            spawnParticles(agarPx, agarPy, C_GREEN, 14);
            agarViruses[i].x = randRange(120, AGAR_WORLD - 120);
            agarViruses[i].y = randRange(120, AGAR_WORLD - 120);
        }
    }

    for (int i = 0; i < AGAR_BOTS; i++) {
        AgarBot& b = agarBots[i];
        if (!b.alive) continue;
        float br = massToRadius(b.mass);
        float dPlayer = sqrtf(dist2(b.x, b.y, agarPx, agarPy));
        float tx = b.x + b.vx * 30.0f;
        float ty = b.y + b.vy * 30.0f;

        if (dPlayer < 210.0f) {
            if (b.mass > agarMass * 1.15f) { tx = agarPx; ty = agarPy; }
            else if (agarMass > b.mass * 1.15f) { tx = b.x - (agarPx - b.x); ty = b.y - (agarPy - b.y); }
        } else {
            tx = agarPellets[i * 5].x;
            ty = agarPellets[i * 5].y;
        }

        float bdx = tx - b.x, bdy = ty - b.y;
        float bl = sqrtf(bdx * bdx + bdy * bdy);
        if (bl > 0.1f) {
            float bspd = 2.6f / (1.0f + b.mass * 0.005f);
            b.vx = (bdx / bl) * bspd;
            b.vy = (bdy / bl) * bspd;
        }
        b.x += b.vx; b.y += b.vy;
        if (b.x < br) b.x = br;
        if (b.x > AGAR_WORLD - br) b.x = AGAR_WORLD - br;
        if (b.y < br) b.y = br;
        if (b.y > AGAR_WORLD - br) b.y = AGAR_WORLD - br;

        for (int p = 0; p < AGAR_PELLETS; p += 3) {
            if (dist2(b.x, b.y, agarPellets[p].x, agarPellets[p].y) < (br + 4.0f) * (br + 4.0f)) {
                b.mass += 2.0f;
                agarPellets[p].x = randRange(30, AGAR_WORLD - 30);
                agarPellets[p].y = randRange(30, AGAR_WORLD - 30);
            }
        }

        float d2 = dist2(agarPx, agarPy, b.x, b.y);
        if (agarMass > b.mass * 1.12f && d2 < pr * pr) {
            agarMass += b.mass * 0.75f;
            agarScore += (int)(b.mass * 3.0f);
            spawnParticles(b.x, b.y, b.col, 14);
            b.mass = randRange(25.0f, 80.0f);
            b.x = randRange(80, AGAR_WORLD - 80);
            b.y = randRange(80, AGAR_WORLD - 80);
        } else if (b.mass > agarMass * 1.12f && d2 < br * br) {
            lastScore = agarScore;
            if (lastScore > bestScores[0]) bestScores[0] = lastScore;
            lastPlayedMode = MODE_AGAR;
            lastVictory = false;
            currentMode = MODE_GAMEOVER;
            return;
        }
    }

    updateParticles();
    if (agarScore > bestScores[0]) bestScores[0] = agarScore;
}

static void renderAgar() {
    float zoom = 1.0f / (1.0f + (massToRadius(agarMass) - 13.0f) * 0.015f);
    if (zoom < 0.45f) zoom = 0.45f;
    float camX = agarPx - (W * 0.5f) / zoom;
    float camY = agarPy - (H * 0.5f) / zoom;

    for (int by0 = 0; by0 < H; by0 += BAND_H) {
        fillRectBand(0, by0, W, BAND_H, by0, C_BG_DARK);

        int gridStep = (int)(40.0f * zoom);
        if (gridStep < 12) gridStep = 12;
        int startGX = -(((int)(camX * zoom)) % gridStep);
        for (int gx = startGX; gx < W; gx += gridStep) fillRectBand(gx, by0, 1, BAND_H, by0, C_GRID);
        int worldTop = (int)(-camY * zoom);
        for (int py = by0; py < by0 + BAND_H; py++) {
            if (((py - worldTop) % gridStep) == 0) fillRectBand(0, py, W, 1, by0, C_GRID);
        }

        int bx0 = (int)(-camX * zoom), byW0 = (int)(-camY * zoom);
        int bx1 = (int)((AGAR_WORLD - camX) * zoom), byW1 = (int)((AGAR_WORLD - camY) * zoom);
        fillRectBand(bx0 - 3, byW0, 3, byW1 - byW0, by0, C_RED);
        fillRectBand(bx1, byW0, 3, byW1 - byW0, by0, C_RED);
        fillRectBand(bx0, byW0 - 3, bx1 - bx0, 3, by0, C_RED);
        fillRectBand(bx0, byW1, bx1 - bx0, 3, by0, C_RED);

        int prPel = (int)(4.0f * zoom);
        if (prPel < 2) prPel = 2;
        for (int i = 0; i < AGAR_PELLETS; i++) {
            fillCircleBand((int)((agarPellets[i].x - camX) * zoom), (int)((agarPellets[i].y - camY) * zoom), prPel, by0, agarPellets[i].col);
        }

        for (int i = 0; i < AGAR_VIRUS; i++) {
            int sx = (int)((agarViruses[i].x - camX) * zoom);
            int sy = (int)((agarViruses[i].y - camY) * zoom);
            int sr = (int)(agarViruses[i].r * zoom);
            fillCircleBand(sx, sy, sr, by0, C_GREEN);
            drawRingBand(sx, sy, sr + 2, sr - 1, by0, C_YELLOW);
        }

        for (int i = 0; i < AGAR_BOTS; i++) {
            if (!agarBots[i].alive) continue;
            int sx = (int)((agarBots[i].x - camX) * zoom);
            int sy = (int)((agarBots[i].y - camY) * zoom);
            int sr = (int)(massToRadius(agarBots[i].mass) * zoom);
            fillCircleBand(sx, sy, sr, by0, agarBots[i].col);
            drawRingBand(sx, sy, sr, sr - 2, by0, C_WHITE);
        }

        int psx = (int)((agarPx - camX) * zoom);
        int psy = (int)((agarPy - camY) * zoom);
        int psr = (int)(massToRadius(agarMass) * zoom);
        fillCircleBand(psx, psy, psr, by0, C_CYAN);
        drawRingBand(psx, psy, psr, psr - 2, by0, C_WHITE);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].life > 0) {
                fillCircleBand((int)((particles[i].x - camX) * zoom), (int)((particles[i].y - camY) * zoom), 2, by0, particles[i].col);
            }
        }

        if (by0 >= 180) {
            fillRectBand(266, 186, 48, 48, by0, C_PANEL);
            drawRingBand(266 + (int)(agarPx / AGAR_WORLD * 48), 186 + (int)(agarPy / AGAR_WORLD * 48), 2, 0, by0, C_CYAN);
        }

        flushBand(by0);
    }

    char hud[48];
    snprintf(hud, sizeof(hud), "AGAR  Masse:%d  Score:%d", (int)agarMass, agarScore);
    eadk_display_draw_string(hud, (eadk_point_t){6, 4}, false, C_WHITE, C_BG_DARK);
}

static constexpr float DIEP_WORLD = 1100.0f;
static constexpr int DIEP_SHAPES = 36;
static constexpr int DIEP_BULLETS = 32;
static constexpr int DIEP_ENEMIES = 6;

struct DiepShape { float x, y, hp, maxHp; int kind; };
struct DiepBullet { float x, y, vx, vy; int life; bool fromPlayer; };
struct DiepEnemy { float x, y, angle, hp, maxHp; int reload; bool alive; };

static DiepShape diepShapes[DIEP_SHAPES];
static DiepBullet diepBullets[DIEP_BULLETS];
static DiepEnemy diepEnemies[DIEP_ENEMIES];
static float diepPx, diepPy, diepAngle, diepHp, diepMaxHp;
static int diepReload, diepXp, diepLevel, diepScore;

static void spawnDiepBullet(float x, float y, float ang, bool fromPlayer) {
    for (int i = 0; i < DIEP_BULLETS; i++) {
        if (diepBullets[i].life <= 0) {
            float spd = fromPlayer ? (6.2f + diepLevel * 0.2f) : 4.6f;
            diepBullets[i] = {x + cosf(ang) * 16.0f, y + sinf(ang) * 16.0f, cosf(ang) * spd, sinf(ang) * spd, 48, fromPlayer};
            return;
        }
    }
}

static void initDiep() {
    diepPx = DIEP_WORLD * 0.5f;
    diepPy = DIEP_WORLD * 0.5f;
    diepAngle = 0.0f;
    diepMaxHp = 100.0f;
    diepHp = 100.0f;
    diepReload = 0;
    diepXp = 0;
    diepLevel = 1;
    diepScore = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < DIEP_BULLETS; i++) diepBullets[i].life = 0;
    for (int i = 0; i < DIEP_SHAPES; i++) {
        int k = (i % 7 == 0) ? 2 : ((i % 3 == 0) ? 1 : 0);
        float hp = (k == 2) ? 60.0f : ((k == 1) ? 25.0f : 12.0f);
        diepShapes[i] = {randRange(50, DIEP_WORLD - 50), randRange(50, DIEP_WORLD - 50), hp, hp, k};
    }
    for (int i = 0; i < DIEP_ENEMIES; i++) {
        diepEnemies[i] = {randRange(100, DIEP_WORLD - 100), randRange(100, DIEP_WORLD - 100), 0.0f, 60.0f, 60.0f, 20 + i * 7, true};
    }
}

static void stepDiep(eadk_keyboard_state_t kb) {
    float dx = 0.0f, dy = 0.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_left))  dx -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_right)) dx += 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_up))    dy -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_down))  dy += 1.0f;

    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.01f) {
        dx /= len; dy /= len;
        diepPx += dx * 2.9f;
        diepPy += dy * 2.9f;
        diepAngle = atan2f(dy, dx);
    }
    if (diepPx < 16) diepPx = 16;
    if (diepPx > DIEP_WORLD - 16) diepPx = DIEP_WORLD - 16;
    if (diepPy < 16) diepPy = 16;
    if (diepPy > DIEP_WORLD - 16) diepPy = DIEP_WORLD - 16;

    if (eadk_keyboard_key_down(kb, eadk_key_shift)) diepAngle -= 0.14f;
    if (eadk_keyboard_key_down(kb, eadk_key_alpha)) diepAngle += 0.14f;
    if (diepHp < diepMaxHp) diepHp += 0.06f;

    if (diepReload > 0) diepReload--;
    if (eadk_keyboard_key_down(kb, eadk_key_ok) && diepReload <= 0) {
        spawnDiepBullet(diepPx, diepPy, diepAngle, true);
        if (diepLevel >= 3) {
            spawnDiepBullet(diepPx, diepPy, diepAngle - 0.18f, true);
            spawnDiepBullet(diepPx, diepPy, diepAngle + 0.18f, true);
        }
        diepReload = diepLevel >= 5 ? 6 : 9;
    }

    for (int i = 0; i < DIEP_ENEMIES; i++) {
        DiepEnemy& e = diepEnemies[i];
        if (!e.alive) continue;
        float ang = atan2f(diepPy - e.y, diepPx - e.x);
        e.angle = ang;
        float d = sqrtf(dist2(e.x, e.y, diepPx, diepPy));
        if (d > 115.0f && d < 360.0f) {
            e.x += cosf(ang) * 1.45f;
            e.y += sinf(ang) * 1.45f;
        }
        if (e.reload > 0) e.reload--;
        if (d < 290.0f && e.reload <= 0) {
            spawnDiepBullet(e.x, e.y, e.angle, false);
            e.reload = 42;
        }
    }

    float dmg = 10.0f + diepLevel * 3.0f;
    for (int i = 0; i < DIEP_BULLETS; i++) {
        DiepBullet& b = diepBullets[i];
        if (b.life <= 0) continue;
        b.x += b.vx; b.y += b.vy; b.life--;
        if (b.x < 0 || b.x > DIEP_WORLD || b.y < 0 || b.y > DIEP_WORLD) { b.life = 0; continue; }

        if (b.fromPlayer) {
            for (int s = 0; s < DIEP_SHAPES; s++) {
                float r = diepShapes[s].kind == 2 ? 15.0f : (diepShapes[s].kind == 1 ? 11.0f : 9.0f);
                if (dist2(b.x, b.y, diepShapes[s].x, diepShapes[s].y) < (r + 4.0f) * (r + 4.0f)) {
                    diepShapes[s].hp -= dmg;
                    b.life = 0;
                    if (diepShapes[s].hp <= 0) {
                        int pts = diepShapes[s].kind == 2 ? 120 : (diepShapes[s].kind == 1 ? 45 : 20);
                        diepScore += pts;
                        diepXp += pts;
                        spawnParticles(diepShapes[s].x, diepShapes[s].y, C_YELLOW, 8);
                        diepShapes[s].x = randRange(50, DIEP_WORLD - 50);
                        diepShapes[s].y = randRange(50, DIEP_WORLD - 50);
                        diepShapes[s].hp = diepShapes[s].maxHp;
                    }
                    break;
                }
            }
            for (int e = 0; e < DIEP_ENEMIES && b.life > 0; e++) {
                if (!diepEnemies[e].alive) continue;
                if (dist2(b.x, b.y, diepEnemies[e].x, diepEnemies[e].y) < 17.0f * 17.0f) {
                    diepEnemies[e].hp -= dmg;
                    b.life = 0;
                    spawnParticles(b.x, b.y, C_ORANGE, 5);
                    if (diepEnemies[e].hp <= 0) {
                        diepScore += 250;
                        diepXp += 200;
                        spawnParticles(diepEnemies[e].x, diepEnemies[e].y, C_RED, 16);
                        diepEnemies[e].x = randRange(80, DIEP_WORLD - 80);
                        diepEnemies[e].y = randRange(80, DIEP_WORLD - 80);
                        diepEnemies[e].maxHp += 15.0f;
                        diepEnemies[e].hp = diepEnemies[e].maxHp;
                    }
                }
            }
        } else {
            if (dist2(b.x, b.y, diepPx, diepPy) < 16.0f * 16.0f) {
                diepHp -= 14.0f;
                b.life = 0;
                spawnParticles(diepPx, diepPy, C_RED, 6);
                if (diepHp <= 0.0f) {
                    lastScore = diepScore;
                    if (lastScore > bestScores[1]) bestScores[1] = lastScore;
                    lastPlayedMode = MODE_DIEP;
                    lastVictory = false;
                    currentMode = MODE_GAMEOVER;
                    return;
                }
            }
        }
    }

    int nextLvlXp = diepLevel * 180;
    if (diepXp >= nextLvlXp) {
        diepXp -= nextLvlXp;
        diepLevel++;
        diepMaxHp += 15.0f;
        diepHp = diepMaxHp;
        spawnParticles(diepPx, diepPy, C_CYAN, 16);
    }

    updateParticles();
    if (diepScore > bestScores[1]) bestScores[1] = diepScore;
}

static void renderDiep() {
    float camX = diepPx - W * 0.5f;
    float camY = diepPy - H * 0.5f;

    for (int by0 = 0; by0 < H; by0 += BAND_H) {
        fillRectBand(0, by0, W, BAND_H, by0, C_BG_DARK);

        int startGX = -(((int)camX) % 32);
        for (int gx = startGX; gx < W; gx += 32) fillRectBand(gx, by0, 1, BAND_H, by0, C_GRID);
        for (int py = by0; py < by0 + BAND_H; py++) {
            if ((((int)(py + camY)) % 32) == 0) fillRectBand(0, py, W, 1, by0, C_GRID);
        }

        for (int i = 0; i < DIEP_SHAPES; i++) {
            int sx = (int)(diepShapes[i].x - camX);
            int sy = (int)(diepShapes[i].y - camY);
            if (diepShapes[i].kind == 0) fillRectBand(sx - 7, sy - 7, 14, 14, by0, C_YELLOW);
            else if (diepShapes[i].kind == 1) fillCircleBand(sx, sy, 9, by0, C_RED);
            else {
                fillCircleBand(sx, sy, 14, by0, C_PURPLE);
                drawRingBand(sx, sy, 14, 12, by0, C_CYAN);
            }
        }

        for (int i = 0; i < DIEP_BULLETS; i++) {
            if (diepBullets[i].life > 0) {
                fillCircleBand((int)(diepBullets[i].x - camX), (int)(diepBullets[i].y - camY), 4, by0, diepBullets[i].fromPlayer ? C_CYAN : C_RED);
            }
        }

        for (int i = 0; i < DIEP_ENEMIES; i++) {
            if (!diepEnemies[i].alive) continue;
            int sx = (int)(diepEnemies[i].x - camX);
            int sy = (int)(diepEnemies[i].y - camY);
            int cx = sx + (int)(cosf(diepEnemies[i].angle) * 16.0f);
            int cy = sy + (int)(sinf(diepEnemies[i].angle) * 16.0f);
            drawLineBand(sx, sy, cx, cy, 4, by0, C_BORDER);
            fillCircleBand(sx, sy, 12, by0, C_RED);
            drawRingBand(sx, sy, 12, 10, by0, C_WHITE);
        }

        int psx = (int)(diepPx - camX);
        int psy = (int)(diepPy - camY);
        int bx = psx + (int)(cosf(diepAngle) * 18.0f);
        int by = psy + (int)(sinf(diepAngle) * 18.0f);
        drawLineBand(psx, psy, bx, by, 4, by0, C_BORDER);
        fillCircleBand(psx, psy, 13, by0, C_BLUE);
        drawRingBand(psx, psy, 13, 11, by0, C_CYAN);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].life > 0) {
                fillCircleBand((int)(particles[i].x - camX), (int)(particles[i].y - camY), 2, by0, particles[i].col);
            }
        }

        if (by0 >= 220) {
            fillRectBand(10, 226, 120, 7, by0, C_PANEL);
            int hpW = (int)(118.0f * (diepHp / diepMaxHp));
            if (hpW < 0) hpW = 0;
            fillRectBand(11, 227, hpW, 5, by0, C_GREEN);

            fillRectBand(140, 226, 170, 7, by0, C_PANEL);
            int xpW = (int)(168.0f * ((float)diepXp / (float)(diepLevel * 180)));
            if (xpW > 168) xpW = 168;
            fillRectBand(141, 227, xpW, 5, by0, C_YELLOW);
        }

        flushBand(by0);
    }

    char hud[52];
    snprintf(hud, sizeof(hud), "DIEP  Niv.%d  Score:%d  [OK:Tir]", diepLevel, diepScore);
    eadk_display_draw_string(hud, (eadk_point_t){6, 4}, false, C_WHITE, C_BG_DARK);
}

static constexpr float HOLE_WORLD = 1100.0f;
static constexpr int HOLE_PROPS = 55;
static constexpr int HOLE_BOTS = 5;

struct HoleProp { float x, y, size; int tier; eadk_color_t col; bool eaten; };
struct HoleBot { float x, y, r; eadk_color_t col; };

static HoleProp holeProps[HOLE_PROPS];
static HoleBot holeBots[HOLE_BOTS];
static float holePx, holePy, holeRadius;
static int holeScore;
static uint64_t holeStartTime;

static void initHole() {
    holePx = HOLE_WORLD * 0.5f;
    holePy = HOLE_WORLD * 0.5f;
    holeRadius = 14.0f;
    holeScore = 0;
    holeStartTime = eadk_timing_millis();
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < HOLE_PROPS; i++) {
        int tier = (i % 6 == 0) ? 2 : ((i % 2 == 0) ? 1 : 0);
        float sz = tier == 2 ? 26.0f : (tier == 1 ? 14.0f : 7.0f);
        eadk_color_t col = tier == 2 ? C_PURPLE : (tier == 1 ? PALETTE[i % 6] : C_GREEN);
        holeProps[i] = {randRange(40, HOLE_WORLD - 40), randRange(40, HOLE_WORLD - 40), sz, tier, col, false};
    }
    for (int i = 0; i < HOLE_BOTS; i++) {
        holeBots[i] = {randRange(100, HOLE_WORLD - 100), randRange(100, HOLE_WORLD - 100), randRange(13.0f, 22.0f), PALETTE[(i + 1) % 8]};
    }
}

static void stepHole(eadk_keyboard_state_t kb) {
    float dx = 0.0f, dy = 0.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_left))  dx -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_right)) dx += 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_up))    dy -= 1.0f;
    if (eadk_keyboard_key_down(kb, eadk_key_down))  dy += 1.0f;

    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.01f) { dx /= len; dy /= len; }

    float dash = eadk_keyboard_key_down(kb, eadk_key_ok) ? 1.45f : 1.0f;
    holePx += dx * 3.1f * dash;
    holePy += dy * 3.1f * dash;
    if (holePx < holeRadius) holePx = holeRadius;
    if (holePx > HOLE_WORLD - holeRadius) holePx = HOLE_WORLD - holeRadius;
    if (holePy < holeRadius) holePy = holeRadius;
    if (holePy > HOLE_WORLD - holeRadius) holePy = HOLE_WORLD - holeRadius;

    for (int i = 0; i < HOLE_PROPS; i++) {
        HoleProp& p = holeProps[i];
        if (p.eaten) continue;
        if (holeRadius > p.size * 0.88f && dist2(holePx, holePy, p.x, p.y) < holeRadius * holeRadius) {
            p.eaten = true;
            holeRadius += (p.tier == 2 ? 1.8f : (p.tier == 1 ? 0.95f : 0.45f));
            holeScore += (p.tier == 2 ? 150 : (p.tier == 1 ? 50 : 15));
            spawnParticles(p.x, p.y, p.col, 8);
            p.x = randRange(40, HOLE_WORLD - 40);
            p.y = randRange(40, HOLE_WORLD - 40);
            p.eaten = false;
        }
    }

    for (int i = 0; i < HOLE_BOTS; i++) {
        HoleBot& b = holeBots[i];
        float ang = atan2f(holePy - b.y, holePx - b.x);
        if (b.r > holeRadius * 1.1f) { b.x += cosf(ang) * 1.9f; b.y += sinf(ang) * 1.9f; }
        else { b.x -= cosf(ang) * 1.6f; b.y -= sinf(ang) * 1.6f; }
        if (b.x < b.r) b.x = b.r;
        if (b.x > HOLE_WORLD - b.r) b.x = HOLE_WORLD - b.r;
        if (b.y < b.r) b.y = b.r;
        if (b.y > HOLE_WORLD - b.r) b.y = HOLE_WORLD - b.r;

        float d2 = dist2(holePx, holePy, b.x, b.y);
        if (holeRadius > b.r * 1.12f && d2 < holeRadius * holeRadius) {
            holeRadius += b.r * 0.25f;
            holeScore += 300;
            spawnParticles(b.x, b.y, b.col, 14);
            b.x = randRange(80, HOLE_WORLD - 80);
            b.y = randRange(80, HOLE_WORLD - 80);
            b.r = holeRadius * 0.7f;
        } else if (b.r > holeRadius * 1.12f && d2 < b.r * b.r) {
            lastScore = holeScore;
            if (lastScore > bestScores[2]) bestScores[2] = lastScore;
            lastPlayedMode = MODE_HOLE;
            lastVictory = false;
            currentMode = MODE_GAMEOVER;
            return;
        }
    }

    updateParticles();
    if (holeScore > bestScores[2]) bestScores[2] = holeScore;

    uint64_t elapsed = (eadk_timing_millis() - holeStartTime) / 1000;
    if (elapsed >= 60) {
        lastScore = holeScore;
        if (lastScore > bestScores[2]) bestScores[2] = lastScore;
        lastPlayedMode = MODE_HOLE;
        lastVictory = true;
        currentMode = MODE_GAMEOVER;
    }
}

static void renderHole() {
    float zoom = 1.0f / (1.0f + (holeRadius - 14.0f) * 0.018f);
    if (zoom < 0.45f) zoom = 0.45f;
    float camX = holePx - (W * 0.5f) / zoom;
    float camY = holePy - (H * 0.5f) / zoom;

    for (int by0 = 0; by0 < H; by0 += BAND_H) {
        fillRectBand(0, by0, W, BAND_H, by0, C_ASPHALT);

        int block = (int)(90.0f * zoom);
        if (block < 24) block = 24;
        int sx0 = -(((int)(camX * zoom)) % block);
        for (int gx = sx0; gx < W; gx += block) fillRectBand(gx, by0, 2, BAND_H, by0, C_ROAD_LINE);
        for (int py = by0; py < by0 + BAND_H; py++) {
            if ((((int)(py + camY * zoom)) % block) == 0) fillRectBand(0, py, W, 2, by0, C_ROAD_LINE);
        }

        for (int i = 0; i < HOLE_BOTS; i++) {
            int sx = (int)((holeBots[i].x - camX) * zoom);
            int sy = (int)((holeBots[i].y - camY) * zoom);
            int sr = (int)(holeBots[i].r * zoom);
            fillCircleBand(sx, sy, sr, by0, C_BLACK);
            drawRingBand(sx, sy, sr + 2, sr, by0, holeBots[i].col);
        }

        int psx = (int)((holePx - camX) * zoom);
        int psy = (int)((holePy - camY) * zoom);
        int psr = (int)(holeRadius * zoom);
        fillCircleBand(psx, psy, psr, by0, C_BLACK);
        drawRingBand(psx, psy, psr + 2, psr, by0, C_CYAN);

        for (int i = 0; i < HOLE_PROPS; i++) {
            if (holeProps[i].eaten) continue;
            int sx = (int)((holeProps[i].x - camX) * zoom);
            int sy = (int)((holeProps[i].y - camY) * zoom);
            int sz = (int)(holeProps[i].size * zoom);
            if (holeProps[i].tier == 0) fillCircleBand(sx, sy, sz, by0, holeProps[i].col);
            else fillRectBand(sx - sz / 2, sy - sz / 2, sz, sz, by0, holeProps[i].col);
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].life > 0) {
                fillCircleBand((int)((particles[i].x - camX) * zoom), (int)((particles[i].y - camY) * zoom), 2, by0, particles[i].col);
            }
        }

        flushBand(by0);
    }

    int rem = 60 - (int)((eadk_timing_millis() - holeStartTime) / 1000);
    if (rem < 0) rem = 0;
    char hud[52];
    snprintf(hud, sizeof(hud), "HOLE  Temps:%ds  Taille:%d  Score:%d", rem, (int)holeRadius, holeScore);
    eadk_display_draw_string(hud, (eadk_point_t){6, 4}, false, C_WHITE, C_ASPHALT);
}

static void renderMenu() {
    eadk_display_push_rect_uniform(eadk_screen_rect, C_BG_DARK);
    eadk_display_push_rect_uniform((eadk_rect_t){0, 0, W, 36}, C_PANEL);
    eadk_display_draw_string("IO ARCADE - NUMWORKS", (eadk_point_t){58, 10}, true, C_CYAN, C_PANEL);

    const char* titles[3] = {
        "1. AGAR.IO  (Mange & grossis)",
        "2. DIEP.IO  (Tank & canons)",
        "3. HOLE.IO  (Avale la ville)"
    };
    const eadk_color_t cols[3] = {C_GREEN, C_ORANGE, C_PURPLE};

    for (int i = 0; i < 3; i++) {
        uint16_t y = (uint16_t)(52 + i * 50);
        bool sel = (menuSelection == i);
        eadk_color_t bg = sel ? C_BORDER : C_PANEL;
        eadk_display_push_rect_uniform((eadk_rect_t){16, y, 288, 40}, bg);
        if (sel) {
            eadk_display_push_rect_uniform((eadk_rect_t){16, y, 6, 40}, cols[i]);
        }
        eadk_display_draw_string(titles[i], (eadk_point_t){30, (uint16_t)(y + 6)}, false, sel ? C_WHITE : cols[i], bg);

        char rec[32];
        snprintf(rec, sizeof(rec), "Record : %d", bestScores[i]);
        eadk_display_draw_string(rec, (eadk_point_t){30, (uint16_t)(y + 22)}, false, C_YELLOW, bg);
    }

    eadk_display_draw_string("Fleches: Choisir | OK: Jouer | Home: Quitter", (eadk_point_t){12, 218}, false, C_WHITE, C_BG_DARK);
}

static void renderGameOver() {
    eadk_display_push_rect_uniform(eadk_screen_rect, C_BG_DARK);
    eadk_display_push_rect_uniform((eadk_rect_t){24, 34, 272, 172}, C_PANEL);

    eadk_display_draw_string(
        lastVictory ? "VICTOIRE ! TEMPS ECOULE" : "PARTIE TERMINEE !",
        (eadk_point_t){52, 54},
        true,
        lastVictory ? C_GREEN : C_RED,
        C_PANEL
    );

    char s1[36], s2[36];
    snprintf(s1, sizeof(s1), "Score final : %d", lastScore);
    snprintf(s2, sizeof(s2), "Meilleur score : %d", bestScores[(int)lastPlayedMode - 1]);
    eadk_display_draw_string(s1, (eadk_point_t){76, 98}, false, C_WHITE, C_PANEL);
    eadk_display_draw_string(s2, (eadk_point_t){76, 122}, false, C_YELLOW, C_PANEL);
    eadk_display_draw_string("OK: Rejouer  |  Back: Menu", (eadk_point_t){58, 166}, false, C_CYAN, C_PANEL);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    for (int i = 0; i < 55 * 56; ++i) {
        eadk_app_icon[i] = 0xFFFF;
    }
    rngState ^= (uint32_t)eadk_timing_millis();

    renderMenu();
    bool prevUp = false, prevDown = false, prevOk = false, prevBack = false;

    while (true) {
        uint64_t frameStart = eadk_timing_millis();
        eadk_keyboard_state_t kb = eadk_keyboard_scan();

        if (eadk_keyboard_key_down(kb, eadk_key_home) || eadk_keyboard_key_down(kb, eadk_key_on_off)) {
            return 0;
        }

        bool up = eadk_keyboard_key_down(kb, eadk_key_up);
        bool down = eadk_keyboard_key_down(kb, eadk_key_down);
        bool ok = eadk_keyboard_key_down(kb, eadk_key_ok);
        bool back = eadk_keyboard_key_down(kb, eadk_key_back);

        if (currentMode == MODE_MENU) {
            bool dirty = false;
            if (up && !prevUp) { menuSelection = (menuSelection + 2) % 3; dirty = true; }
            if (down && !prevDown) { menuSelection = (menuSelection + 1) % 3; dirty = true; }
            if (ok && !prevOk) {
                if (menuSelection == 0) { initAgar(); currentMode = MODE_AGAR; }
                else if (menuSelection == 1) { initDiep(); currentMode = MODE_DIEP; }
                else { initHole(); currentMode = MODE_HOLE; }
            }
            if (dirty) renderMenu();
        } else if (currentMode == MODE_GAMEOVER) {
            renderGameOver();
            if (ok && !prevOk) {
                if (lastPlayedMode == MODE_AGAR) initAgar();
                else if (lastPlayedMode == MODE_DIEP) initDiep();
                else initHole();
                currentMode = lastPlayedMode;
            } else if (back && !prevBack) {
                currentMode = MODE_MENU;
                renderMenu();
            }
        } else {
            if (back && !prevBack) {
                currentMode = MODE_MENU;
                renderMenu();
            } else if (currentMode == MODE_AGAR) {
                stepAgar(kb);
                if (currentMode == MODE_AGAR) renderAgar();
            } else if (currentMode == MODE_DIEP) {
                stepDiep(kb);
                if (currentMode == MODE_DIEP) renderDiep();
            } else if (currentMode == MODE_HOLE) {
                stepHole(kb);
                if (currentMode == MODE_HOLE) renderHole();
            }
        }

        prevUp = up; prevDown = down; prevOk = ok; prevBack = back;

        uint64_t elapsed = eadk_timing_millis() - frameStart;
        if (elapsed < 33) {
            eadk_timing_msleep((uint32_t)(33 - elapsed));
        }
    }
    return 0;
}
