#include "g_local.h"
#include "common/campaign_progress.h"

static wc3CampaignProgress_t campaign_progress;
static BOOL campaign_progress_loaded;

static DWORD G_CampaignProgressEdition(void) {
    LPCSTR expansion = gi.CvarString ? gi.CvarString("fs_expansion", "0") : "0";
    return expansion && atoi(expansion) != 0 ? WC3_CAMPAIGN_EDITION_TFT : WC3_CAMPAIGN_EDITION_ROC;
}

static BOOL G_CampaignProgressPath(LPSTR path, DWORD path_size) {
    if (!path || !path_size) return false;
    path[0] = '\0';
    gi.UserPath(WC3_CAMPAIGN_PROGRESS_FILENAME, path, path_size);
    return path[0] != '\0';
}

static void G_CampaignProgressEnsureLoaded(void) {
    PATHSTR path;

    if (campaign_progress_loaded) return;
    wc3_campaign_progress_init(&campaign_progress);
    if (G_CampaignProgressPath(path, sizeof(path))) {
        wc3_campaign_progress_load(path, &campaign_progress);
    }
    campaign_progress_loaded = true;
}

static BOOL G_CampaignProgressCommit(void) {
    PATHSTR path;

    if (!G_CampaignProgressPath(path, sizeof(path))) {
        fprintf(stderr, "Campaign progress: writable user path is unavailable\n");
        return false;
    }
    return wc3_campaign_progress_save(path, &campaign_progress);
}

void G_CampaignProgressResetRuntime(void) {
    wc3_campaign_progress_init(&campaign_progress);
    campaign_progress_loaded = false;
}

BOOL G_CampaignProgressSetTutorialCleared(BOOL cleared) {
    DWORD const edition = G_CampaignProgressEdition();

    G_CampaignProgressEnsureLoaded();
    if (!wc3_campaign_progress_set_tutorial(&campaign_progress, edition, cleared)) return false;
    return G_CampaignProgressCommit();
}

BOOL G_CampaignProgressSetCampaignAvailable(LONG campaign, BOOL available) {
    DWORD const edition = G_CampaignProgressEdition();
    wc3CampaignProgressKey_t key;

    if (campaign < 0 || campaign >= WC3_CAMPAIGN_PROGRESS_CAMPAIGNS) {
        fprintf(stderr, "Campaign progress: invalid campaign index %ld\n", (long)campaign);
        return false;
    }
    key = MAKE(wc3CampaignProgressKey_t, .edition = edition, .campaign = (DWORD)campaign);
    G_CampaignProgressEnsureLoaded();
    if (!wc3_campaign_progress_set_campaign(&campaign_progress, key, available)) return false;
    return G_CampaignProgressCommit();
}

BOOL G_CampaignProgressSetMissionAvailable(LONG campaign, LONG mission, BOOL available) {
    DWORD const edition = G_CampaignProgressEdition();
    wc3CampaignProgressKey_t key;

    if (campaign < 0 || campaign >= WC3_CAMPAIGN_PROGRESS_CAMPAIGNS ||
        mission < 0 || mission >= WC3_CAMPAIGN_PROGRESS_MISSIONS) {
        fprintf(stderr, "Campaign progress: invalid campaign/mission index %ld/%ld\n",
                (long)campaign, (long)mission);
        return false;
    }
    key = MAKE(wc3CampaignProgressKey_t,
               .edition = edition,
               .campaign = (DWORD)campaign,
               .mission = (DWORD)mission);
    G_CampaignProgressEnsureLoaded();
    if (!wc3_campaign_progress_set_mission(&campaign_progress, key, available)) return false;
    return G_CampaignProgressCommit();
}
