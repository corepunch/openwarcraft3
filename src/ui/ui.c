#include "ui.h"
#include "../client/client.h"

uiFrameDef_t *frame1;
configSection_t *war3skins;
configSection_t *skin;
uiPortrait_t portrait;

void UI_Init(void) {
    war3skins = INI_ParseFile("UI\\war3skins.txt");
    skin = INI_FindSection(war3skins, "Default");
    frame1 = FDF_ParseFile("UI\\FrameDef\\UI\\ConsoleUI.fdf");
    memset(&portrait, 0, sizeof(uiPortrait_t));
}

void UI_Shutdown(void) {
}

entityState_t const *UI_GetSelectedEntity(void) {
    FOR_LOOP(i, cl.num_entities) {
        if (cl.ents[i].selected)
            return &cl.ents[i].current;
    }
    return NULL;
}

void UI_DrawPortrait(uiPortrait_t *pt) {
    entityState_t const *selected = UI_GetSelectedEntity();
    RECT viewport = { 215/800.0, 30/600.0, 80/800.0, 80/600.0 };
    if (selected != pt->current) {
        if (selected) {
            LPCSTR string = cl.configstrings[selected->model + CS_MODELS];
            PATHSTR buffer = { 0 };
            PATHSTR path = { 0 };
            memcpy(buffer, string, strstr(string, ".mdx") - string);
            sprintf(path, "%s_Portrait.mdx", buffer);
            pt->portraitModel = re.LoadModel(path);
        }
        pt->current = selected;
    }
    if (pt->portraitModel) {
        re.DrawPortrait(pt->portraitModel, &viewport);
    }
}

void UI_Draw(void) {//uiFrameDef_t const *frame) {
    UI_DrawPortrait(&portrait);
    
    FOR_EACH_LIST(uiFrameDef_t, f, frame1) {
        FOR_EACH_LIST(uiTextureDef_t, t, f->textures) {
            re.DrawImage(t->Texture, &(const RECT) {
                .x = ((t->Anchor.corner & 1) ? 0.8 + t->Anchor.x - t->Width : t->Anchor.x) * UI_SCALE,
                .y = ((t->Anchor.corner & 2) ? 0.6 + t->Anchor.y - t->Height : t->Anchor.y) * UI_SCALE,
                .width = t->Width * UI_SCALE,
                .height = t->Height * UI_SCALE,
            }, &t->TexCoord);
        }
    }
}
