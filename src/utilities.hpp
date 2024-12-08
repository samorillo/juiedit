#include "../external/SDL2/include/SDL.h"
#include "../external/SDL_ttf/include/SDL_ttf.h"

int roundint(float x) {
    return (int)std::roundf(x);
}

void drawRectangleContour(SDL_Renderer* renderer, int x1, int y1, int x2, int y2) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawLine(renderer, x1, y1, x1, y2);
    SDL_RenderDrawLine(renderer, x1, y2, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x2, y1);
    SDL_RenderDrawLine(renderer, x2, y1, x1, y1);
}

void drawArrow(SDL_Renderer* renderer, float x1, float y1, float x2, float y2) {
    //SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
    int a1 = (int)std::roundf(x1);
    int b1 = (int)std::roundf(y1);
    int a2 = (int)std::roundf(x2);
    int b2 = (int)std::roundf(y2);
    if (a1 == a2 && b1 == b2) return;
    SDL_RenderDrawLine(renderer, a1, b1, a2, b2);
    float u = x1 - x2, v = y1 - y2;
    float tipMagnitude = 10.f;
    float factor = tipMagnitude / std::sqrt(u*u + v*v);
    u *= factor; v *= factor;
    
    static float alpha = 2*3.14159f * (24.f/360.f);
    static float c = std::cos(alpha);
    static float s = std::sin(alpha);
    
    float r1 = c*u + s*v, s1 = -s*u + c*v;
    float r2 = c*u - s*v, s2 = s*u + c*v;
    //SDL_RenderDrawLineF(renderer, x2, y2, x2+r1, y2+s1);
    //SDL_RenderDrawLineF(renderer, x2, y2, x2+r2, y2+s2);
    SDL_RenderDrawLine(renderer, (int)std::roundf(x2), (int)std::roundf(y2), (int)std::roundf(x2+r1), (int)std::roundf(y2+s1));
    SDL_RenderDrawLine(renderer, (int)std::roundf(x2), (int)std::roundf(y2), (int)std::roundf(x2+r2), (int)std::roundf(y2+s2));
}

void renderTextCentered(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Rect rect) { // // GPT-generated
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, (SDL_Color){255, 255, 255, 255});
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    int textWidth, textHeight;
    SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);

    int centerX = rect.x + (rect.w - textWidth) / 2;
    int centerY = rect.y + (rect.h - textHeight) / 2;

    SDL_Rect textRect = { centerX, centerY, textWidth, textHeight };

    SDL_RenderCopy(renderer, texture, NULL, &textRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}