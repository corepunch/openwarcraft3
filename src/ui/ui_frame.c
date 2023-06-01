#include "ui_local.h"

void SetFramePoint(uiFrameDef_t *frame,
                   uiFramePointType_t framePoint,
                   uiFrameDef_t *targetFrame,
                   uiFramePointType_t targetPoint,
                   float x,
                   float y)
{
    uiFramePoint_t *point = &frame->Points[frame->numPoints++];
    point->type = framePoint;
    point->targetFrame = targetFrame;
    point->targetType = targetPoint;
    point->x = x;
    point->y = y;
}

VECTOR2 GetFramePointValue(uiFrameDef_t *frame,
                           uiFramePointType_t framePoint,
                           float x,
                           float y)
{
    RECT const r = frame->rect;
    switch (framePoint) {
        case FRAMEPOINT_TOPLEFT: return (VECTOR2) { r.x+x, r.y+y };
        case FRAMEPOINT_TOP: return (VECTOR2) { r.x+r.width/2+x, r.y+y };
        case FRAMEPOINT_TOPRIGHT: return (VECTOR2) { r.x+r.width-x, r.y+y };
        case FRAMEPOINT_LEFT: return (VECTOR2) { r.x+x, r.y+r.height/2+y };
        case FRAMEPOINT_CENTER: return (VECTOR2) { r.x+r.width/2+x, r.y+r.height/2+y };
        case FRAMEPOINT_RIGHT: return (VECTOR2) { r.x+r.width-x, r.y+r.height/2+y };
        case FRAMEPOINT_BOTTOMLEFT: return (VECTOR2) { r.x+x, r.y+r.height-y };
        case FRAMEPOINT_BOTTOM: return (VECTOR2) { r.x+r.width/2+x, r.y-y };
        case FRAMEPOINT_BOTTOMRIGHT: return (VECTOR2) { r.x+r.width-x, r.y-y };
    }
}


void GetFramePointRect(uiFrameDef_t const *frame,
                       uiFramePoint_t const *framePoint,
                       RECT *output)
{
    VECTOR2 point =
    GetFramePointValue(framePoint->targetFrame,
                       framePoint->targetType,
                       framePoint->x,
                       framePoint->y);
    switch (framePoint->type) {
        case FRAMEPOINT_CENTER:
            output->x = point.x - frame->Width * 0.5;
            output->y = point.y - frame->Height * 0.5;
            break;
        default:
            break;
    }
}

RECT GetFrameRect(uiFrameDef_t const *frame) {
    RECT rect = { 0, 0, frame->Width, frame->Height };
    FOR_LOOP(p, frame->numPoints) {
        GetFramePointRect(frame, &frame->Points[p], &rect);
    }
    return rect;
}


void UI_FrameAddChild(uiFrameDef_t *frame, uiFrameDef_t *child) {
    PUSH_BACK(uiFrameDef_t, child, frame->children);
}

void UI_DrawFrame(uiFrameDef_t const *frame) {
    if (frame->draw) {
        frame->draw(frame);
    }
    FOR_EACH_LIST(uiFrameDef_t, child, frame->children) {
        UI_DrawFrame(child);
    }
}

void UI_UpdateFrame(uiFrameDef_t *frame) {
    frame->rect = GetFrameRect(frame);
    FOR_EACH_LIST(uiFrameDef_t, child, frame->children) {
        UI_UpdateFrame(child);
    }
}

