#ifndef BZ_WC3_PROGRESS_H
#define BZ_WC3_PROGRESS_H
#include "wc3_save.h"

#define BZ_PROGRESS_FILE "campaign.w3p"
#define BZ_PROGRESS_CAMPAIGNS 9 // campaigns; Blizzard.j ROC and TFT campaign indexes 0..8
#define BZ_PROGRESS_MISSIONS 128 // missions per campaign; CampaignStrings documents this native limit
#define BZ_PROGRESS_MAPS 512 // maps; bounded campaign launch/completion history across ROC and TFT

typedef enum { CAMPAIGN_CUSTOM = -1, CAMPAIGN_ROC, CAMPAIGN_TFT } CAMPAIGNDOMAIN;
typedef enum { PROGRESS_UNSET, PROGRESS_LOCKED, PROGRESS_OPEN } PROGRESSSTATE;
typedef enum { PROGRESS_CAMPAIGN, PROGRESS_MISSION, PROGRESS_OPENING, PROGRESS_ENDING,
    PROGRESS_TUTORIAL, PROGRESS_PLAYED, PROGRESS_COMPLETED } PROGRESSKIND;
enum { PROGRESS_MAP_PLAYED = 1, PROGRESS_MAP_COMPLETED = 2 };
typedef struct {
    PROGRESSSTATE avail, opening, ending;
    PROGRESSSTATE missions[BZ_PROGRESS_MISSIONS];
} CAMPAIGNSTATE;
typedef CAMPAIGNSTATE *LPCAMPAIGNSTATE;
typedef const CAMPAIGNSTATE *LPCCAMPAIGNSTATE;
typedef struct { PATHSTR path; DWORD flags; } PROGRESSMAP;
typedef PROGRESSMAP *LPPROGRESSMAP;
typedef const PROGRESSMAP *LPCPROGRESSMAP;
typedef struct {
    PROGRESSSTATE tutorial;
    CAMPAIGNSTATE campaigns[BZ_PROGRESS_CAMPAIGNS];
    DWORD count;
    PROGRESSMAP maps[BZ_PROGRESS_MAPS];
} CAMPAIGNPROGRESS;
typedef CAMPAIGNPROGRESS *LPCAMPAIGNPROGRESS;
typedef const CAMPAIGNPROGRESS *LPCCAMPAIGNPROGRESS;
typedef struct {
    PROGRESSKIND kind;
    DWORD campaign, mission;
    BOOL available;
    LPCSTR map;
} PROGRESSCHANGE;
typedef PROGRESSCHANGE *LPPROGRESSCHANGE;
typedef const PROGRESSCHANGE *LPCPROGRESSCHANGE;

int campaign_index(LPCSTR key, BOOL expansion);
int campaign_offset(DWORD offset, BOOL expansion);
CAMPAIGNDOMAIN campaign_domain(LPCSTR path);
DWORD progress_map_flags(LPCCAMPAIGNPROGRESS state, LPCSTR path);
SAVERESULT progress_load(LPCSTR path, LPCAMPAIGNPROGRESS state);
BOOL progress_change(LPCSTR path, LPCPROGRESSCHANGE change);
#endif
