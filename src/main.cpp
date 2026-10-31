// Windows 9 - PS4 Pro Edition
// Milestone 1: desktop shell test (software rendering, no fake features)
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const int W = 1920;
static const int H = 1080;
static const int BAR_H = 56;
static const int TITLE_H = 44;
static const int MENU_W = 360;
static const int MENU_H = 144;

static SDL_Renderer* R = nullptr;
static float gAlpha = 1.0f;

// ---------- drawing helpers ----------
static void fillRect(int x, int y, int w, int h, int r, int g, int b, int a = 255)
{
    int fa = (int)(a * gAlpha);
    if (fa < 0) fa = 0;
    if (fa > 255) fa = 255;
    if (fa == 0) return;
    SDL_SetRenderDrawBlendMode(R, fa >= 255 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(R, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)fa);
    SDL_Rect rc;
    rc.x = x; rc.y = y; rc.w = w; rc.h = h;
    SDL_RenderFillRect(R, &rc);
}

// 5x7 font: A-Z, 0-9, : . - /   (rows, 5 bits each)
static const char* FONT_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:.-/";
static const uint8_t FONT[40][7] = {
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x11,0x19,0x15,0x13,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x11,0x0A,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
    {0x00,0x04,0x00,0x00,0x00,0x04,0x00}, // :
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}, // .
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10}, // /
};

static void drawText(int x, int y, const char* s, int scale, int r, int g, int b, int a = 255)
{
    for (; *s; s++)
    {
        char c = *s;
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (c != ' ')
        {
            const char* p = strchr(FONT_CHARS, c);
            if (p)
            {
                int gi = (int)(p - FONT_CHARS);
                for (int row = 0; row < 7; row++)
                    for (int col = 0; col < 5; col++)
                        if ((FONT[gi][row] >> (4 - col)) & 1)
                            fillRect(x + col * scale, y + row * scale, scale, scale, r, g, b, a);
            }
        }
        x += 6 * scale;
    }
}

static bool inRect(int px, int py, int x, int y, int w, int h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

static float ease(float t)
{
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

// ---------- windows ----------
struct Win
{
    const char* title;
    int x, y, w, h;
    int state;   // 0 closed, 1 opening, 2 open, 3 closing
    float t;     // animation 0..1
    bool drag;
    int dx, dy;
};

static const int NW = 2;
static Win wins[NW] = {
    { "DIAGNOSTICS", 420, 120, 760, 520, 0, 0.0f, false, 0, 0 },
    { "ABOUT",       700, 260, 760, 360, 0, 0.0f, false, 0, 0 },
};
static int zorder[NW] = { 0, 1 };

static void bringFront(int i)
{
    int tmp[NW];
    int n = 0;
    for (int k = 0; k < NW; k++)
        if (zorder[k] != i) tmp[n++] = zorder[k];
    tmp[n++] = i;
    for (int k = 0; k < NW; k++) zorder[k] = tmp[k];
}

static void openWin(int i)
{
    if (wins[i].state == 0 || wins[i].state == 3) wins[i].state = 1;
    bringFront(i);
}

// ---------- state ----------
static int mx = W / 2, my = H / 2;
static bool mouseDown = false;
static int joyDown = 0;
static int lastButton = -1;
static int mouseEvents = 0;
static int fps = 0;
static int bootCount = 0;
static bool saveOk = false;
static SDL_Joystick* pad = nullptr;
static bool menuOpen = false;
static float menuT = 0.0f;

static void loadSave()
{
    const char* path = "/data/windows9_state.txt";
    int n = 0;
    FILE* f = fopen(path, "r");
    if (f)
    {
        if (fscanf(f, "%d", &n) != 1) n = 0;
        fclose(f);
    }
    n++;
    f = fopen(path, "w");
    if (f)
    {
        saveOk = fprintf(f, "%d\n", n) > 0;
        fclose(f);
    }
    bootCount = n;
}

static int padAxis(int i)
{
    if (!pad) return 0;
    return (int)SDL_JoystickGetAxis(pad, i);
}

static void drawWindow(int i, bool top)
{
    Win& w = wins[i];
    float e = ease(w.t);
    float s = 0.88f + 0.12f * e;
    int ww = (int)(w.w * s);
    int hh = (int)(w.h * s);
    int x = w.x + (w.w - ww) / 2;
    int y = w.y + (w.h - hh) / 2;
    gAlpha = e;

    fillRect(x + 8, y + 10, ww, hh, 0, 0, 0, 90);
    fillRect(x, y, ww, hh, 28, 30, 38);
    if (top) fillRect(x, y, ww, TITLE_H, 36, 104, 210);
    else     fillRect(x, y, ww, TITLE_H, 58, 62, 76);
    drawText(x + 14, y + 15, w.title, 2, 255, 255, 255);
    fillRect(x + ww - TITLE_H, y, TITLE_H, TITLE_H, 200, 50, 60);
    drawText(x + ww - TITLE_H + 17, y + 15, "X", 2, 255, 255, 255);

    int tx = x + 20;
    int ty = y + TITLE_H + 20;
    char line[96];
    if (i == 0)
    {
        snprintf(line, sizeof(line), "FPS: %d", fps);
        drawText(tx, ty + 0 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "MOUSE: %d %d", mx, my);
        drawText(tx, ty + 1 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "MOUSE EVENTS: %d", mouseEvents);
        drawText(tx, ty + 2 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "PADS: %d", SDL_NumJoysticks());
        drawText(tx, ty + 3 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "LAST BUTTON: %d", lastButton);
        drawText(tx, ty + 4 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "AXIS 0: %d  AXIS 1: %d", padAxis(0), padAxis(1));
        drawText(tx, ty + 5 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "AXIS 2: %d  AXIS 3: %d", padAxis(2), padAxis(3));
        drawText(tx, ty + 6 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "SAVE FILE: %s", saveOk ? "OK" : "FAIL");
        drawText(tx, ty + 7 * 28, line, 2, 220, 225, 235);
        snprintf(line, sizeof(line), "BOOT COUNT: %d", bootCount);
        drawText(tx, ty + 8 * 28, line, 2, 220, 225, 235);
    }
    else
    {
        drawText(tx, ty + 0 * 28, "WINDOWS 9 - PS4 PRO EDITION", 2, 220, 225, 235);
        drawText(tx, ty + 1 * 28, "MILESTONE 1: DESKTOP SHELL TEST", 2, 220, 225, 235);
        drawText(tx, ty + 2 * 28, "ONLY WORKING PARTS ARE SHOWN", 2, 220, 225, 235);
        drawText(tx, ty + 3 * 28, "BUILT WITH OPENORBIS + SDL2", 2, 220, 225, 235);
    }
    gAlpha = 1.0f;
}

int main(int argc, char* args[])
{
    (void)argc;
    (void)args;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
        for (;;) {}

    SDL_Window* window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, 0);
    if (!window)
        for (;;) {}

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    R = SDL_CreateSoftwareRenderer(surface);
    if (!R)
        for (;;) {}

    if (SDL_NumJoysticks() > 0) pad = SDL_JoystickOpen(0);

    loadSave();
    openWin(1);

    uint32_t last = SDL_GetTicks();
    uint32_t fpsStart = last;
    int frames = 0;
    bool prevDown = false;
    bool running = true;

    while (running)
    {
        uint32_t now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last = now;

        frames++;
        if (now - fpsStart >= 1000)
        {
            fps = frames;
            frames = 0;
            fpsStart = now;
        }

        // ----- input -----
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_MOUSEMOTION)
            {
                mx = ev.motion.x;
                my = ev.motion.y;
                mouseEvents++;
            }
            else if (ev.type == SDL_MOUSEBUTTONDOWN)
            {
                if (ev.button.button == SDL_BUTTON_LEFT) mouseDown = true;
                mouseEvents++;
            }
            else if (ev.type == SDL_MOUSEBUTTONUP)
            {
                if (ev.button.button == SDL_BUTTON_LEFT) mouseDown = false;
                mouseEvents++;
            }
            else if (ev.type == SDL_JOYBUTTONDOWN)
            {
                joyDown++;
                lastButton = ev.jbutton.button;
            }
            else if (ev.type == SDL_JOYBUTTONUP)
            {
                if (joyDown > 0) joyDown--;
            }
        }

        if (pad)
        {
            float ax = padAxis(0) / 32768.0f;
            float ay = padAxis(1) / 32768.0f;
            if (ax > -0.15f && ax < 0.15f) ax = 0.0f;
            if (ay > -0.15f && ay < 0.15f) ay = 0.0f;
            mx += (int)(ax * 1100.0f * dt);
            my += (int)(ay * 1100.0f * dt);
        }
        if (mx < 0) mx = 0;
        if (mx > W - 1) mx = W - 1;
        if (my < 0) my = 0;
        if (my > H - 1) my = H - 1;

        bool down = mouseDown || joyDown > 0;
        bool pressed = down && !prevDown;
        bool released = !down && prevDown;
        prevDown = down;

        // ----- click handling -----
        int startX = 8, startY = H - BAR_H + 8, startW = 120, startH = 40;
        int menuX = 8, menuY = H - BAR_H - 8 - MENU_H;

        if (pressed)
        {
            bool consumed = false;

            if (menuOpen)
            {
                for (int k = 0; k < 2; k++)
                {
                    if (inRect(mx, my, menuX + 8, menuY + 8 + k * 64, MENU_W - 16, 56))
                    {
                        openWin(k);
                        consumed = true;
                    }
                }
                if (!inRect(mx, my, startX, startY, startW, startH))
                    menuOpen = false;
            }

            if (!consumed && inRect(mx, my, startX, startY, startW, startH))
            {
                menuOpen = !menuOpen;
                consumed = true;
            }

            if (!consumed)
            {
                int slot = 0;
                for (int k = 0; k < NW; k++)
                {
                    if (wins[k].state == 0) continue;
                    if (inRect(mx, my, 144 + slot * 196, H - BAR_H + 8, 188, 40))
                    {
                        bringFront(k);
                        consumed = true;
                    }
                    slot++;
                }
            }

            if (!consumed)
            {
                for (int z = NW - 1; z >= 0 && !consumed; z--)
                {
                    int k = zorder[z];
                    Win& w = wins[k];
                    if (w.state != 2) continue;
                    if (inRect(mx, my, w.x, w.y, w.w, w.h))
                    {
                        bringFront(k);
                        if (inRect(mx, my, w.x + w.w - TITLE_H, w.y, TITLE_H, TITLE_H))
                            w.state = 3;
                        else if (my < w.y + TITLE_H)
                        {
                            w.drag = true;
                            w.dx = mx - w.x;
                            w.dy = my - w.y;
                        }
                        consumed = true;
                    }
                }
            }

            if (!consumed && inRect(mx, my, 48, 48, 96, 96))
                openWin(0);
        }

        if (released)
            for (int k = 0; k < NW; k++) wins[k].drag = false;

        for (int k = 0; k < NW; k++)
        {
            Win& w = wins[k];
            if (w.drag && down)
            {
                w.x = mx - w.dx;
                w.y = my - w.dy;
                if (w.y < 0) w.y = 0;
                if (w.y > H - BAR_H - TITLE_H) w.y = H - BAR_H - TITLE_H;
            }
            if (w.state == 1)
            {
                w.t += dt / 0.18f;
                if (w.t >= 1.0f) { w.t = 1.0f; w.state = 2; }
            }
            else if (w.state == 3)
            {
                w.t -= dt / 0.14f;
                if (w.t <= 0.0f) { w.t = 0.0f; w.state = 0; w.drag = false; }
            }
        }

        menuT += (menuOpen ? 1.0f : -1.0f) * dt / 0.15f;
        if (menuT < 0.0f) menuT = 0.0f;
        if (menuT > 1.0f) menuT = 1.0f;

        // ----- draw -----
        gAlpha = 1.0f;
        for (int y = 0; y < H; y += 10)
        {
            int c = y * 60 / H;
            fillRect(0, y, W, 10, 14 + c / 2, 18 + c, 34 + c * 2);
        }

        fillRect(48, 48, 96, 96, 60, 140, 255, 210);
        drawText(48 + 16, 48 + 40, "DIAG", 3, 255, 255, 255);

        for (int z = 0; z < NW; z++)
        {
            int k = zorder[z];
            if (wins[k].state != 0)
                drawWindow(k, z == NW - 1);
        }

        // taskbar
        fillRect(0, H - BAR_H, W, BAR_H, 16, 18, 24, 240);
        if (menuOpen) fillRect(startX, startY, startW, startH, 36, 104, 210);
        else          fillRect(startX, startY, startW, startH, 44, 48, 60);
        drawText(startX + 20, startY + 13, "START", 2, 255, 255, 255);

        int slot = 0;
        for (int k = 0; k < NW; k++)
        {
            if (wins[k].state == 0) continue;
            bool isTop = (zorder[NW - 1] == k);
            fillRect(144 + slot * 196, H - BAR_H + 8, 188, 40, isTop ? 52 : 36, isTop ? 58 : 40, isTop ? 76 : 50);
            drawText(144 + slot * 196 + 12, H - BAR_H + 21, wins[k].title, 2, 230, 235, 245);
            slot++;
        }

        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS %d", fps);
        drawText(W - 140, H - BAR_H + 20, fpsText, 2, 150, 200, 255);

        // start menu
        if (menuT > 0.0f)
        {
            float e = ease(menuT);
            gAlpha = e;
            int my0 = menuY + (int)((1.0f - e) * 20.0f);
            fillRect(menuX, my0, MENU_W, MENU_H, 22, 24, 32, 245);
            for (int k = 0; k < 2; k++)
            {
                bool hover = menuOpen && inRect(mx, my, menuX + 8, menuY + 8 + k * 64, MENU_W - 16, 56);
                fillRect(menuX + 8, my0 + 8 + k * 64, MENU_W - 16, 56, hover ? 48 : 34, hover ? 100 : 38, hover ? 190 : 50);
                drawText(menuX + 28, my0 + 8 + k * 64 + 20, wins[k].title, 2, 255, 255, 255);
            }
            gAlpha = 1.0f;
        }

        // cursor
        for (int i = 0; i < 18; i++)
        {
            fillRect(mx, my + i, 1 + (i * 2) / 3, 1, 255, 255, 255);
        }
        fillRect(mx, my, 2, 18, 0, 0, 0);

        SDL_UpdateWindowSurface(window);
    }

    SDL_Quit();
    return 0;
}
