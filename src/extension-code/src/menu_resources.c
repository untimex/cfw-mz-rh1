#include "fw/core.h"
#include "fw/resources.h"
#include "fw/sys.h"
#include "fw/tracks.h"
#include "menu_resources.h"

static uint32_t cfw_remote_menu_label(uint32_t id)
{
    uint32_t i = 0;
    while (i < sizeof(menu_id_table) / sizeof(menu_id_table[0]) && menu_id_table[i] != id) ++i;
    return i;
}

// Rewrite menu resource lookup

#define MENU_SPACE 0x20
#define MENU_START 0x8E
#define MENU_CLOSE 0x8F

#define TRACK_ERASE_STRING "Erase Track    "

int cfw_load_menu_string_oled(uint32_t id, char *dest, uint8_t *len, char *n, uint32_t p, uint32_t config)
{

    int res;
    int32_t offset;
    uint32_t tmp_len;
    uint32_t idx = 0;

    void *menu_data = fw_menu_strings_oled;

    if (((config >> 2) & 1) == 0 && ((id & 0xFF) != MENU_CFW_ID_MARKER))
    {
        res = fw_search_for_menu_id(id, &offset);
        if (res != 0)
            return res;
    }
    
    else if ((id & 0xFF) == MENU_CFW_ID_MARKER) {
        offset = 0;
        idx = 0;

        for (idx = 0; idx < (sizeof(menu_id_table) / sizeof(uint32_t)); idx++) {
            if (menu_id_table[idx] == id) {
                break;
            }
        }
        menu_data = (void*)menu_string_table;
    }
    else
    {
        offset = id;
        if (id > FW_MENU_LENGTH)
            return -1;
    }

    _Static_assert(sizeof(menu_str_CHINESE_MODE) - 1 <= 16, "Framed OLED label width");
    if (id == MENU_RESOURCE_ID(DISPLAY_ITA) ||
        id == MENU_RESOURCE_ID(DISPLAY_TAL)) {
        _Static_assert(sizeof(menu_str_DISPLAY_ITA) - 1 <= 18 && sizeof(menu_str_DISPLAY_TAL) - 1 <= 18, "layout label width");
        return fw_load_string_resource(offset, menu_data, 0x10, 1, idx, dest, len, n);
    }

    *dest = ((config & 8) != 0) ? MENU_START : MENU_SPACE;

    res = fw_load_string_resource(offset, menu_data, 0x10, 1, idx, dest + 1, len, n);


    if (id == 0x10500) { // if we are in the track erase menu, inject track number
        uint16_t trackId = *current_track;
        memcpy(dest + 1, TRACK_ERASE_STRING, sizeof(TRACK_ERASE_STRING));
        int_to_string(trackId, &dest[13], 1, 0, 0);
        *len = sizeof(TRACK_ERASE_STRING) - 1;
    }

    if ((config & 8) == 0)
    {
        tmp_len = *len + 1;
    }
    else
    {
        memset(dest + 1 + (*len), MENU_SPACE, 0x10 - (*len));
        dest[0x11] = MENU_CLOSE;
        tmp_len = 0x12;
    }

    *len = tmp_len;
    return res;
}
typedef int remote_string_loader(uint32_t, char *, uint8_t *, uint8_t *, void *, uint32_t);
int cfw_load_menu_string_remote(uint32_t id, char *dest, uint8_t *len,
                                uint8_t *encoding, void *aux, uint32_t flags)
{
    if ((id & 0xff) != MENU_CFW_ID_MARKER)
        return ((remote_string_loader *)THUMB_ADDR(0xb5030))(id, dest, len, encoding, aux, flags);

    uint32_t i = cfw_remote_menu_label(id);
    if (i == sizeof(menu_id_table) / sizeof(menu_id_table[0])) return -1;
    const char *label = menu_string_table[i];
    *len = strlen(label);
    *encoding = 0;
    memcpy(dest, label, *len);
    return 0;
}
