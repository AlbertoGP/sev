#include <SDL3_image/SDL_image.h>

#include "renderer.h"

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static int NUM_CIRCLE_SEGMENTS = 16;

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
    const int segTL = rTL > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rTL * 0.5f)) : 0;
    const int segTR = rTR > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rTR * 0.5f)) : 0;
    const int segBR = rBR > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rBR * 0.5f)) : 0;
    const int segBL = rBL > 0 ? SDL_max(NUM_CIRCLE_SEGMENTS, (int)(rBL * 0.5f)) : 0;

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
                    SDL_roundf(center.x + SDL_cosf(angle) * clampedRadius),
                    SDL_roundf(center.y + SDL_sinf(angle) * clampedRadius) };
        }
        SDL_RenderLines(rendererData->renderer, points, numCircleSegments + 1);
    }
}

static void SDL_Clay_RenderConcaveFan(Clay_SDL3RendererData *rendererData, const SDL_FRect rect, const Clay_Color _color, CustomRenderType side) {
    const SDL_FColor color = { _color.r/255, _color.g/255, _color.b/255, _color.a/255 };
    const float R = rect.w;
    const float overlap = 1.0f;

    const int segments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(R * 0.5f));

    // fan center + overlap vertex + (segments+1) arc points
    SDL_Vertex vertices[segments + 3];
    int indices[(segments + 1) * 3];

    float cx, cy; // arc center
    float startAngle, endAngle;
    int vertexCount = 0;

    if (side == CUSTOM_RENDER_CONCAVE_LEFT) {
        cx = rect.x;  cy = rect.y;
        startAngle = 0;
        endAngle = SDL_PI_F / 2.0f;

        // Fan center: bottom-right, extended 1px into parent
        vertices[vertexCount++] = (SDL_Vertex){ {rect.x + R + overlap, rect.y + R}, color, {0, 0} };
        // Overlap vertex: top-right, extended 1px into parent
        vertices[vertexCount++] = (SDL_Vertex){ {rect.x + R + overlap, rect.y}, color, {0, 0} };
        // Arc points
        const float step = (endAngle - startAngle) / segments;
        for (int i = 0; i <= segments; i++) {
            const float angle = startAngle + (float)i * step;
            vertices[vertexCount++] = (SDL_Vertex){
                {cx + SDL_cosf(angle) * R, cy + SDL_sinf(angle) * R},
                color, {0, 0}
            };
        }
    } else {
        cx = rect.x + R;  cy = rect.y;
        startAngle = SDL_PI_F / 2.0f;
        endAngle = SDL_PI_F;

        // Fan center: bottom-left, extended 1px into parent
        vertices[vertexCount++] = (SDL_Vertex){ {rect.x - overlap, rect.y + R}, color, {0, 0} };
        // Arc points
        const float step = (endAngle - startAngle) / segments;
        for (int i = 0; i <= segments; i++) {
            const float angle = startAngle + (float)i * step;
            vertices[vertexCount++] = (SDL_Vertex){
                {cx + SDL_cosf(angle) * R, cy + SDL_sinf(angle) * R},
                color, {0, 0}
            };
        }
        // Overlap vertex: top-left, extended 1px into parent
        vertices[vertexCount++] = (SDL_Vertex){ {rect.x - overlap, rect.y}, color, {0, 0} };
    }

    int indexCount = 0;
    for (int i = 1; i < vertexCount - 1; i++) {
        indices[indexCount++] = 0;
        indices[indexCount++] = i;
        indices[indexCount++] = i + 1;
    }

    SDL_RenderGeometry(rendererData->renderer, NULL, vertices, vertexCount, indices, indexCount);
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
                TTF_Font *font = rendererData->fonts[config->fontId];
                TTF_SetFontSize(font, config->fontSize);
                TTF_Text *text = TTF_CreateText(rendererData->textEngine, font, config->stringContents.chars, config->stringContents.length);
                TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
                TTF_DrawRendererText(text, rect.x, rect.y);
                TTF_DestroyText(text);
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
                Clay_CustomRenderData *config = &rcmd->renderData.custom;
                CustomRenderType type = (CustomRenderType)(uintptr_t)config->customData;
                switch (type) {
                    case CUSTOM_RENDER_CONCAVE_LEFT:
                    case CUSTOM_RENDER_CONCAVE_RIGHT:
                        SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
                        SDL_Clay_RenderConcaveFan(rendererData, rect, config->backgroundColor, type);
                        break;
                    default:
                        SDL_Log("Unknown custom render type: %d", type);
                }
            } break;
            default:
                SDL_Log("Unknown render command type: %d", rcmd->commandType);
        }
    }
}

