#include "test_framework.h"

#include <stdlib.h>

#include "../common/shared.h"
#include "../common/net.h"

int _tests_run = 0;
int _tests_failed = 0;

void MemFree(HANDLE mem) {
    free(mem);
}

static sizeBuf_t make_msg_buf(BYTE *buf, DWORD bufsz) {
    sizeBuf_t sb;
    SZ_Init(&sb, buf, bufsz);
    return sb;
}

static void test_wow_appearance_pack_unpack_boundaries(void) {
    DWORD packed = Wow_PackAppearance(31, 30, 29, 14, 13, 11, 27);
    wowAppearance_t out = Wow_UnpackAppearance(packed);

    ASSERT_EQ_INT(out.skinColorID, 31);
    ASSERT_EQ_INT(out.faceID, 30);
    ASSERT_EQ_INT(out.hairStyleID, 29);
    ASSERT_EQ_INT(out.hairColorID, 14);
    ASSERT_EQ_INT(out.facialHairStyleID, 13);
    ASSERT_EQ_INT(out.classID, 11);
    ASSERT_EQ_INT(out.flags, 27);
}

static void test_wow_appearance_pack_masks_inputs(void) {
    DWORD packed = Wow_PackAppearance(0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9);
    wowAppearance_t out = Wow_UnpackAppearance(packed);

    ASSERT_EQ_INT(out.skinColorID, 0x1f);
    ASSERT_EQ_INT(out.faceID, 0x1e);
    ASSERT_EQ_INT(out.hairStyleID, 0x1d);
    ASSERT_EQ_INT(out.hairColorID, 0x0c);
    ASSERT_EQ_INT(out.facialHairStyleID, 0x0b);
    ASSERT_EQ_INT(out.classID, 0x0a);
    ASSERT_EQ_INT(out.flags, 0x19);
}

static void test_wow_equipment_pack_unpack(void) {
    DWORD packed = Wow_PackEquipment(1, 2, 127, 255);
    wowEquipment_t out = Wow_UnpackEquipment(packed);

    ASSERT_EQ_INT(out.upperBodyKit, 1);
    ASSERT_EQ_INT(out.lowerBodyKit, 2);
    ASSERT_EQ_INT(out.extremityKit, 127);
    ASSERT_EQ_INT(out.extraKit, 255);
}

static void test_wow_entity_delta_preserves_appearance_and_equipment(void) {
    BYTE buf[256];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    entityState_t from = { 0 };
    entityState_t to = { 0 };
    entityState_t out = { 0 };
    DWORD bits = 0;
    int number;

    to.number = 7;
    to.model = 3;
    to.appearance = Wow_PackAppearance(7, 6, 5, 4, 3, 1, 2);
    to.equipment = Wow_PackEquipment(9, 8, 7, 6);

    MSG_WriteDeltaEntity(&sb, &from, &to, true);

    sb.readcount = 0;
    number = MSG_ReadEntityBits(&sb, &bits);
    MSG_ReadDeltaEntity(&sb, &out, number, bits);

    ASSERT_EQ_INT(number, 7);
    ASSERT_EQ_INT(out.number, 7);
    ASSERT_EQ_INT(out.model, 3);
    ASSERT_EQ_INT(out.appearance, to.appearance);
    ASSERT_EQ_INT(out.equipment, to.equipment);
}

int main(void) {
    RUN_TEST(test_wow_appearance_pack_unpack_boundaries);
    RUN_TEST(test_wow_appearance_pack_masks_inputs);
    RUN_TEST(test_wow_equipment_pack_unpack);
    RUN_TEST(test_wow_entity_delta_preserves_appearance_and_equipment);
    TEST_RESULTS();
}
