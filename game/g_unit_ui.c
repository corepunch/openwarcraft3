/*
 * g_unit_ui.c — Game-side unit UI data collection (Phase 8)
 *
 * Provides unit command buttons, inventory, and build queue data to server.
 */

#include "g_local.h"
#include "g_metadata.h"

/* Get command buttons for an entity */
BYTE G_GetCommandButtons(LPEDICT ent, gameCommandButton_t *buttons, BYTE max_buttons) {
    if (!ent || !ent->class_id || !buttons) {
        return 0;
    }
    
    LPCSTR class_name = GetClassName(ent->class_id);
    if (!class_name) {
        return 0;
    }
    
    BYTE count = 0;
    LPCSTR abilities = FindConfigValue(class_name, "abilList");
    
    /* Parse abilities list */
    if (abilities && *abilities) {
        char ability_buf[256];
        strncpy(ability_buf, abilities, sizeof(ability_buf) - 1);
        ability_buf[sizeof(ability_buf) - 1] = '\0';
        
        char *token = strtok(ability_buf, ",");
        while (token && count < max_buttons) {
            /* Trim whitespace */
            while (*token == ' ') token++;
            
            if (*token) {
                LPCSTR art = FindConfigValue(token, "Art");
                LPCSTR tooltip = FindConfigValue(token, "Tip");
                LPCSTR ubertip = FindConfigValue(token, "Ubertip");
                LPCSTR hotkey = FindConfigValue(token, "Hotkey");
                
                strncpy(buttons[count].art, art ? art : "", sizeof(buttons[count].art) - 1);
                strncpy(buttons[count].tooltip, tooltip ? tooltip : "", sizeof(buttons[count].tooltip) - 1);
                strncpy(buttons[count].ubertip, ubertip ? ubertip : "", sizeof(buttons[count].ubertip) - 1);
                strncpy(buttons[count].command, token, sizeof(buttons[count].command) - 1);
                buttons[count].hotkey = (hotkey && *hotkey) ? *hotkey : '\0';
                
                count++;
            }
            
            token = strtok(NULL, ",");
        }
    }
    
    return count;
}

/* Get inventory for an entity */
BYTE G_GetInventory(LPEDICT ent, gameInventoryItem_t *items, BYTE max_items) {
    if (!ent || !items) {
        return 0;
    }
    
    /* Check if entity is a hero (has non-zero hero level) */
    if (ent->hero.level == 0) {
        return 0;
    }
    
    BYTE count = 0;
    for (BYTE slot = 0; slot < MAX_INVENTORY && count < max_items; slot++) {
        LPEDICT item = ent->inventory[slot];
        if (item && item->class_id) {
            LPCSTR item_name = GetClassName(item->class_id);
            if (!item_name) {
                continue;
            }
            
            LPCSTR art = FindConfigValue(item_name, "Art");
            LPCSTR name = FindConfigValue(item_name, "Name");
            LPCSTR desc = FindConfigValue(item_name, "Description");
            
            strncpy(items[count].art, art ? art : "", sizeof(items[count].art) - 1);
            strncpy(items[count].tooltip, name ? name : "", sizeof(items[count].tooltip) - 1);
            strncpy(items[count].ubertip, desc ? desc : "", sizeof(items[count].ubertip) - 1);
            items[count].slot = slot;
            
            count++;
        }
    }
    
    return count;
}

/* Get build queue for an entity */
BYTE G_GetBuildQueue(LPEDICT ent, gameQueueItem_t *queue, BYTE max_queue) {
    if (!ent || !queue) {
        return 0;
    }
    
    /* Build queue is currently represented by a single building entity (ent->build) */
    /* For Phase 8, we'll return 0-1 items. Full queue support comes later. */
    if (!ent->build || !ent->build->class_id) {
        return 0;
    }
    
    if (max_queue < 1) {
        return 0;
    }
    
    LPCSTR build_name = GetClassName(ent->build->class_id);
    if (!build_name) {
        return 0;
    }
    
    LPCSTR art = FindConfigValue(build_name, "Art");
    
    strncpy(queue[0].art, art ? art : "", sizeof(queue[0].art) - 1);
    queue[0].entity = (WORD)ent->build->s.number;
    
    return 1;
}
