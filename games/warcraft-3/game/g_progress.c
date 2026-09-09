#include "g_local.h"
#include "../common/wc3_progress.h"

/* Profile progress outlives level/JASS state and is committed only by campaign events. */
void G_ProgressChange(LPCPROGRESSCHANGE change) {
    PATHSTR path;
    gi.UserPath(BZ_PROGRESS_FILE, path, sizeof(path));
    if (!progress_change(path, change)) fprintf(stderr, "WC3 progress: could not persist event %u\n", change->kind);
}

void G_ProgressMap(BOOL completed) {
    if (!level.map_path[0] || campaign_domain(level.map_path) == CAMPAIGN_CUSTOM) return;
    PROGRESSCHANGE change = { .kind = completed ? PROGRESS_COMPLETED : PROGRESS_PLAYED, .map = level.map_path };
    G_ProgressChange(&change);
}

/* Mission and campaign natives use offsets; opening/ending cinematics use Blizzard's global indexes. */
void G_ProgressNative(LPJASS j, PROGRESSKIND kind) {
    DWORD offset = (DWORD)jass_checkinteger(j, 1);
    CAMPAIGNDOMAIN domain = level.map_path[0] ? campaign_domain(level.map_path) : CAMPAIGN_CUSTOM;
    /* The loaded campaign owns its namespace even when launched under the other expansion's configuration.
     * Custom maps have no campaign path domain, so their selected runtime supplies the native ABI. */
    BOOL expansion = domain == CAMPAIGN_CUSTOM ? atoi(gi.CvarString("fs_expansion", "0")) != 0 : domain == CAMPAIGN_TFT;
    int index = kind == PROGRESS_CAMPAIGN || kind == PROGRESS_MISSION ?
        campaign_offset(offset, expansion) : (int)offset;
    PROGRESSCHANGE change = {
        .kind = kind, .campaign = (DWORD)index,
        .mission = kind == PROGRESS_MISSION ? (DWORD)jass_checkinteger(j, 2) : 0,
        .available = jass_checkboolean(j, kind == PROGRESS_MISSION ? 3 : 2),
    };
    G_ProgressChange(&change);
}
