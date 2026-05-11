#include "r_mdx.h"
#include "../r_local.h"

#define R_UI_BASE_WIDTH  0.8f
#define R_UI_BASE_HEIGHT 0.6f
#define R_UI_MIN_ASPECT  (4.0f / 3.0f)

static BOX3 R_GetRenderableBounds(mdxModel_t const *model) {
    BOX3 bounds = { 0 };
    bool has_bounds = false;

    if (!model) {
        return bounds;
    }

    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        if (geoset->vertices && geoset->num_vertices > 0) {
            FOR_LOOP(i, geoset->num_vertices) {
                VECTOR3 const *v = &geoset->vertices[i];
                if (!isfinite(v->x) || !isfinite(v->y) || !isfinite(v->z)) {
                    continue;
                }
                if (fabsf(v->x) > 100000.0f || fabsf(v->y) > 100000.0f || fabsf(v->z) > 100000.0f) {
                    continue;
                }
                if (!has_bounds) {
                    bounds.min = *v;
                    bounds.max = *v;
                    has_bounds = true;
                } else {
                    bounds.min.x = MIN(bounds.min.x, v->x);
                    bounds.min.y = MIN(bounds.min.y, v->y);
                    bounds.min.z = MIN(bounds.min.z, v->z);
                    bounds.max.x = MAX(bounds.max.x, v->x);
                    bounds.max.y = MAX(bounds.max.y, v->y);
                    bounds.max.z = MAX(bounds.max.z, v->z);
                }
            }
        }
    }

    if (!has_bounds) {
        FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
            BOX3 const *box = &geoset->default_bounds.box;
            if (!has_bounds) {
                bounds = *box;
                has_bounds = true;
            } else {
                bounds.min.x = MIN(bounds.min.x, box->min.x);
                bounds.min.y = MIN(bounds.min.y, box->min.y);
                bounds.min.z = MIN(bounds.min.z, box->min.z);
                bounds.max.x = MAX(bounds.max.x, box->max.x);
                bounds.max.y = MAX(bounds.max.y, box->max.y);
                bounds.max.z = MAX(bounds.max.z, box->max.z);
            }
        }
    }

    if (!has_bounds) {
        bounds = model->bounds.box;
    }

    return bounds;
}

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
                                   float *depth,
                                   float *radius)
{
    *center = (VECTOR3){ 0, 0, 0 };
    *width = 0.0f;
    *height = 0.0f;
    *depth = 0.0f;
    *radius = 128.0f;

    if (!model) {
        return;
    }

    BOX3 const bounds = R_GetRenderableBounds(model);
    BOX3 const *box = &bounds;
    *width = fabsf(box->max.x - box->min.x);
    *height = fabsf(box->max.y - box->min.y);
    *depth = fabsf(box->max.z - box->min.z);
    float extent = MAX(*width, MAX(*height, *depth));
    center->x = (box->min.x + box->max.x) * 0.5f;
    center->y = (box->min.y + box->max.y) * 0.5f;
    center->z = (box->min.z + box->max.z) * 0.5f;
    if (extent > 0.001f) {
        *radius = extent * 0.5f;
    }
}

bool R_IsSpriteUIScreenSpace(mdxModel_t const *model) {
    VECTOR3 center = { 0, 0, 0 };
    float width = 0.0f;
    float height = 0.0f;
    float depth = 0.0f;
    float radius = 0.0f;

    R_GetBoundsForUICamera(model, &center, &width, &height, &depth, &radius);

    if (width <= 0.001f || height <= 0.001f) {
        return false;
    }

    return width <= 2.5f &&
           height <= 2.5f &&
           depth <= 8.0f &&
           center.x > -0.5f && center.x < 1.5f &&
           center.y > -0.5f && center.y < 1.5f &&
           center.z > 32.0f;
}

static void R_GetUISceneExtents(float aspect, float *left, float *right, float *bottom, float *top) {
    (void)aspect;
    *left = 0.0f;
    *right = R_UI_BASE_WIDTH;
    *bottom = 0.0f;
    *top = R_UI_BASE_HEIGHT;
}

void R_GetSpriteOrthoCameraMatrix(mdxModel_t const *model, float aspect, LPMATRIX4 output, LPVECTOR3 root) {
    MATRIX4 projection, view;
    VECTOR3 center = { 0, 0, 0 };
    float width = 0.0f;
    float height = 0.0f;
    float depth = 0.0f;
    float radius = 128.0f;
    float fit_aspect = MAX(0.001f, aspect);
    float fit_half_w;
    float fit_half_h;
    float half_height;
    float half_width;
    float eye_z;
    float far_clip;
    VECTOR3 eye;
    VECTOR3 dir = { 0, 0, -1 };

    R_GetBoundsForUICamera(model, &center, &width, &height, &depth, &radius);

    if (R_IsSpriteUIScreenSpace(model)) {
        float left, right, bottom, top;
        BOX3 const bounds = R_GetRenderableBounds(model);
        float min_z = bounds.min.z;
        float max_z = bounds.max.z;
        float eye_z = max_z + 100.0f;
        float near_clip = 1.0f;
        float far_clip = MAX(128.0f, eye_z - min_z + 100.0f);

        R_GetUISceneExtents(aspect, &left, &right, &bottom, &top);

        Matrix4_ortho(&projection, left, right, bottom, top, near_clip, far_clip);
        Matrix4_lookAt(&view,
                       &(VECTOR3){ 0, 0, eye_z },
                       &(VECTOR3){ 0, 0, -1 },
                       &(VECTOR3){ 0, 1, 0 });
        Matrix4_multiply(&projection, &view, output);
        *root = center;
        return;
    }

    fit_half_w = MAX(0.15f, width * 0.5f);
    fit_half_h = MAX(0.15f, height * 0.5f);
    half_height = MAX(fit_half_h, fit_half_w / fit_aspect);
    half_width = half_height * fit_aspect;
    if (width <= 0.001f || height <= 0.001f) {
        half_width = MAX(0.15f, radius * 2.0f * fit_aspect);
        half_height = MAX(0.15f, radius * 2.0f);
    }

    eye_z = MAX(4.0f, depth + 4.0f);
    far_clip = MAX(32.0f, depth + 16.0f);

    eye = (VECTOR3) {
        center.x,
        center.y,
        center.z + eye_z,
    };
    Matrix4_ortho(&projection, -half_width, half_width, -half_height, half_height, 0.1f, far_clip);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){0,1,0});
    Matrix4_multiply(&projection, &view, output);
    *root = center;
}

