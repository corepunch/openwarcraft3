#include "wc3_progress.h"
#include <stdlib.h>
#include <ctype.h>

/* Blizzard.j's bj_CAMPAIGN_INDEX_* and bj_CAMPAIGN_OFFSET_* contracts, matched to CampaignStrings sections. */
static struct { LPCSTR key; int roc, tft; } const campaign_ids[] = {
    { "Tutorial", 0, -1 },
    { "Human", 1, 6 },
    { "Undead", 2, 7 },
    { "Orc", 3, 8 },
    { "NightElf", 4, 5 },
};
static struct { LPCSTR path; CAMPAIGNDOMAIN domain; } const campaign_paths[] = {
    { "maps/campaign/", CAMPAIGN_ROC },
    { "maps/frozenthrone/campaign/", CAMPAIGN_TFT },
};
static SAVEFIELD const progress_campaign_fields[] = {
    BZ_SAVE_FIELD(CAMPAIGNSTATE, avail, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, opening, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, ending, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, missions, F_INT),
    {0}
};
static SAVEFIELD const progress_map_fields[] = {
    BZ_SAVE_FIELD(PROGRESSMAP, path, F_STRING),
    BZ_SAVE_FIELD(PROGRESSMAP, flags, F_INT),
    {0}
};
static SAVEFIELD const progress_fields[] = {
    BZ_SAVE_FIELD(CAMPAIGNPROGRESS, tutorial, F_INT),
    BZ_SAVE_ARRAY(CAMPAIGNPROGRESS, campaigns, BZ_PROGRESS_CAMPAIGNS, progress_campaign_fields),
    BZ_SAVE_COUNTED(CAMPAIGNPROGRESS, maps, BZ_PROGRESS_MAPS, progress_map_fields, count),
    {0}
};
static SAVERECORD const progress_record = {
    .magic = MAKEFOURCC('W','3','P','R'), .version = 1, .size = sizeof(CAMPAIGNPROGRESS), .fields = progress_fields,
};

/* Native campaign/mission offsets are expansion-local; cinematic natives already take global campaign indexes. */
int campaign_offset(DWORD offset, BOOL expansion) {
    return expansion ? (offset < 4 ? (int)offset + 5 : -1) : (offset < 5 ? (int)offset : -1);
}

int campaign_index(LPCSTR key, BOOL expansion) {
    FOR_LOOP(i, sizeof(campaign_ids) / sizeof(*campaign_ids))
        if (!strcasecmp(key, campaign_ids[i].key)) return expansion ? campaign_ids[i].tft : campaign_ids[i].roc;
    fprintf(stderr, "WC3 progress: unknown CampaignStrings section %s\n", key);
    return -1;
}

/* Map keys use the VFS's case/separator equivalence, including direct CLI launches. */
static BOOL progress_map_path(LPCSTR path, LPSTR out) {
    if (!path || !*path || strlen(path) >= sizeof(PATHSTR)) {
        fprintf(stderr, "WC3 progress: invalid map path\n");
        return false;
    }
    do { *out++ = *path == '\\' ? '/' : (char)tolower((unsigned char)*path); } while (*path++);
    return true;
}

CAMPAIGNDOMAIN campaign_domain(LPCSTR path) {
    PATHSTR key;
    if (!progress_map_path(path, key)) return CAMPAIGN_CUSTOM;
    FOR_LOOP(i, sizeof(campaign_paths) / sizeof(*campaign_paths))
        if (!strncmp(key, campaign_paths[i].path, strlen(campaign_paths[i].path))) return campaign_paths[i].domain;
    return CAMPAIGN_CUSTOM;
}

DWORD progress_map_flags(LPCCAMPAIGNPROGRESS state, LPCSTR path) {
    PATHSTR key;
    if (!progress_map_path(path, key)) return 0;
    FOR_LOOP(i, state->count)
        if (!strcmp(key, state->maps[i].path)) return state->maps[i].flags;
    return 0;
}

SAVERESULT progress_load(LPCSTR path, LPCAMPAIGNPROGRESS state) { return load_record(path, &progress_record, state); }

/* Read-modify-commit prevents menu refreshes or map teardown from overwriting newer script-authored progress. */
BOOL progress_change(LPCSTR path, LPCPROGRESSCHANGE change) {
    LPCAMPAIGNPROGRESS state = calloc(1, sizeof(*state));
    if (!state) { fprintf(stderr, "WC3 progress: allocation failed\n"); return false; }
    BOOL ok = false;
    if (progress_load(path, state) == SAVE_INVALID) goto done;
    PROGRESSSTATE *slot = NULL;
    if (change->kind == PROGRESS_TUTORIAL) slot = &state->tutorial;
    else if (change->kind == PROGRESS_PLAYED || change->kind == PROGRESS_COMPLETED) {
        PATHSTR key;
        if (!progress_map_path(change->map, key)) goto done;
        DWORD index = 0;
        while (index < state->count && strcmp(key, state->maps[index].path)) index++;
        if (index == BZ_PROGRESS_MAPS) { fprintf(stderr, "WC3 progress: map history limit reached\n"); goto done; }
        DWORD flags = PROGRESS_MAP_PLAYED | (change->kind == PROGRESS_COMPLETED ? PROGRESS_MAP_COMPLETED : 0);
        if ((state->maps[index].flags & flags) == flags) { ok = true; goto done; }
        if (index == state->count) state->count++;
        strlcpy(state->maps[index].path, key, sizeof(state->maps[index].path));
        state->maps[index].flags |= flags;
    } else {
        if (change->campaign >= BZ_PROGRESS_CAMPAIGNS ||
            (change->kind == PROGRESS_MISSION && change->mission >= BZ_PROGRESS_MISSIONS)) {
            fprintf(stderr, "WC3 progress: invalid campaign/mission %u/%u\n", change->campaign, change->mission);
            goto done;
        }
        LPCAMPAIGNSTATE camp = &state->campaigns[change->campaign];
        switch (change->kind) {
        case PROGRESS_CAMPAIGN: slot = &camp->avail; break;
        case PROGRESS_MISSION: slot = &camp->missions[change->mission]; break;
        case PROGRESS_OPENING: slot = &camp->opening; break;
        case PROGRESS_ENDING: slot = &camp->ending; break;
        default: fprintf(stderr, "WC3 progress: invalid update kind %u\n", change->kind); goto done;
        }
    }
    if (slot) {
        PROGRESSSTATE value = change->available ? PROGRESS_OPEN : PROGRESS_LOCKED;
        if (*slot == value) { ok = true; goto done; }
        *slot = value;
    }
    ok = save_record(path, &progress_record, state);
done:
    free(state);
    return ok;
}
