#include <string.h>

#include "test_framework.h"
#include "test_harness.h"
#include "../../../common/net.h"

static int fake_image_index_ser(LPCSTR name) {
    return (name && *name) ? 321 : 0;
}

static int fake_model_index_ser(LPCSTR name) {
    return (name && *name) ? 654 : 0;
}

static void reset_ui_ser_state(void) {
    UI_ClearTemplates();
    gi.ImageIndex = fake_image_index_ser;
    gi.ModelIndex = fake_model_index_ser;
    UI_WriteStart(0);
}

static BOOL build_frame(LPCFRAMEDEF frame, uiFrame_t *out, BYTE *typedata, char *textbuf) {
    return UI_BuildFrameForWrite(frame,
                                 out,
                                 typedata,
                                 512,
                                 textbuf,
                                 sizeof(UINAME));
}

static void test_build_frame_rejects_null_arguments(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[32];
    UINAME textbuf;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_FRAME);

    ASSERT_EQ_INT(UI_BuildFrameForWrite(NULL, &out, typedata, sizeof(typedata), textbuf, sizeof(textbuf)), 0);
    ASSERT_EQ_INT(UI_BuildFrameForWrite(&frame, NULL, typedata, sizeof(typedata), textbuf, sizeof(textbuf)), 0);
    ASSERT_EQ_INT(UI_BuildFrameForWrite(&frame, &out, NULL, sizeof(typedata), textbuf, sizeof(textbuf)), 0);
    ASSERT_EQ_INT(UI_BuildFrameForWrite(&frame, &out, typedata, 0, textbuf, sizeof(textbuf)), 0);
    ASSERT_EQ_INT(UI_BuildFrameForWrite(&frame, &out, typedata, sizeof(typedata), NULL, sizeof(textbuf)), 0);
    ASSERT_EQ_INT(UI_BuildFrameForWrite(&frame, &out, typedata, sizeof(typedata), textbuf, 0), 0);
}

static void test_build_text_frame_sets_label_payload_and_color(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiLabel_t *label;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_TEXT);
    strcpy(frame.Name, "LabelA");
    frame.Font.Justification.Horizontal = FONT_JUSTIFYRIGHT;
    frame.Font.Justification.Vertical = FONT_JUSTIFYBOTTOM;
    frame.Font.Justification.Offset.x = 4;
    frame.Font.Justification.Offset.y = -3;
    frame.Font.Color = (COLOR32){10, 20, 30, 40};

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_TEXT);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiLabel_t));
    ASSERT(out.text == frame.Name);
    ASSERT_EQ_INT(out.color.r, 10);
    ASSERT_EQ_INT(out.color.g, 20);
    ASSERT_EQ_INT(out.color.b, 30);
    ASSERT_EQ_INT(out.color.a, 40);

    label = (uiLabel_t *)out.buffer.data;
    ASSERT_EQ_INT(label->textalignx, FONT_JUSTIFYRIGHT);
    ASSERT_EQ_INT(label->textaligny, FONT_JUSTIFYBOTTOM);
    ASSERT_EQ_INT(label->offsetx, 4);
    ASSERT_EQ_INT(label->offsety, -3);
}

static void test_build_text_frame_uses_explicit_text_when_present(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_TEXT);
    strcpy(frame.Name, "LabelB");
    UI_SetText(&frame, "Hello %s", "UI");

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_STR_EQ(out.text, "Hello UI");
}

static void test_build_backdrop_payload_size_and_values(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiBackdrop_t *bd;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_BACKDROP);
    frame.Backdrop.TileBackground = true;
    frame.Backdrop.BlendAll = true;
    frame.Backdrop.CornerFlags = 0x17;
    frame.Backdrop.CornerSize = 0.015f;
    frame.Backdrop.BackgroundSize = 0.125f;
    frame.Backdrop.BackgroundInsets[0] = 0.01f;
    frame.Backdrop.BackgroundInsets[1] = 0.02f;
    frame.Backdrop.BackgroundInsets[2] = 0.03f;
    frame.Backdrop.BackgroundInsets[3] = 0.04f;
    frame.Backdrop.EdgeFile = 11;
    frame.Backdrop.Background = 22;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_BACKDROP);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiBackdrop_t));

    bd = (uiBackdrop_t *)out.buffer.data;
    ASSERT_EQ_INT(bd->TileBackground, 1);
    ASSERT_EQ_INT(bd->BlendAll, 1);
    ASSERT_EQ_INT(bd->CornerFlags, 0x17);
    ASSERT_FLOAT_EQ(bd->CornerSize, 0.015f);
    ASSERT_FLOAT_EQ(bd->BackgroundSize, 0.125f);
    ASSERT_FLOAT_EQ(bd->BackgroundInsets[0], 0.01f);
    ASSERT_FLOAT_EQ(bd->BackgroundInsets[3], 0.04f);
    ASSERT_EQ_INT(bd->EdgeFile, 11);
    ASSERT_EQ_INT(bd->Background, 22);
}

static void test_build_sprite_uses_portrait_model_index(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_SPRITE);
    frame.Portrait.model = 777;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_SPRITE);
    ASSERT_EQ_INT(out.tex.index, 777);
    ASSERT_EQ_INT((int)out.buffer.size, 0);
}

static void test_build_frame_preserves_type_specific_alpha_mode(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_BACKDROP);
    frame.AlphaMode = BLEND_MODE_BLEND;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_BACKDROP);
    ASSERT_EQ_INT(out.flags.alphaMode, BLEND_MODE_BLEND);
}

static void test_build_frame_scales_offsets_to_ui_framepoint_int16(void) {
    FRAMEDEF root;
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();

    UI_InitFrame(&root, FT_FRAME);
    UI_InitFrame(&frame, FT_FRAME);

    UI_SetPoint(&frame, FRAMEPOINT_TOPLEFT, &root, FRAMEPOINT_TOPLEFT, 0.25f, -0.125f);

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(out.points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT((int)out.points.x[FPP_MIN].offset, (int)(0.25f * UI_FRAMEPOINT_SCALE));
    ASSERT_EQ_INT((int)out.points.y[FPP_MIN].offset, (int)(-0.125f * UI_FRAMEPOINT_SCALE));
}

static void write_then_read_delta_ui(const uiFrame_t *from,
                                     const uiFrame_t *to,
                                     uiFrame_t *decoded,
                                     BYTE *typedata_out,
                                     DWORD *typedata_size,
                                     const char **decoded_text)
{
    BYTE msgdata[4096] = {0};
    sizeBuf_t msg = {0};
    sizeBuf_t rd = {0};
    DWORD bits = 0;
    int num = 0;

    msg.data = msgdata;
    msg.maxsize = sizeof(msgdata);

    MSG_WriteDeltaUIFrame(&msg, from, to, true);
    MSG_WriteShort(&msg, to->buffer.size);
    MSG_Write(&msg, to->buffer.data, to->buffer.size);

    rd.data = msgdata;
    rd.cursize = msg.cursize;

    num = MSG_ReadEntityBits(&rd, &bits);
    MSG_ReadDeltaUIFrame(&rd, decoded, num, bits);

    *typedata_size = (DWORD)MSG_ReadShort(&rd);
    ASSERT(*typedata_size <= 4096);
    MSG_Read(&rd, typedata_out, *typedata_size);

    *decoded_text = decoded->text;
}

static void test_delta_roundtrip_preserves_text_fields(void) {
    FRAMEDEF frame;
    uiFrame_t base;
    uiFrame_t to;
    uiFrame_t decoded;
    BYTE typedata[512];
    BYTE typedata_out[512];
    UINAME textbuf;
    DWORD typedata_size = 0;
    const char *decoded_text = NULL;

    reset_ui_ser_state();
    memset(&base, 0, sizeof(base));
    memset(&decoded, 0, sizeof(decoded));

    UI_InitFrame(&frame, FT_TEXT);
    strcpy(frame.Name, "NameFallback");
    UI_SetText(&frame, "SerializedText");
    frame.Stat = 9;

    ASSERT(build_frame(&frame, &to, typedata, textbuf));

    write_then_read_delta_ui(&base, &to, &decoded, typedata_out, &typedata_size, &decoded_text);

    ASSERT_EQ_INT(decoded.flags.type, FT_TEXT);
    ASSERT_EQ_INT(decoded.stat, 9);
    ASSERT_STR_EQ(decoded_text, "SerializedText");
    ASSERT_EQ_INT((int)typedata_size, (int)sizeof(uiLabel_t));
}

static void write_then_read_delta_player(const PLAYER *from,
                                         const PLAYER *to,
                                         PLAYER *decoded)
{
    BYTE msgdata[4096] = {0};
    sizeBuf_t msg = {0};
    sizeBuf_t rd = {0};
    DWORD bits = 0;
    int number = 0;

    msg.data = msgdata;
    msg.maxsize = sizeof(msgdata);

    MSG_WriteDeltaPlayerState(&msg, from, to);

    rd.data = msgdata;
    rd.cursize = msg.cursize;

    number = MSG_ReadPlayerBits(&rd, &bits);
    MSG_ReadDeltaPlayerState(&rd, decoded, number, bits);
}

static void test_player_text_delta_roundtrip_updates_dynamic_text(void) {
    GAMECLIENT client;
    PLAYER base;
    PLAYER previous;
    PLAYER decoded;

    memset(&client, 0, sizeof(client));
    memset(&base, 0, sizeof(base));
    memset(&decoded, 0, sizeof(decoded));

    G_SetPlayerText(&client, PLAYERTEXT_MAP_TITLE, "First Map");
    write_then_read_delta_player(&base, &client.ps, &decoded);
    ASSERT_STR_EQ(decoded.texts[PLAYERTEXT_MAP_TITLE], "First Map");

    previous = client.ps;
    G_SetPlayerText(&client, PLAYERTEXT_MAP_TITLE, "Second Map");
    ASSERT(previous.texts[PLAYERTEXT_MAP_TITLE] != client.ps.texts[PLAYERTEXT_MAP_TITLE]);

    write_then_read_delta_player(&previous, &client.ps, &decoded);
    ASSERT_STR_EQ(decoded.texts[PLAYERTEXT_MAP_TITLE], "Second Map");

    previous = client.ps;
    G_SetPlayerText(&client, PLAYERTEXT_MAP_PREVIEW, "Maps\\Example.w3m/war3mapMap.blp");
    write_then_read_delta_player(&previous, &client.ps, &decoded);
    ASSERT_STR_EQ(decoded.texts[PLAYERTEXT_MAP_PREVIEW], "Maps\\Example.w3m/war3mapMap.blp");
}

static void test_player_identity_delta_roundtrip(void) {
    PLAYER base;
    PLAYER to;
    PLAYER decoded;

    memset(&base, 0, sizeof(base));
    memset(&to, 0, sizeof(to));
    memset(&decoded, 0, sizeof(decoded));

    to.number = 3;
    to.team = 2;
    to.color = 6;
    to.race = 4;
    to.start_location = 3;
    to.name = "Blue Player";

    write_then_read_delta_player(&base, &to, &decoded);

    ASSERT_EQ_INT((int)decoded.number, 3);
    ASSERT_EQ_INT((int)decoded.team, 2);
    ASSERT_EQ_INT((int)decoded.color, 6);
    ASSERT_EQ_INT((int)decoded.race, 4);
    ASSERT_EQ_INT((int)decoded.start_location, 3);
    ASSERT_STR_EQ(decoded.name, "Blue Player");
}

static void test_delta_roundtrip_preserves_backdrop_typedata(void) {
    FRAMEDEF frame;
    uiFrame_t base;
    uiFrame_t to;
    uiFrame_t decoded;
    BYTE typedata[512];
    BYTE typedata_out[512];
    UINAME textbuf;
    DWORD typedata_size = 0;
    const char *decoded_text = NULL;
    uiBackdrop_t *bd;

    reset_ui_ser_state();
    memset(&base, 0, sizeof(base));
    memset(&decoded, 0, sizeof(decoded));

    UI_InitFrame(&frame, FT_BACKDROP);
    frame.Backdrop.TileBackground = true;
    frame.Backdrop.BlendAll = true;
    frame.Backdrop.CornerFlags = 0x33;
    frame.Backdrop.Background = 101;
    frame.Backdrop.EdgeFile = 202;

    ASSERT(build_frame(&frame, &to, typedata, textbuf));

    write_then_read_delta_ui(&base, &to, &decoded, typedata_out, &typedata_size, &decoded_text);

    ASSERT_EQ_INT(decoded.flags.type, FT_BACKDROP);
    ASSERT_EQ_INT((int)typedata_size, (int)sizeof(uiBackdrop_t));
    ASSERT(decoded_text == NULL || *decoded_text == '\0');

    bd = (uiBackdrop_t *)typedata_out;
    ASSERT_EQ_INT(bd->TileBackground, 1);
    ASSERT_EQ_INT(bd->BlendAll, 1);
    ASSERT_EQ_INT(bd->CornerFlags, 0x33);
    ASSERT_EQ_INT(bd->Background, 101);
    ASSERT_EQ_INT(bd->EdgeFile, 202);
}

static void test_build_frame_sets_uv_bytes_from_texcoord(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();
    UI_InitFrame(&frame, FT_FRAME);
    frame.Texture.TexCoord.min.x = 0.25f;
    frame.Texture.TexCoord.max.x = 0.75f;
    frame.Texture.TexCoord.min.y = 0.10f;
    frame.Texture.TexCoord.max.y = 0.90f;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.tex.coord[0], 63);
    ASSERT_EQ_INT(out.tex.coord[1], 191);
    ASSERT_EQ_INT(out.tex.coord[2], 25);
    ASSERT_EQ_INT(out.tex.coord[3], 229);
}

static void test_collect_tree_and_build_frame_numbering_is_stable(void) {
    FRAMEDEF root;
    FRAMEDEF child;
    uiFrame_t out_root;
    uiFrame_t out_child;
    BYTE typedata_root[512];
    BYTE typedata_child[512];
    UINAME textbuf_root;
    UINAME textbuf_child;

    reset_ui_ser_state();

    UI_InitFrame(&root, FT_FRAME);
    strcpy(root.Name, "Root");
    root.inuse = true;

    UI_InitFrame(&child, FT_TEXT);
    strcpy(child.Name, "Child");
    child.inuse = true;
    child.Parent = &root;

    ASSERT(build_frame(&root, &out_root, typedata_root, textbuf_root));
    ASSERT(build_frame(&child, &out_child, typedata_child, textbuf_child));

    ASSERT_EQ_INT(out_root.number, 1);
    ASSERT_EQ_INT(out_child.number, 2);
    ASSERT_EQ_INT(out_child.parent, 1);
}

static void test_build_frame_returns_true_for_supported_types(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;

    reset_ui_ser_state();

    UI_InitFrame(&frame, FT_TEXTAREA);
    frame.TextArea.Inset = 0.5f;
    frame.Font.Index = 42;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_TEXTAREA);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiTextArea_t));
}

static void test_build_editbox_writes_control_payload(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiEditBox_t *edit;

    reset_ui_ser_state();

    UI_InitFrame(&frame, FT_EDITBOX);
    frame.Font.Index = 7;
    frame.Edit.BorderSize = 0.009f;
    frame.Edit.MaxChars = 15;
    frame.Edit.CursorColor = (COLOR32){1, 2, 3, 4};
    frame.Edit.TextColor = (COLOR32){5, 6, 7, 8};
    UI_SetText(&frame, "Player");

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_EDITBOX);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiEditBox_t));
    ASSERT_STR_EQ(out.text, "Player");

    edit = (uiEditBox_t *)out.buffer.data;
    ASSERT_EQ_INT(edit->font, 7);
    ASSERT_FLOAT_EQ(edit->borderSize, 0.009f);
    ASSERT_EQ_INT(edit->maxChars, 15);
    ASSERT_EQ_INT(edit->cursorColor.b, 3);
    ASSERT_EQ_INT(edit->textColor.g, 6);
}

static void test_build_editbox_uses_edit_text_frame_font_and_color(void) {
    LPFRAMEDEF frame;
    LPFRAMEDEF text;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiEditBox_t *edit;

    reset_ui_ser_state();

    frame = UI_Spawn(FT_EDITBOX, NULL);
    text = UI_Spawn(FT_TEXT, frame);
    ASSERT_NOT_NULL(frame);
    ASSERT_NOT_NULL(text);

    strcpy(frame->Name, "Edit");
    strcpy(text->Name, "EditText");
    strcpy(frame->Edit.TextFrame, text->Name);
    frame->Font.Index = 7;
    frame->Font.Color = (COLOR32){240, 200, 18, 255};
    text->Font.Index = 12;
    text->Font.Color = COLOR32_WHITE;

    ASSERT(build_frame(frame, &out, typedata, textbuf));

    edit = (uiEditBox_t *)out.buffer.data;
    ASSERT_EQ_INT(edit->font, 12);
    ASSERT_EQ_INT(edit->textColor.r, 255);
    ASSERT_EQ_INT(edit->textColor.g, 255);
    ASSERT_EQ_INT(edit->textColor.b, 255);
}

static void test_build_listbox_writes_control_payload(void) {
    FRAMEDEF frame;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiListBox_t *listbox;

    reset_ui_ser_state();

    UI_InitFrame(&frame, FT_LISTBOX);
    strcpy(frame.Name, "MapListBox");
    frame.Font.Index = 11;
    frame.Font.Justification.Horizontal = FONT_JUSTIFYLEFT;
    frame.ListBox.Border = 0.01f;
    strcpy(frame.ListBox.FetchCommand, "maps");
    frame.Menu.Item.Height = 0.02f;

    ASSERT(build_frame(&frame, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_LISTBOX);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiListBox_t));

    listbox = (uiListBox_t *)out.buffer.data;
    ASSERT_EQ_INT(listbox->text.font, 11);
    ASSERT_FLOAT_EQ(listbox->border, 0.01f);
    ASSERT_FLOAT_EQ(listbox->itemHeight, 0.02f);
    ASSERT_EQ_INT(listbox->selectedIndex, -1);
    ASSERT_STR_EQ(listbox->id, "MapListBox");
    ASSERT_STR_EQ(listbox->fetchCommand, "maps");
}

static LPFRAMEDEF make_scrollbar_button(LPFRAMEDEF scrollbar,
                                        LPCSTR buttonName,
                                        LPCSTR backdropName,
                                        DWORD background) {
    LPFRAMEDEF button = UI_Spawn(FT_BUTTON, scrollbar);
    LPFRAMEDEF backdrop = UI_Spawn(FT_BACKDROP, button);

    ASSERT_NOT_NULL(button);
    ASSERT_NOT_NULL(backdrop);
    strcpy(button->Name, buttonName);
    strcpy(backdrop->Name, backdropName);
    strcpy(button->Control.Backdrop.Normal, backdropName);
    backdrop->Backdrop.Background = background;
    return button;
}

static void test_build_scrollbar_writes_control_payload(void) {
    LPFRAMEDEF scrollbar;
    LPFRAMEDEF track;
    LPFRAMEDEF inc;
    LPFRAMEDEF dec;
    LPFRAMEDEF thumb;
    uiFrame_t out;
    BYTE typedata[512];
    UINAME textbuf;
    uiScrollBar_t *data;

    reset_ui_ser_state();

    scrollbar = UI_Spawn(FT_SCROLLBAR, NULL);
    track = UI_Spawn(FT_BACKDROP, scrollbar);
    ASSERT_NOT_NULL(scrollbar);
    ASSERT_NOT_NULL(track);

    strcpy(scrollbar->Name, "Scroll");
    strcpy(track->Name, "ScrollTrack");
    strcpy(scrollbar->Control.Backdrop.Normal, track->Name);
    track->Backdrop.Background = 10;
    inc = make_scrollbar_button(scrollbar, "IncButton", "IncBackdrop", 20);
    dec = make_scrollbar_button(scrollbar, "DecButton", "DecBackdrop", 30);
    thumb = make_scrollbar_button(scrollbar, "ThumbButton", "ThumbBackdrop", 40);
    strcpy(scrollbar->Slider.IncButtonFrame, inc->Name);
    strcpy(scrollbar->Slider.DecButtonFrame, dec->Name);
    strcpy(scrollbar->Slider.ThumbButtonFrame, thumb->Name);

    ASSERT(build_frame(scrollbar, &out, typedata, textbuf));
    ASSERT_EQ_INT(out.flags.type, FT_SCROLLBAR);
    ASSERT_EQ_INT((int)out.buffer.size, (int)sizeof(uiScrollBar_t));

    data = (uiScrollBar_t *)out.buffer.data;
    ASSERT_EQ_INT(data->background.Background, 10);
    ASSERT_EQ_INT(data->incButton.Background, 20);
    ASSERT_EQ_INT(data->decButton.Background, 30);
    ASSERT_EQ_INT(data->thumbButton.Background, 40);
}

BEGIN_SUITE(ui_serialize)
    RUN_TEST(test_build_frame_rejects_null_arguments);
    RUN_TEST(test_build_text_frame_sets_label_payload_and_color);
    RUN_TEST(test_build_text_frame_uses_explicit_text_when_present);
    RUN_TEST(test_build_backdrop_payload_size_and_values);
    RUN_TEST(test_build_sprite_uses_portrait_model_index);
    RUN_TEST(test_build_frame_preserves_type_specific_alpha_mode);
    RUN_TEST(test_build_frame_scales_offsets_to_ui_framepoint_int16);
    RUN_TEST(test_delta_roundtrip_preserves_text_fields);
    RUN_TEST(test_player_text_delta_roundtrip_updates_dynamic_text);
    RUN_TEST(test_player_identity_delta_roundtrip);
    RUN_TEST(test_delta_roundtrip_preserves_backdrop_typedata);
    RUN_TEST(test_build_frame_sets_uv_bytes_from_texcoord);
    RUN_TEST(test_collect_tree_and_build_frame_numbering_is_stable);
    RUN_TEST(test_build_frame_returns_true_for_supported_types);
    RUN_TEST(test_build_editbox_writes_control_payload);
    RUN_TEST(test_build_editbox_uses_edit_text_frame_font_and_color);
    RUN_TEST(test_build_listbox_writes_control_payload);
    RUN_TEST(test_build_scrollbar_writes_control_payload);
END_SUITE()
