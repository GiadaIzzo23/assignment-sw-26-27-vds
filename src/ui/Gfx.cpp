#include "ui/Gfx.hpp"

namespace vds {

Gfx::~Gfx() {
    for (auto& entry : fonts_) TTF_CloseFont(entry.second);
    if (TTF_WasInit() != 0) TTF_Quit();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
}

bool Gfx::init(const std::string& font_path, const char* title, int width, int height,
               std::string& error) {
    if (TTF_Init() != 0) {
        error = std::string("TTF_Init failed: ") + TTF_GetError();
        return false;
    }
    font_path_ = font_path;

    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                               height, SDL_WINDOW_RESIZABLE);
    if (!window_) {
        error = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
        return false;
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        error = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
        return false;
    }
    SDL_RenderSetLogicalSize(renderer_, width, height);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    if (!font(16)) {
        error = "cannot open font '" + font_path_ + "': " + TTF_GetError();
        return false;
    }
    return true;
}

TTF_Font* Gfx::font(int size) {
    const auto found = fonts_.find(size);
    if (found != fonts_.end()) return found->second;
    TTF_Font* f = TTF_OpenFont(font_path_.c_str(), size);
    if (f) fonts_[size] = f;
    return f;
}

void Gfx::beginFrame(Color bg) {
    SDL_SetRenderDrawColor(renderer_, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(renderer_);
}

void Gfx::endFrame() { SDL_RenderPresent(renderer_); }

void Gfx::fillRect(int x, int y, int w, int h, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    const SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer_, &r);
}

void Gfx::outlineRect(int x, int y, int w, int h, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    const SDL_Rect r{x, y, w, h};
    SDL_RenderDrawRect(renderer_, &r);
}

void Gfx::text(int x, int y, const std::string& s, int size, Color c) {
    if (s.empty()) return;
    TTF_Font* f = font(size);
    if (!f) return;
    SDL_Surface* surface = TTF_RenderUTF8_Blended(f, s.c_str(), SDL_Color{c.r, c.g, c.b, c.a});
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture) {
        const SDL_Rect dst{x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer_, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

int Gfx::textWidth(const std::string& s, int size) {
    TTF_Font* f = font(size);
    int w = 0, h = 0;
    if (f) TTF_SizeUTF8(f, s.c_str(), &w, &h);
    return w;
}

}  // namespace vds
