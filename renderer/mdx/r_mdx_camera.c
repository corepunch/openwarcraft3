#include "r_mdx.h"
#include "../r_local.h"

bool R_GetModelCameraMatrix(mdxModel_t const *model, float aspect, LPMATRIX4 output, LPVECTOR3 root) {
    if (!model || !model->cameras) {
        return false;
    }

    mdxCamera_t const *camera = model->cameras;
    MATRIX4 projection, view;
    VECTOR3 dir = Vector3_sub(&camera->targetPivot, &camera->pivot);
    float fov_deg = camera->fieldOfView * (180.0f / (float)M_PI);
    float near_clip = camera->nearClip;
    float far_clip = camera->farClip;

    if (!isfinite(fov_deg) || fov_deg <= 1.0f || fov_deg >= 179.0f) {
        fov_deg = 35.0f;
    }
    if (!isfinite(near_clip) || near_clip < 0.01f) {
        near_clip = 1.0f;
    }
    if (!isfinite(far_clip) || far_clip <= near_clip + 1.0f) {
        far_clip = near_clip + 5000.0f;
    }
    if (!isfinite(aspect) || aspect <= 0.0f) {
        aspect = 1.0f;
    }
    if (Vector3_len(&dir) < 0.001f) {
        return false;
    }

    Matrix4_perspective(&projection, fov_deg, aspect, near_clip, far_clip);
    Matrix4_lookAt(&view, &camera->pivot, &dir, &(VECTOR3){0,0,1});
    Matrix4_multiply(&projection, &view, output);
    *root = camera->targetPivot;
    return true;
}

static void R_GetBoundsForUICamera(mdxModel_t const *model,
                                   LPVECTOR3 center,
                                   float *width,
                                   float *height,
                                   float *radius)
{
    *center = (VECTOR3){ 0, 0, 0 };
    *width = 0.0f;
    *height = 0.0f;
    *radius = 128.0f;

    if (!model) {
        return;
    }

    BOX3 const *box = &model->bounds.box;
    float depth = fabsf(box->max.y - box->min.y);
    *width = fabsf(box->max.x - box->min.x);
    *height = fabsf(box->max.z - box->min.z);
    float extent = MAX(*width, MAX(depth, *height));
    center->x = (box->min.x + box->max.x) * 0.5f;
    center->y = (box->min.y + box->max.y) * 0.5f;
    center->z = (box->min.z + box->max.z) * 0.5f;
    if (extent > 0.001f) {
        *radius = extent * 0.5f;
    }
}

static void R_GetSpriteOrthoCameraMatrix(mdxModel_t const *model, float aspect, LPMATRIX4 output, LPVECTOR3 root) {
    MATRIX4 projection, view;
    VECTOR3 center = { 0, 0, 0 };
    float width = 0.0f;
    float height = 0.0f;
    float radius = 128.0f;
    float fit_aspect = MAX(0.001f, aspect);
    float fit_half_w;
    float fit_half_h;
    float half_height;
    float half_width;
    VECTOR3 eye;
    VECTOR3 dir = { 0, 0, -1 };

    R_GetBoundsForUICamera(model, &center, &width, &height, &radius);

    fit_half_w = MAX(0.15f, width * 0.5f);
    fit_half_h = MAX(0.15f, height * 0.5f);
    half_height = MAX(fit_half_h, fit_half_w / fit_aspect);
    half_width = half_height * fit_aspect;
    if (width <= 0.001f) {
        half_width = MAX(0.15f, radius * 2.0f * fit_aspect);
        half_height = MAX(0.15f, radius * 2.0f);
    }

    eye = (VECTOR3) {
        center.x,
        center.y,
        center.z + 1.0f,
    };
    Matrix4_ortho(&projection, -half_width, half_width, -half_height, half_height, 0.1f, 100.0f);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){0,1,0});
    Matrix4_multiply(&projection, &view, output);
    *root = center;
}

