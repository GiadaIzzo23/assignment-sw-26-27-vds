// Thin wrapper around SDL2 + SDL2_ttf: the pages never touch SDL directly,
// they only ask for rectangles and text. Drawing happens on a fixed logical
// canvas, so the layout does not depend on the window size.
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <map>
#include <string>

namespace vds {

struct Color {
    Uint8 r, g, b, a = 255;
};

class Gfx {
public:
    Gfx() = default;
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    // Returns false and fills `error` if the window or the font cannot be created.
    bool init(const std::string& font_path, const char* title, int width, int height,
              std::string& error);

    void beginFrame(Color background);
    void endFrame();

    void fillRect(int x, int y, int w, int h, Color c);
    void outlineRect(int x, int y, int w, int h, Color c);
    void text(int x, int y, const std::string& s, int size, Color c);
    int textWidth(const std::string& s, int size);

private:
    TTF_Font* font(int size);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::string font_path_;
    std::map<int, TTF_Font*> fonts_;  // one opened font per pixel size
};

}  // namespace vds
