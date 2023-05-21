#ifndef tables_h
#define tables_h

typedef char SHEETSTR[80];

struct SpawnData {
    DWORD Name;
    SHEETSTR Model;
};

struct AnimLookup {
    DWORD AnimSoundEvent;
    SHEETSTR SoundLabel;
};

struct SplatData {
    DWORD Name;
    SHEETSTR comment;
    SHEETSTR Dir;
    SHEETSTR file;
    DWORD Rows;
    DWORD Columns;
    DWORD BlendMode;
    DWORD Scale;
    DWORD Lifespan;
    DWORD Decay;
    DWORD UVLifespanStart;
    DWORD UVLifespanEnd;
    DWORD LifespanRepeat;
    DWORD UVDecayStart;
    DWORD UVDecayEnd;
    DWORD DecayRepeat;
    SHEETSTR Water;
    SHEETSTR Sound;
};

struct TerrainInfo {
    DWORD tileID;
    SHEETSTR dir;
    SHEETSTR file;
};

struct CliffInfo {
    DWORD cliffID;
    SHEETSTR cliffModelDir;
    SHEETSTR rampModelDir;
    SHEETSTR texDir;
    SHEETSTR texFile;
    SHEETSTR name;
    DWORD groundTile;
    DWORD upperTile;
};

struct UnitBalance {
    DWORD unitBalanceID;
    DWORD sortBalance;
    DWORD sort2;
    SHEETSTR comment;
    DWORD level;
    SHEETSTR flag;
    DWORD goldcost;
    DWORD lumbercost;
    DWORD goldRep;
    DWORD lumberRep;
    DWORD fmade;
    DWORD fused;
    DWORD bountydice;
    DWORD bountysides;
    DWORD bountyplus;
    DWORD stockMax;
    DWORD stockRegen;
    DWORD stockStart;
    DWORD HP;
    DWORD realHP;
    float regenHP;
    SHEETSTR regenType;
    DWORD manaN;
    DWORD realM;
    DWORD mana0;
    float regenMana;
    DWORD def;
    DWORD defUp;
    float realdef;
    SHEETSTR defType;
    DWORD spd;
    DWORD bldtm;
    DWORD sight;
    DWORD nsight;
    DWORD STR;
    DWORD INT;
    DWORD AGI;
    float STRplus;
    float INTplus;
    float AGIplus;
    DWORD abilTest;
    DWORD Primary;
    SHEETSTR upgrades;
    DWORD InBeta;
};

struct UnitData {
    DWORD unitID;
    SHEETSTR sort;
    SHEETSTR comment;
    SHEETSTR race;
    DWORD prio;
    DWORD threat;
    DWORD type;
    DWORD valid;
    DWORD deathType;
    float death;
    DWORD canSleep;
    DWORD cargoSize;
    SHEETSTR movetp;
    DWORD moveHeight;
    DWORD moveFloor;
    DWORD launchX;
    DWORD launchY;
    DWORD launchZ;
    DWORD impactZ;
    float turnRate;
    DWORD propWin;
    DWORD orientInterp;
    DWORD formation;
    float castpt;
    float castbsw;
    SHEETSTR targType;
    SHEETSTR pathTex;
    DWORD fatLOS;
    DWORD collision;
    DWORD points;
    DWORD buffType;
    DWORD buffRadius;
    DWORD nameCount;
    DWORD InBeta;
};

struct UnitUI {
    DWORD unitUIID;
    SHEETSTR file;
    SHEETSTR unitSound;
    SHEETSTR tilesets;
    DWORD tilesetSpecific;
    SHEETSTR name;
    SHEETSTR unitClass;
    DWORD special;
    DWORD inEditor;
    DWORD hiddenInEditor;
    DWORD hostilePal;
    DWORD dropItems;
    DWORD nbrandom;
    DWORD nbmmIcon;
    DWORD useClickHelper;
    float blend;
    float scale;
    DWORD scaleBull;
    SHEETSTR preventPlace;
    DWORD requirePlace;
    DWORD isbldg;
    DWORD maxPitch;
    DWORD maxRoll;
    DWORD elevPts;
    DWORD elevRad;
    DWORD fogRad;
    DWORD walk;
    DWORD run;
    DWORD selZ;
    DWORD weap1;
    DWORD weap2;
    DWORD teamColor;
    DWORD customTeamColor;
    SHEETSTR armor;
    DWORD modelScale;
    DWORD red;
    DWORD green;
    DWORD blue;
    SHEETSTR uberSplat;
    SHEETSTR unitShadow;
    SHEETSTR buildingShadow;
    DWORD shadowW;
    DWORD shadowH;
    DWORD shadowX;
    DWORD shadowY;
    DWORD occH;
    DWORD InBeta;
};

struct DoodadInfo {
    DWORD doodID;
    SHEETSTR dir;
    SHEETSTR file;
    SHEETSTR pathTex;
};

struct DestructableData {
    DWORD DestructableID;
    SHEETSTR category;
    SHEETSTR tilesets;
    SHEETSTR tilesetSpecific;
    SHEETSTR dir;
    SHEETSTR file;
    DWORD lightweight;
    DWORD fatLOS;
    DWORD texID;
    SHEETSTR texFile;
};

#endif
