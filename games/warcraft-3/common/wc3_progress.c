#include "wc3_progress.h"
#include <stdlib.h>
#include <ctype.h>

static BOOL progress_valid(LPCVOID data);

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
static field_t const progress_campaign_fields[] = {
    BZ_SAVE_FIELD(CAMPAIGNSTATE, avail, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, opening, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, ending, F_INT),
    BZ_SAVE_FIELD(CAMPAIGNSTATE, missions, F_INT),
    {0}
};
static field_t const progress_map_fields[] = {
    BZ_SAVE_FIELD(PROGRESSMAP, path, F_STRING),
    BZ_SAVE_FIELD(PROGRESSMAP, flags, F_INT),
    {0}
};
static field_t const progress_fields[] = {
    BZ_SAVE_FIELD(CAMPAIGNPROGRESS, tutorial, F_INT),
    BZ_SAVE_ARRAY(CAMPAIGNPROGRESS, campaigns, BZ_PROGRESS_CAMPAIGNS, progress_campaign_fields),
    BZ_SAVE_COUNTED(CAMPAIGNPROGRESS, maps, BZ_PROGRESS_MAPS, progress_map_fields, count),
    {0}
};
static SAVERECORD const progress_record = {
    .magic = MAKEFOURCC('W','3','P','R'), .version = 1, .size = sizeof(CAMPAIGNPROGRESS), .footer = MAKEFOURCC('W','3','O','K'), .fields = progress_fields, .valid = progress_valid,
};

STATEDEF const progress_def = {
    .key = BZ_PROGRESS_FILE, .life = STATE_PROFILE,
    .record = { .magic = MAKEFOURCC('W','3','P','R'), .version = 2, .size = sizeof(CAMPAIGNPROGRESS), .valid = progress_valid },
    .legacy = &progress_record,
};

/* Disk checksums establish integrity; these bounds establish safe campaign lookups. */
static BOOL progress_valid(LPCVOID data) {
    LPCCAMPAIGNPROGRESS state = data;
    if (state->count > BZ_PROGRESS_MAPS || (unsigned)state->tutorial > PROGRESS_OPEN) goto bad;
    FOR_LOOP(i, BZ_PROGRESS_CAMPAIGNS) {
        LPCCAMPAIGNSTATE camp = state->campaigns + i;
        if ((unsigned)camp->avail > PROGRESS_OPEN || (unsigned)camp->opening > PROGRESS_OPEN ||
            (unsigned)camp->ending > PROGRESS_OPEN) goto bad;
        FOR_LOOP(j, BZ_PROGRESS_MISSIONS) if ((unsigned)camp->missions[j] > PROGRESS_OPEN) goto bad;
    }
    FOR_LOOP(i, state->count)
        if (!memchr(state->maps[i].path, 0, sizeof(PATHSTR)) ||
            state->maps[i].flags & ~(PROGRESS_MAP_PLAYED | PROGRESS_MAP_COMPLETED)) goto bad;
    return true;
bad:
    fprintf(stderr, "WC3 progress: invalid profile values\n");
    return false;
}

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

/* Mutate the authoritative profile; storage and commit boundaries belong to the engine. */
BOOL progress_update(LPCAMPAIGNPROGRESS state, LPCPROGRESSCHANGE change) {
    BOOL ok = false;
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
    ok = true;
done:
    return ok;
}

#ifdef BZ_TESTS
/* Fixture helpers deliberately bypass live acquisition to exercise disk restart and legacy import. */
SAVERESULT progress_load(LPCSTR path, LPCAMPAIGNPROGRESS state) {
    SAVERESULT result = load_record(path, &progress_def.record, state);
    return result == SAVE_INVALID ? load_record(path, &progress_record, state) : result;
}
BOOL progress_change(LPCSTR path, LPCPROGRESSCHANGE change) {
    LPCAMPAIGNPROGRESS state = calloc(1, sizeof(*state));
    BOOL ok = state && progress_load(path, state) != SAVE_INVALID && progress_update(state, change) &&
        save_record(path, &progress_def.record, state);
    free(state);
    state_reset(); /* Fixture disk edits model a process restart. */
    return ok;
}
#endif
