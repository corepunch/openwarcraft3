/*
 * cl_unit_ui.c — Client-side unit UI data parser (Phase 8)
 *
 * Parses svc_unit_ui messages from server and forwards to UI library.
 */

#include "client.h"

void CL_ParseUnitUI(LPSIZEBUF msg) {
    BYTE num_units = MSG_ReadByte(msg);
    
    if (num_units == 0 || num_units > 12) {
        return; /* Invalid message */
    }
    
    /* Allocate array for unit data */
    uiUnitData_t *units = (uiUnitData_t *)MemAlloc(sizeof(uiUnitData_t) * num_units);
    memset(units, 0, sizeof(uiUnitData_t) * num_units);
    
    /* Parse each unit's data */
    for (BYTE i = 0; i < num_units; i++) {
        uiUnitData_t *unit = &units[i];
        
        /* Read entity number */
        unit->entity_num = MSG_ReadShort(msg);
        
        /* Read command buttons */
        unit->num_buttons = MSG_ReadByte(msg);
        if (unit->num_buttons > MAX_COMMAND_BUTTONS) {
            unit->num_buttons = MAX_COMMAND_BUTTONS;
        }
        
        for (BYTE j = 0; j < unit->num_buttons; j++) {
            uiCommandButton_t *btn = &unit->buttons[j];
            LPCSTR art = MSG_ReadString2(msg);
            LPCSTR tip = MSG_ReadString2(msg);
            LPCSTR ubertip = MSG_ReadString2(msg);
            LPCSTR command = MSG_ReadString2(msg);
            
            strncpy(btn->art, art, sizeof(btn->art) - 1);
            strncpy(btn->tooltip, tip, sizeof(btn->tooltip) - 1);
            strncpy(btn->ubertip, ubertip, sizeof(btn->ubertip) - 1);
            strncpy(btn->command, command, sizeof(btn->command) - 1);
            btn->hotkey = MSG_ReadByte(msg);
        }
        
        /* Read inventory items */
        unit->num_inventory = MSG_ReadByte(msg);
        if (unit->num_inventory > MAX_INVENTORY_SLOTS) {
            unit->num_inventory = MAX_INVENTORY_SLOTS;
        }
        
        for (BYTE j = 0; j < unit->num_inventory; j++) {
            uiInventoryItem_t *item = &unit->inventory[j];
            LPCSTR art = MSG_ReadString2(msg);
            LPCSTR tip = MSG_ReadString2(msg);
            LPCSTR ubertip = MSG_ReadString2(msg);
            
            strncpy(item->art, art, sizeof(item->art) - 1);
            strncpy(item->tooltip, tip, sizeof(item->tooltip) - 1);
            strncpy(item->ubertip, ubertip, sizeof(item->ubertip) - 1);
            item->slot = MSG_ReadByte(msg);
        }
        
        /* Read build queue */
        unit->num_queue = MSG_ReadByte(msg);
        if (unit->num_queue > MAX_BUILD_QUEUE_ITEMS) {
            unit->num_queue = MAX_BUILD_QUEUE_ITEMS;
        }
        
        for (BYTE j = 0; j < unit->num_queue; j++) {
            uiQueueItem_t *queue_item = &unit->queue[j];
            LPCSTR art = MSG_ReadString2(msg);
            
            strncpy(queue_item->art, art, sizeof(queue_item->art) - 1);
            queue_item->entity = MSG_ReadShort(msg);
        }
    }
    
    /* Forward to UI library */
    if (ui.UpdateUnitUI) {
        ui.UpdateUnitUI((DWORD)num_units, units);
    }
    
    /* Free temporary buffer */
    MemFree(units);
}
