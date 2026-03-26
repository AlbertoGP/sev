#include <SDL3_image/SDL_image.h>
#include <string.h>

#include "renderer.h"

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static int NUM_CIRCLE_SEGMENTS = 16;

/* ── Sized font cache ─────────────────────────────────────────────────
 * Fonts opened at specific sizes for rendering. Never TTF_SetFontSize'd,
 * so TTF_Text objects created from them keep their cached layout.
 */
#define RENDER_FONT_CACHE_SIZE 8

static struct {
    TTF_Font *font;
    uint16_t  font_id;
    float     size;
} render_font_cache[RENDER_FONT_CACHE_SIZE];
static int render_font_cache_count = 0;

static void flush_text_cache_for_font(uint16_t font_id, float size);

TTF_Font *SDL_Clay_GetRenderFont(Clay_SDL3RendererData *rd, uint16_t font_id, float size) {
    for (int i = 0; i < render_font_cache_count; i++) {
        if (render_font_cache[i].font_id == font_id && render_font_cache[i].size == size)
            return render_font_cache[i].font;
    }
    /* Miss — open a new font at the exact size */
    TTF_Font *f = TTF_OpenFont(rd->font_paths[font_id], size);
    if (!f) return rd->fonts[font_id]; /* fallback */

    if (render_font_cache_count < RENDER_FONT_CACHE_SIZE) {
        render_font_cache[render_font_cache_count++] = (typeof(render_font_cache[0])){f, font_id, size};
    } else {
        /* Evict slot 0 — flush dependent text cache entries first */
        flush_text_cache_for_font(render_font_cache[0].font_id, render_font_cache[0].size);
        TTF_CloseFont(render_font_cache[0].font);
        render_font_cache[0] = (typeof(render_font_cache[0])){f, font_id, size};
    }
    return f;
}

/* ── TTF_Text cache (direct-mapped, 256 slots) ───────────────────── */
#define TEXT_CACHE_SIZE 256
#define TEXT_CACHE_MASK (TEXT_CACHE_SIZE - 1)

static struct {
    TTF_Text *text;
    char     *str;
    uint32_t  hash;
    int       len;
    uint16_t  font_id;
    float     font_size;
} text_cache[TEXT_CACHE_SIZE];

/* Destroy all text cache entries created with a given (font_id, size) pair.
 * Must be called before TTF_CloseFont to avoid dangling TTF_Text pointers. */
static void flush_text_cache_for_font(uint16_t font_id, float size) {
    for (int i = 0; i < TEXT_CACHE_SIZE; i++) {
        if (text_cache[i].text &&
            text_cache[i].font_id == font_id &&
            text_cache[i].font_size == size) {
            TTF_DestroyText(text_cache[i].text);
            SDL_free(text_cache[i].str);
            text_cache[i].text = NULL;
            text_cache[i].str = NULL;
        }
    }
}

static uint32_t fnv1a(const char *data, int len, uint16_t font_id, float font_size) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++)
        h = (h ^ (uint8_t)data[i]) * 16777619u;
    h = (h ^ font_id) * 16777619u;
    uint32_t fs;
    memcpy(&fs, &font_size, sizeof(fs));
    h = (h ^ fs) * 16777619u;
    return h;
}

static TTF_Text *get_cached_text(Clay_SDL3RendererData *rd, uint16_t font_id, float font_size, const char *str, int len) {
    uint32_t hash = fnv1a(str, len, font_id, font_size);
    int idx = hash & TEXT_CACHE_MASK;

    /* Hit check */
    if (text_cache[idx].text &&
        text_cache[idx].hash == hash &&
        text_cache[idx].len == len &&
        text_cache[idx].font_id == font_id &&
        text_cache[idx].font_size == font_size &&
        memcmp(text_cache[idx].str, str, len) == 0) {
        return text_cache[idx].text;
    }

    /* Miss — evict old entry. NULL the pointers before calling get_render_font,
     * which may trigger flush_text_cache_for_font and would otherwise double-free. */
    if (text_cache[idx].text) {
        TTF_DestroyText(text_cache[idx].text);
        SDL_free(text_cache[idx].str);
        text_cache[idx].text = NULL;
        text_cache[idx].str = NULL;
    }

    TTF_Font *font = SDL_Clay_GetRenderFont(rd, font_id, font_size);
    TTF_Text *text = TTF_CreateText(rd->textEngine, font, str, len);

    char *str_copy = SDL_malloc(len);
    memcpy(str_copy, str, len);

    text_cache[idx] = (typeof(text_cache[0])){text, str_copy, hash, len, font_id, font_size};
    return text;
}

void SDL_Clay_DestroyTextCache(void) {
    for (int i = 0; i < TEXT_CACHE_SIZE; i++) {
        if (text_cache[i].text) {
            TTF_DestroyText(text_cache[i].text);
            SDL_free(text_cache[i].str);
            text_cache[i].text = NULL;
            text_cache[i].str = NULL;
        }
    }
    for (int i = 0; i < render_font_cache_count; i++) {
        TTF_CloseFont(render_font_cache[i].font);
        render_font_cache[i].font = NULL;
    }
    render_font_cache_count = 0;
}

//all rendering is performed by a single SDL call, avoiding multiple RenderRect + plumbing choice for circles.
static void SDL_Clay_RenderFillRoundedRect(Clay_SDL3RendererData *rendererData, const SDL_FRect rect, const Clay_CornerRadius cornerRadius, const Clay_Color _color) {
    const SDL_FColor color = { _color.r/255, _color.g/255, _color.b/255, _color.a/255 };

    const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;

    // Clamp each corner radius independently
    const float rTL = SDL_min(cornerRadius.topLeft, minRadius);
    const float rTR = SDL_min(cornerRadius.topRight, minRadius);
    const float rBR = SDL_min(cornerRadius.bottomRight, minRadius);
    const float rBL = SDL_min(cornerRadius.bottomLeft, minRadius);

    // Per-corner segment counts
    const int segTL = rTL > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rTL * 2.0f)) : 0;
    const int segTR = rTR > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rTR * 2.0f)) : 0;
    const int segBR = rBR > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rBR * 2.0f)) : 0;
    const int segBL = rBL > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rBL * 2.0f)) : 0;

    // Perimeter points: rounded corners get segments+1 points, sharp corners get 1
    const int perimTL = segTL > 0 ? segTL + 1 : 1;
    const int perimTR = segTR > 0 ? segTR + 1 : 1;
    const int perimBR = segBR > 0 ? segBR + 1 : 1;
    const int perimBL = segBL > 0 ? segBL + 1 : 1;
    const int perimCount = perimTL + perimTR + perimBR + perimBL;

    // 1 center vertex + perimeter vertices
    const int totalVertices = 1 + perimCount;
    // Triangle fan: one triangle per consecutive perimeter pair (wrapping)
    const int totalIndices = perimCount * 3;

    SDL_Vertex vertices[totalVertices];
    int indices[totalIndices];

    int vertexCount = 0;

    // Vertex 0: center of rect
    vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f}, color, {0.5f, 0.5f} };

    // Corner descriptors: arcCenter, startAngle, radius, segments, sharpPoint
    const struct {
        float cx, cy;       // arc center
        float startAngle;   // start angle in radians
        float radius;
        int segments;
        float sharpX, sharpY; // point if radius == 0
    } corners[4] = {
        { rect.x + rTL,          rect.y + rTL,          SDL_PI_F,           rTL, segTL, rect.x,          rect.y },           // TL: π → 3π/2
        { rect.x + rect.w - rTR, rect.y + rTR,          SDL_PI_F * 1.5f,    rTR, segTR, rect.x + rect.w, rect.y },           // TR: 3π/2 → 2π
        { rect.x + rect.w - rBR, rect.y + rect.h - rBR, 0.0f,               rBR, segBR, rect.x + rect.w, rect.y + rect.h }, // BR: 0 → π/2
        { rect.x + rBL,          rect.y + rect.h - rBL, SDL_PI_F * 0.5f,    rBL, segBL, rect.x,          rect.y + rect.h },  // BL: π/2 → π
    };

    // Emit perimeter vertices clockwise
    for (int c = 0; c < 4; c++) {
        if (corners[c].segments == 0) {
            // Sharp corner: single vertex at the corner point
            vertices[vertexCount++] = (SDL_Vertex){ {corners[c].sharpX, corners[c].sharpY}, color, {0, 0} };
        } else {
            const float step = (SDL_PI_F / 2.0f) / corners[c].segments;
            for (int i = 0; i <= corners[c].segments; i++) {
                const float angle = corners[c].startAngle + (float)i * step;
                vertices[vertexCount++] = (SDL_Vertex){
                    {corners[c].cx + SDL_cosf(angle) * corners[c].radius,
                     corners[c].cy + SDL_sinf(angle) * corners[c].radius},
                    color, {0, 0}
                };
            }
        }
    }

    // Build triangle fan indices: (0, i, i+1) for each consecutive perimeter pair
    int indexCount = 0;
    for (int i = 1; i < perimCount; i++) {
        indices[indexCount++] = 0;
        indices[indexCount++] = i;
        indices[indexCount++] = i + 1;
    }
    // Wrap: last perimeter vertex back to first
    indices[indexCount++] = 0;
    indices[indexCount++] = perimCount;
    indices[indexCount++] = 1;

    SDL_RenderGeometry(rendererData->renderer, NULL, vertices, vertexCount, indices, indexCount);

    // Alpha-feathered corona: 1px wide strip around each arc that fades to transparent,
    // giving smooth sub-pixel edges without requiring shaders.
    SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
    const SDL_FColor color_fade = { color.r, color.g, color.b, 0.0f };

    for (int c = 0; c < 4; c++) {
        if (corners[c].segments == 0) continue;
        const int segs = corners[c].segments;
        const int fv_count = (segs + 1) * 2;
        const int fi_count = segs * 6;
        SDL_Vertex fv[fv_count];
        int        fi[fi_count];

        const float step = (SDL_PI_F / 2.0f) / segs;
        for (int i = 0; i <= segs; i++) {
            const float angle = corners[c].startAngle + (float)i * step;
            const float r     = corners[c].radius;
            const float cx    = corners[c].cx;
            const float cy    = corners[c].cy;
            // Inner vertex at fill edge — full opacity
            fv[i * 2]     = (SDL_Vertex){ { cx + SDL_cosf(angle) * r,         cy + SDL_sinf(angle) * r         }, color,      {0,0} };
            // Outer vertex 1px further — transparent
            fv[i * 2 + 1] = (SDL_Vertex){ { cx + SDL_cosf(angle) * (r + 1.0f), cy + SDL_sinf(angle) * (r + 1.0f) }, color_fade, {0,0} };
        }
        int idx = 0;
        for (int i = 0; i < segs; i++) {
            fi[idx++] = i*2;     fi[idx++] = i*2+1;   fi[idx++] = (i+1)*2;
            fi[idx++] = i*2+1;   fi[idx++] = (i+1)*2+1; fi[idx++] = (i+1)*2;
        }
        SDL_RenderGeometry(rendererData->renderer, NULL, fv, fv_count, fi, fi_count);
    }

    // Feather straight edge sections between adjacent rounded corners.
    // Only applied when both flanking corners are rounded so the outer vertex
    // of this strip coincides exactly with the terminal outer vertex of each arc corona.
    // (Edges adjacent to a sharp corner are left hard to avoid bleeding into neighbours.)
    const struct { float x0,y0, x1,y1, nx,ny; int active; } edges[4] = {
        { rect.x + rTL, rect.y,          rect.x + rect.w - rTR, rect.y,           0,-1, rTL>0 && rTR>0 }, // top
        { rect.x+rect.w, rect.y+rTR,     rect.x+rect.w, rect.y+rect.h-rBR,        1, 0, rTR>0 && rBR>0 }, // right
        { rect.x+rect.w-rBR, rect.y+rect.h, rect.x+rBL, rect.y+rect.h,           0, 1, rBR>0 && rBL>0 }, // bottom
        { rect.x, rect.y+rect.h-rBL,     rect.x, rect.y+rTL,                     -1, 0, rBL>0 && rTL>0 }, // left
    };
    for (int e = 0; e < 4; e++) {
        if (!edges[e].active) continue;
        if (edges[e].x0 == edges[e].x1 && edges[e].y0 == edges[e].y1) continue;
        SDL_Vertex ev[4] = {
            { { edges[e].x0,              edges[e].y0              }, color,      {0,0} },
            { { edges[e].x0 + edges[e].nx, edges[e].y0 + edges[e].ny }, color_fade, {0,0} },
            { { edges[e].x1,              edges[e].y1              }, color,      {0,0} },
            { { edges[e].x1 + edges[e].nx, edges[e].y1 + edges[e].ny }, color_fade, {0,0} },
        };
        int ei[6] = { 0,1,2, 1,3,2 };
        SDL_RenderGeometry(rendererData->renderer, NULL, ev, 4, ei, 6);
    }
}

static void SDL_Clay_RenderArc(Clay_SDL3RendererData *rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness, const Clay_Color color) {
    SDL_SetRenderDrawColor(rendererData->renderer, color.r, color.g, color.b, color.a);

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd = endAngle * (SDL_PI_F / 180.0f);

    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(radius * 1.5f)); //increase circle segments for larger circles, 1.5 is arbitrary.

    const float angleStep = (radEnd - radStart) / (float)numCircleSegments;
    const float thicknessStep = 0.4f; //arbitrary value to avoid overlapping lines. Changing THICKNESS_STEP or numCircleSegments might cause artifacts.

    for (float t = thicknessStep; t < thickness - thicknessStep; t += thicknessStep) {
        SDL_FPoint points[numCircleSegments + 1];
        const float clampedRadius = SDL_max(radius - t, 1.0f);

        for (int i = 0; i <= numCircleSegments; i++) {
            const float angle = radStart + i * angleStep;
            points[i] = (SDL_FPoint){
                    center.x + SDL_cosf(angle) * clampedRadius,
                    center.y + SDL_sinf(angle) * clampedRadius };
        }
        SDL_RenderLines(rendererData->renderer, points, numCircleSegments + 1);
    }
}


SDL_Rect currentClippingRectangle;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands)
{
    for (int32_t i = 0; i < rcommands->length; i++) {
        Clay_RenderCommand *rcmd = Clay_RenderCommandArray_Get(rcommands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect = { (int)bounding_box.x, (int)bounding_box.y, (int)bounding_box.width, (int)bounding_box.height };

        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData *config = &rcmd->renderData.rectangle;
                SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(rendererData->renderer, config->backgroundColor.r, config->backgroundColor.g, config->backgroundColor.b, config->backgroundColor.a);
                if (config->cornerRadius.topLeft > 0 || config->cornerRadius.topRight > 0 ||
                    config->cornerRadius.bottomLeft > 0 || config->cornerRadius.bottomRight > 0) {
                    SDL_Clay_RenderFillRoundedRect(rendererData, rect, config->cornerRadius, config->backgroundColor);
                } else {
                    SDL_RenderFillRect(rendererData->renderer, &rect);
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData *config = &rcmd->renderData.text;
                TTF_Text *text = get_cached_text(rendererData, config->fontId, config->fontSize,
                                                 config->stringContents.chars, config->stringContents.length);
                if (text) {
                    TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
                    TTF_DrawRendererText(text, rect.x, rect.y);
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData *config = &rcmd->renderData.border;

                const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
                const Clay_CornerRadius clampedRadii = {
                    .topLeft = SDL_min(config->cornerRadius.topLeft, minRadius),
                    .topRight = SDL_min(config->cornerRadius.topRight, minRadius),
                    .bottomLeft = SDL_min(config->cornerRadius.bottomLeft, minRadius),
                    .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius)
                };
                //edges
                SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
                if (config->width.left > 0) {
                    const float starting_y = rect.y + clampedRadii.topLeft;
                    const float length = rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;
                    SDL_FRect line = { rect.x - 1, starting_y, config->width.left, length };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.right > 0) {
                    const float starting_x = rect.x + rect.w - (float)config->width.right + 1;
                    const float starting_y = rect.y + clampedRadii.topRight;
                    const float length = rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
                    SDL_FRect line = { starting_x, starting_y, config->width.right, length };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.top > 0) {
                    const float starting_x = rect.x + clampedRadii.topLeft;
                    const float length = rect.w - clampedRadii.topLeft - clampedRadii.topRight;
                    SDL_FRect line = { starting_x, rect.y - 1, length, config->width.top };
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                if (config->width.bottom > 0) {
                    const float starting_x = rect.x + clampedRadii.bottomLeft;
                    const float starting_y = rect.y + rect.h - (float)config->width.bottom + 1;
                    const float length = rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;
                    SDL_FRect line = { starting_x, starting_y, length, config->width.bottom };
                    SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
                    SDL_RenderFillRect(rendererData->renderer, &line);
                }
                //corners
                if (config->cornerRadius.topLeft > 0) {
                    const float centerX = rect.x + clampedRadii.topLeft -1;
                    const float centerY = rect.y + clampedRadii.topLeft - 1;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topLeft,
                        180.0f, 270.0f, config->width.top, config->color);
                }
                if (config->cornerRadius.topRight > 0) {
                    const float centerX = rect.x + rect.w - clampedRadii.topRight;
                    const float centerY = rect.y + clampedRadii.topRight - 1;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topRight,
                        270.0f, 360.0f, config->width.top, config->color);
                }
                if (config->cornerRadius.bottomLeft > 0) {
                    const float centerX = rect.x + clampedRadii.bottomLeft -1;
                    const float centerY = rect.y + rect.h - clampedRadii.bottomLeft;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomLeft,
                        90.0f, 180.0f, config->width.bottom, config->color);
                }
                if (config->cornerRadius.bottomRight > 0) {
                    const float centerX = rect.x + rect.w - clampedRadii.bottomRight;
                    const float centerY = rect.y + rect.h - clampedRadii.bottomRight;
                    SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomRight,
                        0.0f, 90.0f, config->width.bottom, config->color);
                }

            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_BoundingBox boundingBox = rcmd->boundingBox;
                currentClippingRectangle = (SDL_Rect) {
                        .x = boundingBox.x,
                        .y = boundingBox.y,
                        .w = boundingBox.width,
                        .h = boundingBox.height,
                };
                SDL_SetRenderClipRect(rendererData->renderer, &currentClippingRectangle);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                SDL_SetRenderClipRect(rendererData->renderer, NULL);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                SDL_Texture *texture = (SDL_Texture *)rcmd->renderData.image.imageData;
                const SDL_FRect dest = { rect.x, rect.y, rect.w, rect.h };
                SDL_RenderTexture(rendererData->renderer, texture, NULL, &dest);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                CursorRenderData *d = (CursorRenderData *)rcmd->renderData.custom.customData;
                if (!d) break;

                // Save current SDL clip state
                SDL_Rect old_clip;
                bool was_clipped = SDL_RenderClipEnabled(rendererData->renderer);
                SDL_GetRenderClipRect(rendererData->renderer, &old_clip);

                // Intersect provided clip rect with existing clip
                SDL_Rect new_clip = {
                    (int)d->clip_x, (int)d->clip_y,
                    (int)d->clip_w, (int)d->clip_h
                };
                if (was_clipped) {
                    // Intersect
                    int x1 = SDL_max(new_clip.x, old_clip.x);
                    int y1 = SDL_max(new_clip.y, old_clip.y);
                    int x2 = SDL_min(new_clip.x + new_clip.w, old_clip.x + old_clip.w);
                    int y2 = SDL_min(new_clip.y + new_clip.h, old_clip.y + old_clip.h);
                    new_clip = (SDL_Rect){ x1, y1, SDL_max(0, x2 - x1), SDL_max(0, y2 - y1) };
                }
                SDL_SetRenderClipRect(rendererData->renderer, &new_clip);

                SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
                float cx = bounding_box.x;
                float cy = bounding_box.y;

                switch (d->cursor_type) {
                case 1: { // CURSOR_HOLLOW
                    float bw = d->border_width;
                    SDL_SetRenderDrawColor(rendererData->renderer,
                        d->border_color.r, d->border_color.g,
                        d->border_color.b, d->border_color.a);
                    SDL_FRect top   = { cx,                   cy,                    d->width, bw };
                    SDL_FRect bot   = { cx,                   cy + d->height - bw,   d->width, bw };
                    SDL_FRect left  = { cx,                   cy,                    bw, d->height };
                    SDL_FRect right = { cx + d->width - bw,   cy,                    bw, d->height };
                    SDL_RenderFillRect(rendererData->renderer, &top);
                    SDL_RenderFillRect(rendererData->renderer, &bot);
                    SDL_RenderFillRect(rendererData->renderer, &left);
                    SDL_RenderFillRect(rendererData->renderer, &right);
                    break;
                }
                default: { // CURSOR_SOLID (0), CURSOR_THIN (2), CURSOR_UNDER (3)
                    SDL_SetRenderDrawColor(rendererData->renderer,
                        d->bg_color.r, d->bg_color.g,
                        d->bg_color.b, d->bg_color.a);
                    SDL_FRect r = { cx, cy, d->width, d->height };
                    SDL_RenderFillRect(rendererData->renderer, &r);
                    // Character overlay (CURSOR_SOLID only)
                    if (d->cursor_type == 0 && d->char_len > 0) {
                        TTF_Font *font = SDL_Clay_GetRenderFont(rendererData, d->font_id, (float)d->font_size);
                        if (font) {
                            TTF_Text *txt = TTF_CreateText(rendererData->textEngine, font,
                                                           d->char_buf, d->char_len);
                            if (txt) {
                                TTF_SetTextColorFloat(txt,
                                    d->text_color.r / 255.0f,
                                    d->text_color.g / 255.0f,
                                    d->text_color.b / 255.0f,
                                    d->text_color.a / 255.0f);
                                TTF_DrawRendererText(txt, cx, cy);
                                TTF_DestroyText(txt);
                            }
                        }
                    }
                    break;
                }
                }

                // Restore SDL clip
                SDL_SetRenderClipRect(rendererData->renderer, was_clipped ? &old_clip : NULL);
                break;
            }
            default:
                SDL_Log("Unknown render command type: %d", rcmd->commandType);
        }
    }
}

