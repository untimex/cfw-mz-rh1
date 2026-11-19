#include "cfw_config.h"
#include "fw/tracks.h"
#include "fw/sys.h"

#define STOCK(type, addr) ((type *)THUMB_ADDR(addr))
#define DATA8(addr) (*(volatile const uint8_t *)(addr))
#define DATA32(addr) (*(volatile const uint32_t *)(addr))
extern int cfw_is_40elk(void);

enum { TITLE = 0, ARTIST = 1, ALBUM = 2, INFO = 3 };
enum { REMOTE = 2, HOME_PAGE = 0x0b, FIRST_ROW = 6, LAST_ROW = 8 };
static const uint8_t layouts[][3] = {
    {INFO, TITLE, ARTIST}, {INFO, TITLE, ALBUM}, {TITLE, ARTIST, ALBUM}
};

/* Private copies of the native 0x74-byte name records. Sony's formatter
 * keeps the multi-byte character cursor in each record. The default and
 * DISPLAY pages continue using their own Sony-owned records. */
struct name_row {
    _Alignas(4) uint8_t record[0x74];
    uint32_t track;
    uint8_t active, type, disc, caps, window, continued;
};
static struct name_row rows[3];

typedef void reset_record(void *);
typedef void name_reader(uint32_t device, uint32_t caps, void *record);
typedef int send_text(uint32_t, const void *, uint32_t, uint32_t, uint32_t,
                     uint32_t, uint32_t, uint32_t, uint32_t, const void *, uint32_t);

static void read_chunk(struct name_row *row, int restart)
{
    uint8_t *rec = row->record;
    if (restart) {
        memset(rec, 0, sizeof(row->record));
        STOCK(reset_record, 0x3c974)(rec);
        /* 0x58 = native track id, 0x60 = field type, 0x63 = icon. */
        memcpy(rec + 0x58, &row->track, sizeof(row->track));
        rec[0x60] = row->type;
        rec[0x63] = row->type == TITLE ? 2 : row->type == ARTIST ? 7 : 8;
        row->continued = 0;
    } else {
        row->continued = 1;
    }
    if (row->disc == DISC_TYPE_HIMD) {
        STOCK(name_reader, 0x3bfb8)(REMOTE, row->caps, rec);
    } else {
        /* Same wide-title preference as Sony's reader at 0x3b36a.
         * A missing wide title falls back to the ordinary MD title. */
        if (!(row->caps & 4) && (restart || (rec[0x73] & 0x20))) {
            rec[0x73] |= 0x20;
            STOCK(name_reader, 0x3c190)(REMOTE, row->caps, rec);
            if (rec[0x56]) return;
            rec[0x73] &= ~0x20;
            STOCK(reset_record, 0x3c974)(rec);
        }
        STOCK(name_reader, 0x3c228)(REMOTE, row->caps, rec);
    }
}

static void render_name(uint32_t slot, uint8_t type)
{
    struct name_row *row = &rows[slot - FIRST_ROW];
    uint32_t track = DATA32(0x008020f0);
    uint32_t disc = *disc_type;
    uint8_t caps = DATA8(0x00800665);
    /* Four playback seconds per chunk. Use the same cached second the
     * stock Info renderer reads. No I/O on ordinary redraws. */
    uint8_t window = DATA32(0x00801b38) / 4;
    if (!track || disc > DISC_TYPE_MD || (disc == DISC_TYPE_MD && type != TITLE)) {
        row->active = 0;
        STOCK(func1, 0xb3d94)(slot);
        return;
    }
    if (!row->active || row->track != track || row->type != type ||
        row->disc != disc || row->caps != caps) {
        row->track = track;
        row->type = type;
        row->disc = disc;
        row->caps = caps;
        row->active = 1;
        read_chunk(row, 1);
    } else if (row->window != window) {
        if (row->record[0x56] == 1)
            read_chunk(row, 0);
        else if (row->continued || !row->record[0x61])
            read_chunk(row, 1);
    }
    row->window = window;
    const uint8_t *rec = row->record;
    /* Sony requests at most 14 characters for a remote chunk. Leave room
     * for the driver prefix and terminator even for a two-byte encoding. */
    if (!rec[0x56] || !rec[0x61] || rec[0x61] > 28) {
        STOCK(func1, 0xb3d94)(slot);
        return;
    }
    STOCK(send_text, 0x35b80)(slot - FIRST_ROW, rec, rec[0x63], 0, 0,
                              rec[0x62], 0, 0, 0, rec + 0x6c, rec[0x61]);
}

typedef void renderer(uint32_t device, uint32_t slot, uint32_t id);

void cfw_remote_display(uint32_t device, uint32_t slot, uint32_t id,
                        renderer *original, uint32_t page)
{
    uint32_t layout = cfw_config.remote_display;
    /* Slot 9 is the independent status/icon renderer. Only replace the
     * three known home-page handlers and retain Sony's temporary overlays. */
    static const uint8_t home_ids[] = {0x17, 0x18, 0xa5};
    if (slot >= FIRST_ROW && slot <= LAST_ROW) {
        uint32_t index = slot - FIRST_ROW;
        if (layout >= 1 && layout <= 3 && device == REMOTE && page == HOME_PAGE &&
            *disc_type != DISC_TYPE_MD &&
            id == home_ids[index] && DATA8(0x00800590) == 0 && DATA8(0x00800661) == 3 &&
            !DATA8(0x0080069e) && !DATA8(0x008006a0) && !DATA8(0x008006a2) &&
            STOCK(func, 0x2cc6a)() && STOCK(func, 0x2d07c)() && cfw_is_40elk()) {
            uint8_t field = layouts[layout - 1][index];
            if (field == INFO) {
                rows[index].active = 0;
                STOCK(renderer, 0xb13e6)(device, slot, 0x18);
            } else {
                render_name(slot, field);
            }
            return;
        }
        rows[index].active = 0;
    }
    original(device, slot, id);
}

/* In Sony's three-line loop, r0-r3 already describe the row and renderer.
 * Its stack word at +4 holds the effective page after overlay selection. */
__attribute__((naked)) void cfw_remote_display_hook(void)
{
    __asm__ volatile (
        ".syntax unified\n"
        "push {r4, lr}\n"
        "ldr r4, [sp, #12]\n"
        "sub sp, #8\n"
        "str r4, [sp]\n"
        "bl cfw_remote_display\n"
        "add sp, #8\n"
        "pop {r4, pc}\n"
    );
}
