#ifndef WC3_CAMPAIGN_PROGRESS_H
#define WC3_CAMPAIGN_PROGRESS_H

#include "common/shared.h"

#define WC3_CAMPAIGN_PROGRESS_FILENAME "campaign-progress.orcp"
#define WC3_CAMPAIGN_PROGRESS_EDITIONS 2 // editions; ROC and TFT slots; used in progress arrays and file validation
#define WC3_CAMPAIGN_PROGRESS_CAMPAIGNS 5 // campaigns; maximum campaign slots per edition; used in progress arrays
#define WC3_CAMPAIGN_PROGRESS_MISSIONS 128 // missions; bounded mission slots per campaign; used in progress arrays

typedef enum {
    WC3_CAMPAIGN_EDITION_ROC = 0,
    WC3_CAMPAIGN_EDITION_TFT = 1,
} wc3CampaignEdition_t;

typedef struct {
    DWORD edition;
    DWORD campaign;
    DWORD mission;
} wc3CampaignProgressKey_t;

typedef struct {
    BYTE tutorial_known[WC3_CAMPAIGN_PROGRESS_EDITIONS];
    BYTE tutorial_cleared[WC3_CAMPAIGN_PROGRESS_EDITIONS];
    BYTE campaign_known[WC3_CAMPAIGN_PROGRESS_EDITIONS][WC3_CAMPAIGN_PROGRESS_CAMPAIGNS];
    BYTE campaign_available[WC3_CAMPAIGN_PROGRESS_EDITIONS][WC3_CAMPAIGN_PROGRESS_CAMPAIGNS];
    BYTE mission_known[WC3_CAMPAIGN_PROGRESS_EDITIONS][WC3_CAMPAIGN_PROGRESS_CAMPAIGNS]
                      [WC3_CAMPAIGN_PROGRESS_MISSIONS];
    BYTE mission_available[WC3_CAMPAIGN_PROGRESS_EDITIONS][WC3_CAMPAIGN_PROGRESS_CAMPAIGNS]
                          [WC3_CAMPAIGN_PROGRESS_MISSIONS];
} wc3CampaignProgress_t;

void wc3_campaign_progress_init(wc3CampaignProgress_t *progress);
BOOL wc3_campaign_progress_load(LPCSTR path, wc3CampaignProgress_t *progress);
BOOL wc3_campaign_progress_save(LPCSTR path, wc3CampaignProgress_t const *progress);
BOOL wc3_campaign_progress_set_tutorial(wc3CampaignProgress_t *progress, DWORD edition, BOOL cleared);
BOOL wc3_campaign_progress_set_campaign(wc3CampaignProgress_t *progress,
                                        wc3CampaignProgressKey_t key, BOOL available);
BOOL wc3_campaign_progress_set_mission(wc3CampaignProgress_t *progress,
                                       wc3CampaignProgressKey_t key, BOOL available);
BOOL wc3_campaign_progress_has_campaign(wc3CampaignProgress_t const *progress,
                                        wc3CampaignProgressKey_t key);
BOOL wc3_campaign_progress_campaign_available(wc3CampaignProgress_t const *progress,
                                              wc3CampaignProgressKey_t key);
BOOL wc3_campaign_progress_has_mission(wc3CampaignProgress_t const *progress,
                                       wc3CampaignProgressKey_t key);
BOOL wc3_campaign_progress_mission_available(wc3CampaignProgress_t const *progress,
                                             wc3CampaignProgressKey_t key);
LONG wc3_campaign_progress_campaign_index(DWORD edition, LPCSTR key);

#endif
