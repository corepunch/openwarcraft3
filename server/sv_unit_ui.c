/*
 * sv_unit_ui.c — Server-side unit UI data request handler (Phase 8)
 *
 * Handles clc_request_unit_ui from client and responds with svc_unit_ui
 * containing command buttons, inventory, and build queue for selected units.
 */

#include "server.h"

/* Main handler for clc_request_unit_ui */
void SV_HandleUnitUIRequest(LPCLIENT client, LPSIZEBUF msg) {
    BYTE num_selected = MSG_ReadByte(msg);
    
    if (num_selected == 0 || num_selected > 12) {
        return; /* Invalid request */
    }
    
    /* Read entity numbers from request */
    SHORT entity_nums[12];
    for (BYTE i = 0; i < num_selected; i++) {
        entity_nums[i] = MSG_ReadShort(msg);
    }
    
    /* Begin response message */
    sizeBuf_t *response = &client->netchan.message;
    MSG_WriteByte(response, svc_unit_ui);
    MSG_WriteByte(response, num_selected);
    
    /* For each unit, query game DLL and write data */
    gameCommandButton_t buttons[12];
    gameInventoryItem_t inventory[6];
    gameQueueItem_t queue[7];
    
    for (BYTE i = 0; i < num_selected; i++) {
        SHORT entity_num = entity_nums[i];
        
        /* Validate entity number */
        if (entity_num < 0 || entity_num >= ge->max_edicts) {
            continue;
        }
        
        edict_t *ent = &ge->edicts[entity_num];
        
        /* Write entity number */
        MSG_WriteShort(response, entity_num);
        
        /* Query and write command buttons */
        BYTE num_buttons = ge->GetCommandButtons(ent, buttons, 12);
        MSG_WriteByte(response, num_buttons);
        for (BYTE j = 0; j < num_buttons; j++) {
            MSG_WriteString(response, buttons[j].art);
            MSG_WriteString(response, buttons[j].tooltip);
            MSG_WriteString(response, buttons[j].ubertip);
            MSG_WriteString(response, buttons[j].command);
            MSG_WriteByte(response, (BYTE)buttons[j].hotkey);
        }
        
        /* Query and write inventory */
        BYTE num_inventory = ge->GetInventory(ent, inventory, 6);
        MSG_WriteByte(response, num_inventory);
        for (BYTE j = 0; j < num_inventory; j++) {
            MSG_WriteString(response, inventory[j].art);
            MSG_WriteString(response, inventory[j].tooltip);
            MSG_WriteString(response, inventory[j].ubertip);
            MSG_WriteByte(response, inventory[j].slot);
        }
        
        /* Query and write build queue */
        BYTE num_queue = ge->GetBuildQueue(ent, queue, 7);
        MSG_WriteByte(response, num_queue);
        for (BYTE j = 0; j < num_queue; j++) {
            MSG_WriteString(response, queue[j].art);
            MSG_WriteShort(response, queue[j].entity);
        }
    }
}

