#include "g_local.h"

field_t fields[] = {
    EDICTFIELD(class_id, F_INT),
    EDICTFIELD(variation, F_INT),
    EDICTFIELD(build_project, F_INT),
    EDICTFIELD(spawn_time, F_INT),
    EDICTFIELD(harvested_lumber, F_INT),
    EDICTFIELD(harvested_gold, F_INT),
    EDICTFIELD(heatmap2, F_INT),
    EDICTFIELD(peonsinside, F_INT),
    EDICTFIELD(aiflags, F_INT),
    EDICTFIELD(damage, F_INT),
};

void WriteEdict(FILE *f, LPCEDICT ent) {
//    field_t *field;
//    edict_t temp = *ent;

//    // change the pointers to lengths or indexes
//    for (field=fields; field->name; field++) {
//        WriteField1 (f, field, (byte *)&temp);
//    }
//
//    // write the block
//    fwrite (&temp, sizeof(temp), 1, f);
//
//    // now write any allocated data following the edict
//    for (field=fields ; field->name ; field++)
//    {
//        WriteField2 (f, field, (byte *)ent);
//    }

}

void WriteGame(LPCSTR filename) {
    FILE *f = fopen(filename, "wb");
    
    FOR_LOOP(i, globals.num_edicts) {
        LPCEDICT ent = globals.edicts+i;
        if (!ent->inuse)
            continue;
        fwrite(&i, sizeof(i), 1, f);
        WriteEdict (f, ent);
    }
    
    fclose(f);
}

void ReadGame(LPCSTR filename) {
    FILE *f = fopen(filename, "rb");
    
    fclose(f);
}
