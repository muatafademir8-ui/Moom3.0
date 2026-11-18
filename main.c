// MOOM - PSP / PPSSPP  (cok bolumlu surum)
// Gercek 3D dunya (doku, isik, sis), 2D sprite dusmanlar, 3D silah ve eller.
// Sadece saga-sola bakis (Doom gibi). 7 bolum: Zindan, Hastane, Fabrika, Depo, Kanalizasyon, Laboratuvar, Cati Kati.

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

PSP_MODULE_INFO("MOOM", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define BUF_WIDTH 512
#define SCR_W 480
#define SCR_H 272
#define PI 3.14159265f
#define RGBA(r,g,b,a) ((unsigned int)(r)|((unsigned int)(g)<<8)|((unsigned int)(b)<<16)|((unsigned int)(a)<<24))

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct { float u, v; unsigned int c; float x, y, z; } Vertex;
typedef struct { unsigned int c; float x, y, z; } V2;
#define VT  (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)
#define VT2 (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D)

/* ---------------------------------------------------------- callbacks */
static int exit_callback(int a, int b, void *c) { sceKernelExitGame(); return 0; }
static int callback_thread(SceSize args, void *argp) {
    int id = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    return 0;
}
static void setup_callbacks(void) {
    int t = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (t >= 0) sceKernelStartThread(t, 0, 0);
}

/* ---------------------------------------------------------- dokular */
#define TS 64
static unsigned int __attribute__((aligned(16))) texBrick[TS * TS];
static unsigned int __attribute__((aligned(16))) texTech[TS * TS];
static unsigned int __attribute__((aligned(16))) texFloor[TS * TS];
static unsigned int __attribute__((aligned(16))) texCeil[TS * TS];
static unsigned int __attribute__((aligned(16))) texPlank[TS * TS];
static unsigned int __attribute__((aligned(16))) texConcrete[TS * TS];
static unsigned int __attribute__((aligned(16))) texTile[TS * TS];
static unsigned int __attribute__((aligned(16))) texMetal[TS * TS];
static unsigned int __attribute__((aligned(16))) texSkin[TS * TS];
static unsigned int __attribute__((aligned(16))) texZombie[TS * TS];
static unsigned int __attribute__((aligned(16))) texImp[TS * TS];
static unsigned int __attribute__((aligned(16))) texBoss[TS * TS];
static unsigned int __attribute__((aligned(16))) texMed[TS * TS];
static unsigned int __attribute__((aligned(16))) texAmmo[TS * TS];
static unsigned int __attribute__((aligned(16))) texFlash[TS * TS];

static unsigned int hash2(int x, int y) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static int nz(int x, int y) { return (int)(hash2(x, y) & 255); }
static int cl(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
#define PX(t,x,y,r,g,b,a) (t)[(y) * TS + (x)] = RGBA(cl(r), cl(g), cl(b), (a))

static void mk_brick(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int row = y / 16, off = (row & 1) ? 16 : 0;
        int bx = (x + off) % 32, by = y % 16;
        int n = (nz(x, y) - 128) / 7;
        if (by < 2 || bx < 2) {
            int m = 38 + n / 2;
            PX(texBrick, x, y, m, m - 3, m - 6, 255);
        } else {
            int id = (x + off) / 32 + row * 5;
            int v = (int)(hash2(id, row + 9) & 31) - 14;
            int sh = (by > 12) ? -18 : ((by < 4) ? 12 : 0);
            PX(texBrick, x, y, 150 + v + n + sh, 62 + v / 2 + n / 2 + sh / 2, 44 + v / 3 + n / 3 + sh / 2, 255);
        }
    }
}
static void mk_tech(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int lx = x % 32, ly = y % 32;
        int n = (nz(x, y) - 128) / 10;
        int r = 88 + n, g = 96 + n, b = 108 + n;
        if (lx < 2 || ly < 2) { r = g = b = 28; }
        else if (lx == 2 || ly == 2) { r = 140; g = 150; b = 162; }
        int d1 = (lx - 6) * (lx - 6) + (ly - 6) * (ly - 6);
        int d2 = (lx - 26) * (lx - 26) + (ly - 6) * (ly - 6);
        int d3 = (lx - 6) * (lx - 6) + (ly - 26) * (ly - 26);
        int d4 = (lx - 26) * (lx - 26) + (ly - 26) * (ly - 26);
        if (d1 <= 4 || d2 <= 4 || d3 <= 4 || d4 <= 4) { r = 50; g = 54; b = 60; }
        if (x < 32 && ly >= 14 && ly <= 17 && lx >= 6 && lx <= 26) { r = 40; g = 230; b = 120; }
        PX(texTech, x, y, r, g, b, 255);
    }
}
static void mk_floor(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int lx = x % 32, ly = y % 32;
        int tile = x / 32 + (y / 32) * 2;
        int v = (int)(hash2(tile, 3) & 15) - 8;
        int n = (nz(x, y) - 128) / 9;
        int base = 84 + v + n;
        if (lx < 1 || ly < 1) base = 38; else if (lx < 2 || ly < 2) base = 60;
        PX(texFloor, x, y, base + 4, base, base - 6, 255);
    }
}
static void mk_ceil(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int lx = x % 16, ly = y % 16;
        int n = (nz(x, y) - 128) / 12;
        int base = 56 + n;
        if (lx < 1 || ly < 1) base = 24; else if (lx < 2 || ly < 2) base = 74;
        PX(texCeil, x, y, base, base + 2, base + 8, 255);
    }
}
static void mk_plank(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int board = y / 16, ly = y % 16;
        int sh = (int)(hash2(board, 7) & 31) - 14;
        int grain = (nz(x / 3, y) - 128) / 9 + (nz(x / 12, board * 3) - 128) / 12;
        int r = 140 + sh + grain, g = 94 + sh * 2 / 3 + grain * 2 / 3, b = 54 + sh / 2 + grain / 2;
        if (ly < 1) { r = g = b = 30; }
        else if (ly == 1) { r -= 30; g -= 22; b -= 14; }
        int lx = (x + board * 13) % 64;
        if (ly == 8 && (lx == 5 || lx == 59)) { r = 40; g = 34; b = 30; }
        PX(texPlank, x, y, r, g, b, 255);
    }
}
static void mk_concrete(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int n = (nz(x / 2, y / 2) - 128) / 10 + (nz(x, y) - 128) / 22;
        int base = 112 + n;
        if (x % 32 == 0 || y % 32 == 0) base -= 40;
        if (nz(x / 8, y / 8) > 230) base -= 18;
        PX(texConcrete, x, y, base, base, base + 3, 255);
    }
}
static void mk_tile(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int lx = x % 16, ly = y % 16;
        int n = (nz(x, y) - 128) / 16;
        int v = (int)(hash2(x / 16, y / 16) & 7) - 4;
        int r = 196 + v + n, g = 214 + v + n, b = 206 + v + n;
        if (lx == 0 || ly == 0) { r = 140; g = 150; b = 148; }
        if (y >= 28 && y <= 32) { r = 70 + n; g = 150 + n; b = 120 + n; }
        PX(texTile, x, y, r, g, b, 255);
    }
}
static void mk_metal(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int n = (nz(x, 0) - 128) / 12 + (nz(x, y) - 128) / 20;
        PX(texMetal, x, y, 175 + n, 177 + n, 186 + n, 255);
    }
}
static void mk_skin(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int n = (nz(x, y) - 128) / 10;
        PX(texSkin, x, y, 222 + n, 168 + n, 138 + n, 255);
    }
}
static void mk_flash(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        float dx = (x - 31.5f) / 32.0f, dy = (y - 31.5f) / 32.0f;
        float d = sqrtf(dx * dx + dy * dy);
        float a = 1.0f - d; if (a < 0) a = 0;
        a = a * a;
        PX(texFlash, x, y, 255, 200, 80, (int)(a * 255));
    }
}
static void mk_enemy(unsigned int *t, int br, int bg, int bb, int hr, int hg, int hb, int er, int eg, int eb, int horns) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        int r = 0, g = 0, b = 0, a = 0;
        int n = (nz(x, y) - 128) / 9;
        float side = 1.0f - 0.35f * fabsf((x - 32) / 32.0f);
        if (y >= 44 && ((x >= 20 && x <= 30) || (x >= 34 && x <= 44))) {
            r = (int)(br * 0.6f * side) + n; g = (int)(bg * 0.6f * side) + n; b = (int)(bb * 0.6f * side) + n; a = 255;
        }
        if (x >= 16 && x <= 48 && y >= 22 && y <= 46) {
            int rib = (y % 5 == 0) ? -25 : 0;
            r = (int)(br * side) + n + rib; g = (int)(bg * side) + n + rib; b = (int)(bb * side) + n + rib; a = 255;
        }
        if (((x >= 7 && x <= 16) || (x >= 48 && x <= 57)) && y >= 24 && y <= 44) {
            r = (int)(br * 1.1f * side) + n; g = (int)(bg * 1.1f * side) + n; b = (int)(bb * 1.1f * side) + n; a = 255;
            if (y >= 40) { r = 220; g = 215; b = 190; }
        }
        float dx = (float)(x - 32), dy = (float)(y - 14);
        if (dx * dx + dy * dy <= 121.0f) {
            r = (int)(hr * side) + n; g = (int)(hg * side) + n; b = (int)(hb * side) + n; a = 255;
        }
        if (a && y >= 11 && y <= 14 && ((x >= 26 && x <= 30) || (x >= 34 && x <= 38))) { r = er; g = eg; b = eb; }
        if (a && y >= 19 && y <= 21 && x >= 27 && x <= 37) { r = 30; g = 0; b = 0; }
        if (horns && y >= 2 && y <= 9 && ((x >= 21 && x <= 24) || (x >= 40 && x <= 43))) { r = 230; g = 220; b = 190; a = 255; }
        if (a) PX(t, x, y, r, g, b, 255); else t[y * TS + x] = 0;
    }
}
static void mk_med(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        unsigned int c = 0;
        if (x >= 14 && x <= 50 && y >= 30 && y <= 58) {
            int n = (nz(x, y) - 128) / 14;
            c = RGBA(cl(225 + n), cl(225 + n), cl(228 + n), 255);
            if ((x >= 28 && x <= 36 && y >= 34 && y <= 54) || (x >= 20 && x <= 44 && y >= 42 && y <= 48)) c = RGBA(205, 20, 20, 255);
            if (x <= 15 || x >= 49 || y <= 31 || y >= 57) c = RGBA(120, 120, 125, 255);
        }
        texMed[y * TS + x] = c;
    }
}
static void mk_ammo(void) {
    for (int y = 0; y < TS; y++) for (int x = 0; x < TS; x++) {
        unsigned int c = 0;
        int n = (nz(x, y) - 128) / 14;
        if (x >= 16 && x <= 48 && y >= 38 && y <= 58) {
            c = RGBA(cl(190 + n), cl(150 + n), cl(40 + n), 255);
            if (y >= 46 && y <= 49) c = RGBA(110, 80, 20, 255);
            if (x <= 17 || x >= 47 || y <= 39 || y >= 57) c = RGBA(90, 65, 15, 255);
        }
        if (y >= 26 && y < 38 && ((x >= 20 && x <= 25) || (x >= 29 && x <= 34) || (x >= 38 && x <= 43))) {
            c = (y < 31) ? RGBA(215, 170, 50, 255) : RGBA(200, 40, 30, 255);
        }
        texAmmo[y * TS + x] = c;
    }
}
static void make_textures(void) {
    mk_brick(); mk_tech(); mk_floor(); mk_ceil(); mk_plank(); mk_concrete(); mk_tile();
    mk_metal(); mk_skin(); mk_flash(); mk_med(); mk_ammo();
    mk_enemy(texZombie, 96, 110, 70, 120, 160, 100, 255, 40, 20, 0);   /* zombi: yesil-kahve */
    mk_enemy(texImp, 150, 60, 40, 190, 90, 60, 255, 230, 40, 1);       /* iblis: kirmizi, boynuzlu */
    mk_enemy(texBoss, 80, 40, 110, 120, 70, 140, 80, 255, 80, 1);      /* patron: mor, yesil gozlu */
    sceKernelDcacheWritebackAll();
}
static void bind(unsigned int *t) {
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, TS, TS, TS, t);
}

/* ---------------------------------------------------------- temalar */
typedef struct {
    unsigned int *wa, *wb, *fl, *ce;   /* '#' duvar, '=' duvar, zemin, tavan */
    unsigned int fog;
    float amb[3];
    float pal[3][3];                   /* lamba renkleri */
} Theme;

static const Theme TH[] = {
    /* 0 zindan */  { texBrick, texTech, texFloor, texCeil, RGBA(10, 5, 8, 255), {0.20f, 0.20f, 0.25f},
                      {{1.0f, 0.75f, 0.40f}, {1.0f, 0.30f, 0.25f}, {0.40f, 0.60f, 1.0f}} },
    /* 1 hastane */ { texTile, texTech, texFloor, texCeil, RGBA(8, 14, 14, 255), {0.30f, 0.34f, 0.34f},
                      {{0.70f, 1.0f, 0.90f}, {0.90f, 1.0f, 1.0f}, {0.50f, 1.0f, 0.60f}} },
    /* 2 fabrika */ { texConcrete, texBrick, texFloor, texCeil, RGBA(14, 6, 4, 255), {0.22f, 0.18f, 0.16f},
                      {{1.0f, 0.55f, 0.20f}, {1.0f, 0.30f, 0.15f}, {1.0f, 0.80f, 0.35f}} },
    /* 3 depo */    { texConcrete, texPlank, texFloor, texCeil, RGBA(6, 8, 12, 255), {0.20f, 0.22f, 0.28f},
                      {{1.0f, 0.90f, 0.55f}, {0.55f, 0.70f, 1.0f}, {1.0f, 0.75f, 0.40f}} },
    /* 4 cati kati */ { texPlank, texBrick, texPlank, texPlank, RGBA(12, 8, 4, 255), {0.24f, 0.19f, 0.15f},
                      {{1.0f, 0.65f, 0.25f}, {1.0f, 0.50f, 0.20f}, {0.55f, 0.65f, 1.0f}} },
    /* 5 kanalizasyon */ { texConcrete, texTech, texFloor, texCeil, RGBA(4, 12, 6, 255), {0.16f, 0.24f, 0.18f},
                      {{0.60f, 1.0f, 0.40f}, {0.90f, 1.0f, 0.40f}, {0.40f, 0.90f, 0.70f}} },
    /* 6 laboratuvar */ { texTile, texTech, texFloor, texCeil, RGBA(8, 4, 14, 255), {0.25f, 0.22f, 0.34f},
                      {{0.80f, 0.40f, 1.0f}, {0.40f, 0.90f, 1.0f}, {1.0f, 0.40f, 0.80f}} }
};
static const Theme *th = &TH[0];

/* ---------------------------------------------------------- bolumler */
typedef struct {
    const char *name;
    int w, h;
    const char *const *rows;
    int theme;
    float yaw;
} Level;

static const char *const LV0_ROWS[] = {
    "################",
    "#P.....#.......#",
    "#..Z...#...S...#",
    "#......#.......#",
    "###.####...###.#",
    "#......=.......#",
    "#..H.......Z...#",
    "#......=...A...#",
    "###.#####..#####",
    "#.......=......#",
    "#..Z....=..S...#",
    "#.......=......#",
    "##.####=###.####",
    "#......#.......#",
    "#.H..Z.#..Z..A.#",
    "################"
};
static const char *const LV1_ROWS[] = {
    "####################",
    "#P...=.....=.......#",
    "#....#..Z..#...S...#",
    "#..................#",
    "#....=.....=...Z...#",
    "##.#####.#####.#####",
    "#..................#",
    "#..H...........A...#",
    "##.###.#######.#####",
    "#...#..#.....#.....#",
    "#.Z.......S.....Z..#",
    "#...#..#.....#.....#",
    "#...#..#..H..#...A.#",
    "#...#..#.....#.....#",
    "#..Z#..#..Z..#..Z..#",
    "####################"
};
static const char *const LV2_ROWS[] = {
    "######################",
    "#P...................#",
    "#.......=..S.=.......#",
    "#.....Z..............#",
    "#...=............=...#",
    "#.............A....Z.#",
    "######...####........#",
    "#........####........#",
    "#........####..S.....#",
    "#...=....####....=.H.#",
    "#..Z.....####........#",
    "#........####...######",
    "#....S..A............#",
    "#...=.........Z..=...#",
    "#.H........A.........#",
    "#......Z=....=.....S.#",
    "#....................#",
    "######################"
};
static const char *const LV3_ROWS[] = {
    "########################",
    "#......................#",
    "#...==...==...==...==..#",
    "#.Z.==...==...==...==B.#",
    "#......Z...............#",
    "#..A..................S#",
    "#...==...==...==...==..#",
    "#...==...==...==...==..#",
    "#.S...H.....Z...S......#",
    "#.....................A#",
    "###########.H###########",
    "#H.....................#",
    "#...==...==...==...==..#",
    "#...==.Z.==...==...==..#",
    "#.......A..............#",
    "#...==...==...==...==..#",
    "#...==...==S..==...==..#",
    "#...........Z........Z.#",
    "#P..........A..........#",
    "########################"
};
static const char *const LV4_ROWS[] = {
    "######################",
    "#P...................#",
    "#.....Z........Z.....#",
    "#....A....S..........#",
    "#...=...=...=...=..S.#",
    "#....................#",
    "#......Z.........Z...#",
    "#....................#",
    "#######.=...=..#######",
    "#............A.......#",
    "#.H.......Z........H.#",
    "#....................#",
    "#..Z=...=...=...=....#",
    "#.......A........A...#",
    "#.....S.......S......#",
    "#..................Z.#",
    "#.........H..........#",
    "######################"
};
static const char *const LV5_ROWS[] = {
    "####################",
    "#P................H#",
    "#.......Z....SA....#",
    "#................Z.#",
    "#..................#",
    "#....====..====....#",
    "#.A..=........=..A.#",
    "#....=.S......=....#",
    "#....=..#..#..=....#",
    "#....=...B....=....#",
    "#.Z..=........=..Z.#",
    "#....=..#..#..=....#",
    "#....=...H..S.=....#",
    "#....=........=....#",
    "#....====..====....#",
    "#..................#",
    "#.S..............S.#",
    "#....A..Z......Z...#",
    "#H.................#",
    "####################"
};
static const char *const LV6_ROWS[] = {
    "####################",
    "#P.................#",
    "#..A....=...Z....S.#",
    "#........H.........#",
    "#################..#",
    "#..................#",
    "#.S..Z........=.A..#",
    "#.........H........#",
    "#..#################",
    "#..................#",
    "#..A....=...Z....S.#",
    "#........H.........#",
    "#################..#",
    "#..................#",
    "#.S...Z.......=.A..#",
    "#.........H........#",
    "#..#################",
    "#..................#",
    "#.......=.S..Z..B..#",
    "#.....A..........B.#",
    "####################"
};
static const Level LV[] = {
    {"ZINDAN", 16, 16, LV0_ROWS, 0, 1.5708f},
    {"HASTANE", 20, 16, LV1_ROWS, 1, 1.5708f},
    {"FABRIKA", 22, 18, LV2_ROWS, 2, 1.5708f},
    {"DEPO", 24, 20, LV3_ROWS, 3, 0.0f},
    {"KANALIZASYON", 22, 18, LV4_ROWS, 5, 1.5708f},
    {"LABORATUVAR", 20, 20, LV5_ROWS, 6, 1.5708f},
    {"CATI KATI", 20, 21, LV6_ROWS, 4, 1.5708f},
};

#define NLEV ((int)(sizeof(LV) / sizeof(LV[0])))

#define MAXL 40
static const char *const *curMap;
static int curW, curH, curLevel;

typedef struct { float x, z, r, g, b; } Lamp;
static Lamp lamps[MAXL];
static int nlamp;

static int is_wall_cell(int cx, int cz) {
    if (cx < 0 || cz < 0 || cx >= curW || cz >= curH) return 1;
    char c = curMap[cz][cx];
    return c == '#' || c == '=';
}
static int is_wall(float x, float z) { return is_wall_cell((int)floorf(x), (int)floorf(z)); }
static int blocked(float x, float z, float r) {
    return is_wall(x - r, z - r) || is_wall(x + r, z - r) || is_wall(x - r, z + r) || is_wall(x + r, z + r);
}
static int los(float x0, float z0, float x1, float z1) {
    float dx = x1 - x0, dz = z1 - z0;
    float d = sqrtf(dx * dx + dz * dz);
    int n = (int)(d / 0.15f) + 1;
    for (int i = 1; i < n; i++) {
        float t = (float)i / (float)n;
        if (is_wall(x0 + dx * t, z0 + dz * t)) return 0;
    }
    return 1;
}

/* ---------------------------------------------------------- isik */
static unsigned int lit(float x, float y, float z, float k) {
    float r = th->amb[0], g = th->amb[1], b = th->amb[2];
    for (int i = 0; i < nlamp; i++) {
        float dx = x - lamps[i].x, dy = y - 0.95f, dz = z - lamps[i].z;
        float a = 1.1f / (1.0f + 0.8f * (dx * dx + dy * dy + dz * dz));
        r += lamps[i].r * a; g += lamps[i].g * a; b += lamps[i].b * a;
    }
    r *= k; g *= k; b *= k;
    if (r > 1) r = 1;
    if (g > 1) g = 1;
    if (b > 1) b = 1;
    return RGBA((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

/* ---------------------------------------------------------- dunya geometrisi */
static Vertex __attribute__((aligned(16))) wA[4500];
static Vertex __attribute__((aligned(16))) wB[4500];
static Vertex __attribute__((aligned(16))) wFloor[4500];
static Vertex __attribute__((aligned(16))) wCeil[4500];
static int nA, nB, nFloor, nCeil;

static float ao_corner(int i, int j) {
    int c = 0;
    for (int a = -1; a <= 0; a++) for (int b = -1; b <= 0; b++) if (is_wall_cell(i + a, j + b)) c++;
    return 1.0f - 0.16f * (float)c;
}
static Vertex *flat(Vertex *v, int cx, int cz, float y, float dark) {
    float xs[4] = {(float)cx, (float)(cx + 1), (float)(cx + 1), (float)cx};
    float zs[4] = {(float)cz, (float)cz, (float)(cz + 1), (float)(cz + 1)};
    Vertex q[4];
    for (int i = 0; i < 4; i++) {
        q[i].u = xs[i]; q[i].v = zs[i]; q[i].x = xs[i]; q[i].y = y; q[i].z = zs[i];
        q[i].c = lit(xs[i], y, zs[i], ao_corner((int)xs[i], (int)zs[i]) * dark);
    }
    v[0] = q[0]; v[1] = q[1]; v[2] = q[2]; v[3] = q[0]; v[4] = q[2]; v[5] = q[3];
    return v + 6;
}
static Vertex *wallface(Vertex *v, float x0, float z0, float x1, float z1, float k) {
    unsigned int cb0 = lit(x0, 0.0f, z0, k * 0.7f), cb1 = lit(x1, 0.0f, z1, k * 0.7f);
    unsigned int ct0 = lit(x0, 1.0f, z0, k), ct1 = lit(x1, 1.0f, z1, k);
    v[0] = (Vertex){0, 1, cb0, x0, 0, z0};
    v[1] = (Vertex){1, 1, cb1, x1, 0, z1};
    v[2] = (Vertex){1, 0, ct1, x1, 1, z1};
    v[3] = v[0]; v[4] = v[2];
    v[5] = (Vertex){0, 0, ct0, x0, 1, z0};
    return v + 6;
}
static void build_world(void) {
    Vertex *pa = wA, *pb = wB, *pf = wFloor, *pc = wCeil;
    for (int cz = 0; cz < curH; cz++) for (int cx = 0; cx < curW; cx++) {
        char c = curMap[cz][cx];
        if (c == '#' || c == '=') {
            Vertex **p = (c == '#') ? &pa : &pb;
            float x = (float)cx, z = (float)cz;
            if (!is_wall_cell(cx, cz - 1)) *p = wallface(*p, x, z, x + 1, z, 1.0f);
            if (!is_wall_cell(cx, cz + 1)) *p = wallface(*p, x, z + 1, x + 1, z + 1, 0.95f);
            if (!is_wall_cell(cx - 1, cz)) *p = wallface(*p, x, z, x, z + 1, 0.80f);
            if (!is_wall_cell(cx + 1, cz)) *p = wallface(*p, x + 1, z, x + 1, z + 1, 0.85f);
        } else {
            pf = flat(pf, cx, cz, 0.0f, 1.0f);
            pc = flat(pc, cx, cz, 1.0f, 0.75f);
        }
    }
    nA = (int)(pa - wA); nB = (int)(pb - wB);
    nFloor = (int)(pf - wFloor); nCeil = (int)(pc - wCeil);
    sceKernelDcacheWritebackAll();
}

/* ---------------------------------------------------------- 3D silah modeli */
static Vertex __attribute__((aligned(16))) gunA[36 * 6];   /* metal, sabit */
static Vertex __attribute__((aligned(16))) gunB[36 * 3];   /* ten, sabit */
static Vertex __attribute__((aligned(16))) gunC[36 * 3];   /* metal, pompa ile hareket eder */
static Vertex __attribute__((aligned(16))) gunD[36 * 3];   /* ten, hareketli (sol el) */
static int gA, gB, gC, gD;

static const int FACE[6][4] = {{0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{3,2,6,7},{4,5,1,0}};
static const float FSH[6] = {0.80f, 0.60f, 0.70f, 0.90f, 1.00f, 0.40f};

static Vertex *box(Vertex *v, float cx, float cy, float cz, float sx, float sy, float sz, int r, int g, int b) {
    float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
    float P[8][3] = {
        {cx - hx, cy - hy, cz + hz}, {cx + hx, cy - hy, cz + hz}, {cx + hx, cy + hy, cz + hz}, {cx - hx, cy + hy, cz + hz},
        {cx - hx, cy - hy, cz - hz}, {cx + hx, cy - hy, cz - hz}, {cx + hx, cy + hy, cz - hz}, {cx - hx, cy + hy, cz - hz}
    };
    static const float U[4] = {0, 1, 1, 0}, V[4] = {1, 1, 0, 0};
    for (int f = 0; f < 6; f++) {
        unsigned int col = RGBA(cl((int)(r * FSH[f])), cl((int)(g * FSH[f])), cl((int)(b * FSH[f])), 255);
        Vertex q[4];
        for (int i = 0; i < 4; i++) {
            const float *p = P[FACE[f][i]];
            q[i].u = U[i]; q[i].v = V[i]; q[i].c = col; q[i].x = p[0]; q[i].y = p[1]; q[i].z = p[2];
        }
        v[0] = q[0]; v[1] = q[1]; v[2] = q[2]; v[3] = q[0]; v[4] = q[2]; v[5] = q[3];
        v += 6;
    }
    return v;
}
static void build_gun(void) {
    Vertex *p;
    p = gunA;
    p = box(p, 0.00f, 0.025f, -0.32f, 0.040f, 0.040f, 0.62f, 200, 200, 210);  /* namlu */
    p = box(p, 0.00f, -0.020f, -0.28f, 0.034f, 0.030f, 0.50f, 150, 150, 160);  /* sarjor tupu */
    p = box(p, 0.00f, -0.010f, 0.00f, 0.070f, 0.085f, 0.24f, 125, 125, 135);   /* govde */
    p = box(p, 0.00f, -0.060f, 0.22f, 0.050f, 0.080f, 0.22f, 150, 100, 60);    /* dipcik */
    p = box(p, 0.01f, -0.130f, 0.26f, 0.075f, 0.070f, 0.30f, 75, 100, 65);     /* sag kol */
    gA = (int)(p - gunA);
    p = gunB;
    p = box(p, 0.00f, -0.085f, 0.10f, 0.065f, 0.070f, 0.09f, 255, 255, 255);   /* sag el */
    p = box(p, -0.04f, -0.045f, 0.08f, 0.025f, 0.030f, 0.07f, 255, 255, 255);  /* basparmak */
    gB = (int)(p - gunB);
    p = gunC;
    p = box(p, 0.00f, -0.030f, -0.30f, 0.065f, 0.050f, 0.15f, 160, 105, 60);   /* pompa */
    p = box(p, -0.03f, -0.110f, -0.12f, 0.070f, 0.060f, 0.28f, 75, 100, 65);   /* sol kol */
    gC = (int)(p - gunC);
    p = gunD;
    p = box(p, 0.00f, -0.075f, -0.30f, 0.080f, 0.050f, 0.10f, 255, 255, 255);  /* sol el */
    gD = (int)(p - gunD);
    sceKernelDcacheWritebackAll();
}

/* ---------------------------------------------------------- oyun durumu */
/* tip: 0 zombi, 1 iblis (uzaktan), 2 patron */
typedef struct { float x, z; int type, hp, state; float cd, hurt, dead; int awake; } Enemy;
typedef struct { float x, z; int type, active; } Item;
#define MAXE 32
static Enemy en[MAXE]; static int nen;
static Item it[MAXE]; static int nit;
static float px, pz, yaw, bob;
static int hp, ammo, kills, total, gstate;      /* gstate: 0 oyun, 1 oldun, 2 bolum bitti */
static float cdFire, fireAnim, flash, dmgf, healf, tsec, levelT;
static SceCtrlData pad, padp;

static void load_level(int idx, int carry) {
    const Level *L = &LV[idx];
    curLevel = idx; curMap = L->rows; curW = L->w; curH = L->h;
    th = &TH[L->theme];
    nen = 0; nit = 0; nlamp = 0;
    for (int z = 0; z < curH; z++) for (int x = 0; x < curW; x++) {
        char c = curMap[z][x];
        if (c == 'P') { px = x + 0.5f; pz = z + 0.5f; }
        if ((c == 'Z' || c == 'S' || c == 'B') && nen < MAXE) {
            int type = (c == 'Z') ? 0 : ((c == 'S') ? 1 : 2);
            int hpv = (type == 0) ? 60 : ((type == 1) ? 40 : 220);
            Enemy e = {x + 0.5f, z + 0.5f, type, hpv, 0, 1.0f, 0, 0, 0};
            en[nen++] = e;
        }
        if ((c == 'H' || c == 'A') && nit < MAXE) {
            Item i = {x + 0.5f, z + 0.5f, (c == 'H') ? 0 : 1, 1};
            it[nit++] = i;
        }
    }
    for (int z = 0; z < curH; z++) for (int x = 0; x < curW; x++) {
        if (!is_wall_cell(x, z) && (x % 4) == 2 && (z % 4) == 2 && nlamp < MAXL) {
            int k = (int)(hash2(x, z) % 3u);
            Lamp l = {x + 0.5f, z + 0.5f, th->pal[k][0], th->pal[k][1], th->pal[k][2]};
            lamps[nlamp++] = l;
        }
    }
    build_world();
    yaw = L->yaw; bob = 0;
    if (carry) {
        hp += 30; if (hp > 100) hp = 100;
        ammo += 12; if (ammo > 99) ammo = 99;
    } else { hp = 100; ammo = 24; }
    kills = 0; total = nen; gstate = 0;
    cdFire = 0; fireAnim = 0; flash = 0; dmgf = 0; healf = 0;
    levelT = 3.0f;
}

static void damage(int n) {
    hp -= n; dmgf = 0.35f;
    if (hp <= 0) { hp = 0; gstate = 1; }
}

static void shoot(void) {
    float fx = sinf(yaw), fz = -cosf(yaw), rx = cosf(yaw), rz = sinf(yaw);
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        if (e->state != 0) continue;
        float dx = e->x - px, dz = e->z - pz;
        float d = sqrtf(dx * dx + dz * dz);
        if (d < 12.0f && los(px, pz, e->x, e->z)) e->awake = 1;
        float along = dx * fx + dz * fz;
        if (along < 0.2f) continue;
        float lat = dx * rx + dz * rz;
        float tol = 0.38f + along * 0.07f + ((e->type == 2) ? 0.25f : 0.0f);
        if (fabsf(lat) < tol && los(px, pz, e->x, e->z)) {
            int dmg = (int)(70.0f / (1.0f + 0.35f * along));
            if (dmg < 12) dmg = 12;
            e->hp -= dmg; e->hurt = 0.15f; e->awake = 1;
            if (e->hp <= 0) {
                e->state = 1; e->dead = 0; kills++;
                ammo += (e->type == 2) ? 12 : 5;
                if (ammo > 99) ammo = 99;
            }
        }
    }
}

static void update(float dt) {
    tsec += dt;
    int newStart = (pad.Buttons & PSP_CTRL_START) && !(padp.Buttons & PSP_CTRL_START);
    if (gstate != 0) {
        if (newStart) {
            if (gstate == 1) load_level(curLevel, 0);
            else if (curLevel + 1 < NLEV) load_level(curLevel + 1, 1);
            else load_level(0, 0);
        }
        return;
    }
    if (levelT > 0) levelT -= dt;

    float mv = 0, st = 0, tn = 0;
    float ay = ((float)pad.Ly - 128.0f) / 128.0f, ax = ((float)pad.Lx - 128.0f) / 128.0f;
    if (fabsf(ay) > 0.2f) mv -= ay;
    if (fabsf(ax) > 0.2f) tn += ax;
    if (pad.Buttons & PSP_CTRL_UP) mv += 1;
    if (pad.Buttons & PSP_CTRL_DOWN) mv -= 1;
    if (pad.Buttons & PSP_CTRL_RIGHT) tn += 1;
    if (pad.Buttons & PSP_CTRL_LEFT) tn -= 1;
    if (pad.Buttons & PSP_CTRL_RTRIGGER) st += 1;
    if (pad.Buttons & PSP_CTRL_LTRIGGER) st -= 1;
    if (mv > 1) mv = 1;
    if (mv < -1) mv = -1;
    if (tn > 1) tn = 1;
    if (tn < -1) tn = -1;

    yaw += tn * 2.6f * dt;
    float speed = (pad.Buttons & PSP_CTRL_CIRCLE) ? 4.0f : 2.6f;
    float fx = sinf(yaw), fz = -cosf(yaw), rx = cosf(yaw), rz = sinf(yaw);
    float dx = (fx * mv + rx * st) * speed * dt, dz = (fz * mv + rz * st) * speed * dt;
    if (!blocked(px + dx, pz, 0.2f)) px += dx;
    if (!blocked(px, pz + dz, 0.2f)) pz += dz;
    if (fabsf(mv) > 0.05f || fabsf(st) > 0.05f) bob += dt * (speed > 3.0f ? 11.0f : 8.0f);

    /* ates */
    if (cdFire > 0) cdFire -= dt;
    if (fireAnim > 0) { fireAnim -= dt / 0.6f; if (fireAnim < 0) fireAnim = 0; }
    if (flash > 0) flash -= dt;
    if (dmgf > 0) dmgf -= dt;
    if (healf > 0) healf -= dt;
    int fire = (pad.Buttons & (PSP_CTRL_CROSS | PSP_CTRL_SQUARE)) != 0;
    if (fire && cdFire <= 0) {
        if (ammo > 0) { ammo--; cdFire = 0.6f; fireAnim = 1.0f; flash = 0.08f; shoot(); }
        else cdFire = 0.3f;
    }

    /* dusmanlar */
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        if (e->state == 1) { e->dead += dt; if (e->dead > 0.6f) e->state = 2; continue; }
        if (e->state == 2) continue;
        if (e->hurt > 0) e->hurt -= dt;
        e->cd -= dt;
        float ex = px - e->x, ez = pz - e->z;
        float d = sqrtf(ex * ex + ez * ez);
        int vis = (d < 16.0f) && los(e->x, e->z, px, pz);
        if (vis && d < 12.0f) e->awake = 1;
        if (!e->awake) continue;
        float sp = (e->type == 0) ? 1.5f : ((e->type == 1) ? 1.0f : 0.8f);
        float stop = (e->type == 0) ? 0.8f : ((e->type == 1) ? 5.0f : 6.0f);
        float rad = (e->type == 2) ? 0.4f : 0.28f;
        if (d > stop && d > 0.01f) {
            float mx = ex / d * sp * dt, mz = ez / d * sp * dt;
            if (!blocked(e->x + mx, e->z, rad)) e->x += mx;
            if (!blocked(e->x, e->z + mz, rad)) e->z += mz;
        }
        if (e->type == 0 && d < 1.0f && e->cd <= 0) { damage(10); e->cd = 0.9f; }
        if (e->type == 1 && vis && d < 9.0f && e->cd <= 0) {
            e->cd = 1.5f;
            if (rand() % 100 < 65) damage(9 + rand() % 5);
        }
        if (e->type == 2 && vis && d < 11.0f && e->cd <= 0) {
            e->cd = 1.1f;
            if (rand() % 100 < 70) damage(12 + rand() % 8);
        }
    }

    /* esyalar */
    for (int i = 0; i < nit; i++) {
        if (!it[i].active) continue;
        float dx2 = it[i].x - px, dz2 = it[i].z - pz;
        if (dx2 * dx2 + dz2 * dz2 < 0.36f) {
            if (it[i].type == 0) { if (hp < 100) { hp += 25; if (hp > 100) hp = 100; healf = 0.3f; it[i].active = 0; } }
            else { ammo += 8; if (ammo > 99) ammo = 99; healf = 0.2f; it[i].active = 0; }
        }
    }

    if (kills >= total) gstate = 2;
}

/* ---------------------------------------------------------- cizim yardimcilari */
static void sprite(float x, float z, float w, float h, unsigned int col) {
    float rx = cosf(yaw) * w * 0.5f, rz = sinf(yaw) * w * 0.5f;
    Vertex *v = (Vertex *)sceGuGetMemory(6 * sizeof(Vertex));
    v[0] = (Vertex){0, 1, col, x - rx, 0, z - rz};
    v[1] = (Vertex){1, 1, col, x + rx, 0, z + rz};
    v[2] = (Vertex){1, 0, col, x + rx, h, z + rz};
    v[3] = v[0]; v[4] = v[2];
    v[5] = (Vertex){0, 0, col, x - rx, h, z - rz};
    sceGuDrawArray(GU_TRIANGLES, VT, 6, 0, v);
}

static void rect(float x, float y, float w, float h, unsigned int c) {
    V2 *v = (V2 *)sceGuGetMemory(2 * sizeof(V2));
    v[0] = (V2){c, x, y, 0};
    v[1] = (V2){c, x + w, y + h, 0};
    sceGuDrawArray(GU_SPRITES, VT2, 2, 0, v);
}

typedef struct { char c; unsigned char r[5]; } Gl;
static const Gl GLY[] = {
    {'0',{7,5,5,5,7}},{'1',{2,6,2,2,7}},{'2',{7,1,7,4,7}},{'3',{7,1,7,1,7}},{'4',{5,5,7,1,1}},
    {'5',{7,4,7,1,7}},{'6',{7,4,7,5,7}},{'7',{7,1,1,1,1}},{'8',{7,5,7,5,7}},{'9',{7,5,7,1,7}},
    {'A',{2,5,7,5,5}},{'B',{6,5,6,5,6}},{'C',{7,4,4,4,7}},{'D',{6,5,5,5,6}},{'E',{7,4,7,4,7}},
    {'F',{7,4,7,4,4}},{'G',{7,4,5,5,7}},{'H',{5,5,7,5,5}},{'I',{7,2,2,2,7}},{'J',{1,1,1,5,7}},
    {'K',{5,5,6,5,5}},{'L',{4,4,4,4,7}},{'M',{5,7,7,5,5}},{'N',{7,5,5,5,5}},{'O',{7,5,5,5,7}},
    {'P',{7,5,7,4,4}},{'Q',{7,5,5,7,1}},{'R',{6,5,6,5,5}},{'S',{7,4,7,1,7}},{'T',{7,2,2,2,2}},
    {'U',{5,5,5,5,7}},{'V',{5,5,5,5,2}},{'W',{5,5,5,7,5}},{'X',{5,5,2,5,5}},{'Y',{5,5,2,2,2}},
    {'Z',{7,1,2,4,7}},{'/',{1,1,2,4,4}},{'-',{0,0,7,0,0}},{':',{0,2,0,2,0}}
};
static void text(float x, float y, float s, const char *str, unsigned int c) {
    for (; *str; str++) {
        for (unsigned int k = 0; k < sizeof(GLY) / sizeof(GLY[0]); k++) {
            if (GLY[k].c == *str) {
                for (int r = 0; r < 5; r++) for (int col = 0; col < 3; col++)
                    if (GLY[k].r[r] & (4 >> col)) rect(x + col * s, y + r * s, s, s, c);
                break;
            }
        }
        x += 4 * s;
    }
}
static void text_c(float y, float s, const char *str, unsigned int c) {
    float w = 4.0f * s * (float)strlen(str) - s;
    text((SCR_W - w) * 0.5f, y, s, str, c);
}

static void gun_model(float extraZ) {
    float fa = fireAnim;
    ScePspFVector3 t = {0.16f + sinf(bob) * 0.010f, -0.12f - fabsf(cosf(bob)) * 0.012f, -0.45f + fa * 0.06f + extraZ};
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumTranslate(&t);
    sceGumRotateX(fa * 0.12f);
    sceGumRotateY(-0.05f);
    sceGumUpdateMatrix();
}

/* ---------------------------------------------------------- render */
static void render(void) {
    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(th->fog);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(68.0f, (float)SCR_W / (float)SCR_H, 0.1f, 30.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumRotateY(yaw);
    ScePspFVector3 cam = {-px, -(0.5f + fabsf(cosf(bob)) * 0.012f), -pz};
    sceGumTranslate(&cam);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumUpdateMatrix();

    sceGuEnable(GU_DEPTH_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuFog(1.5f, 13.0f, th->fog);
    sceGuEnable(GU_FOG);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    bind(th->wa); sceGuDrawArray(GU_TRIANGLES, VT, nA, 0, wA);
    bind(th->wb); sceGuDrawArray(GU_TRIANGLES, VT, nB, 0, wB);
    bind(th->fl); sceGuDrawArray(GU_TRIANGLES, VT, nFloor, 0, wFloor);
    bind(th->ce); sceGuDrawArray(GU_TRIANGLES, VT, nCeil, 0, wCeil);

    /* sprite'lar: esyalar ve dusmanlar (2D karakterler) */
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0x20, 0xff);
    for (int i = 0; i < nit; i++) {
        if (!it[i].active) continue;
        bind(it[i].type == 0 ? texMed : texAmmo);
        sprite(it[i].x, it[i].z, 0.45f, 0.45f, lit(it[i].x, 0.3f, it[i].z, 1.0f));
    }
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        if (e->state == 2) continue;
        bind(e->type == 0 ? texZombie : (e->type == 1 ? texImp : texBoss));
        float w = (e->type == 2) ? 1.25f : 0.75f;
        float h = (e->type == 2) ? 1.40f : 0.90f;
        if (e->state == 1) { h *= 1.0f - e->dead / 0.6f; if (h < 0.02f) h = 0.02f; }
        unsigned int col = (e->hurt > 0) ? RGBA(255, 90, 90, 255) : lit(e->x, 0.5f, e->z, 1.1f);
        sprite(e->x, e->z, w, h, col);
    }

    /* 3D silah ve eller */
    sceGuDisable(GU_FOG);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuClear(GU_DEPTH_BUFFER_BIT);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    float fa = fireAnim, pump = 0;
    if (fa < 0.8f && fa > 0.15f) pump = sinf((0.8f - fa) / 0.65f * PI) * 0.12f;

    gun_model(0);
    bind(texMetal); sceGuDrawArray(GU_TRIANGLES, VT, gA, 0, gunA);
    bind(texSkin);  sceGuDrawArray(GU_TRIANGLES, VT, gB, 0, gunB);
    gun_model(pump);
    bind(texMetal); sceGuDrawArray(GU_TRIANGLES, VT, gC, 0, gunC);
    bind(texSkin);  sceGuDrawArray(GU_TRIANGLES, VT, gD, 0, gunD);

    if (flash > 0) {
        gun_model(0);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuEnable(GU_BLEND);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xffffffff);
        bind(texFlash);
        Vertex *v = (Vertex *)sceGuGetMemory(6 * sizeof(Vertex));
        unsigned int w = 0xffffffff;
        v[0] = (Vertex){0, 1, w, -0.20f, -0.17f, -0.66f};
        v[1] = (Vertex){1, 1, w,  0.20f, -0.17f, -0.66f};
        v[2] = (Vertex){1, 0, w,  0.20f,  0.23f, -0.66f};
        v[3] = v[0]; v[4] = v[2];
        v[5] = (Vertex){0, 0, w, -0.20f,  0.23f, -0.66f};
        sceGuDrawArray(GU_TRIANGLES, VT, 6, 0, v);
        sceGuDisable(GU_BLEND);
    }

    /* HUD (2D) */
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    for (int i = 1; i <= 6; i++) {
        float w = i * 9.0f, hh = w * 0.6f;
        unsigned int c = RGBA(0, 0, 0, 20);
        rect(0, 0, w, SCR_H, c); rect(SCR_W - w, 0, w, SCR_H, c);
        rect(0, 0, SCR_W, hh, c); rect(0, SCR_H - hh, SCR_W, hh, c);
    }
    if (dmgf > 0) rect(0, 0, SCR_W, SCR_H, RGBA(200, 0, 0, (int)(dmgf / 0.35f * 110)));
    if (healf > 0) rect(0, 0, SCR_W, SCR_H, RGBA(0, 200, 90, (int)(healf / 0.3f * 70)));
    if (flash > 0) {
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xffffffff);
        rect(0, 0, SCR_W, SCR_H, RGBA(255, 170, 60, (int)(flash / 0.08f * 50)));
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    }

    unsigned int ch = RGBA(120, 255, 140, 170);
    rect(239, 126, 2, 6, ch); rect(239, 140, 2, 6, ch); rect(226, 135, 6, 2, ch); rect(248, 135, 6, 2, ch);

    rect(0, 238, SCR_W, 34, RGBA(10, 10, 14, 210));
    char buf[24];
    rect(14, 244, 6, 20, RGBA(220, 30, 30, 255)); rect(7, 251, 20, 6, RGBA(220, 30, 30, 255));
    snprintf(buf, sizeof(buf), "%d", hp);
    text(36, 245, 4, buf, RGBA(255, 255, 255, 255));
    rect(104, 250, 70, 9, RGBA(60, 10, 10, 255));
    rect(104, 250, 70.0f * (float)hp / 100.0f, 9, RGBA(220, 40, 40, 255));

    rect(214, 244, 9, 20, RGBA(210, 50, 35, 255)); rect(214, 244, 9, 6, RGBA(230, 190, 60, 255));
    snprintf(buf, sizeof(buf), "%d", ammo);
    text(232, 245, 4, buf, RGBA(255, 220, 90, 255));

    rect(350, 245, 18, 14, RGBA(200, 200, 200, 255)); rect(354, 256, 10, 6, RGBA(200, 200, 200, 255));
    rect(353, 249, 4, 4, RGBA(20, 20, 20, 255)); rect(361, 249, 4, 4, RGBA(20, 20, 20, 255));
    snprintf(buf, sizeof(buf), "%d/%d", kills, total);
    text(376, 245, 4, buf, RGBA(255, 255, 255, 255));

    if (gstate == 0 && levelT > 0) {
        char lv[24];
        snprintf(lv, sizeof(lv), "LEVEL %d", curLevel + 1);
        float oy = 0;
        if (curLevel == 0) { text_c(8, 10, "MOOM", RGBA(255, 60, 40, 255)); oy = 62; }
        text_c(30 + oy, 4, lv, RGBA(255, 220, 90, 255));
        text_c(56 + oy, 7, LV[curLevel].name, RGBA(255, 255, 255, 255));
    }

    if (gstate == 1) {
        rect(0, 0, SCR_W, SCR_H, RGBA(120, 0, 0, 150));
        text_c(100, 8, "YOU DIED", RGBA(255, 255, 255, 255));
        if (((int)(tsec * 2.0f)) & 1) text_c(170, 5, "START", RGBA(255, 220, 90, 255));
    } else if (gstate == 2) {
        rect(0, 0, SCR_W, SCR_H, RGBA(0, 90, 30, 130));
        if (curLevel + 1 < NLEV) text_c(100, 7, "LEVEL CLEAR", RGBA(255, 255, 255, 255));
        else text_c(100, 8, "YOU WIN", RGBA(255, 255, 255, 255));
        if (((int)(tsec * 2.0f)) & 1) text_c(170, 5, "START", RGBA(255, 220, 90, 255));
    }

    sceGuFinish();
    sceGuSync(0, 0);
}

/* ---------------------------------------------------------- GU kurulum */
static void init_gu(void) {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_W, SCR_H, (void *)0x88000, BUF_WIDTH);
    sceGuDepthBuffer((void *)0x110000, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_W / 2), 2048 - (SCR_H / 2));
    sceGuViewport(2048, 2048, SCR_W, SCR_H);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_W, SCR_H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main(int argc, char *argv[]) {
    setup_callbacks();
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    memset(&pad, 0, sizeof(pad));
    memset(&padp, 0, sizeof(padp));
    pad.Lx = pad.Ly = 128;

    make_textures();
    build_gun();
    init_gu();
    load_level(0, 0);

    while (1) {
        sceCtrlPeekBufferPositive(&pad, 1);
        update(1.0f / 60.0f);
        render();
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
        padp = pad;
    }
    return 0;
}
