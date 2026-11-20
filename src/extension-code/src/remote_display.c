#include "cfw_config.h"
#include "fw/tracks.h"
#include "fw/sys.h"
#include "menu_resources.h"

#define STOCK(type, addr) ((type *)THUMB_ADDR(addr))
#define DATA8(addr) (*(volatile const uint8_t *)(addr))
#define DATA32(addr) (*(volatile const uint32_t *)(addr))
extern int cfw_is_40elk(void);

enum { TITLE = 0, ARTIST = 1, ALBUM = 2, INFO = 3, CYCLE = 4 };
enum { REMOTE = 2, HOME_PAGE = 0x0b, FIRST_ROW = 6, LAST_ROW = 8 };
enum {
    FIRST_PAUSE = 30, STEP_PAUSE = 8, NAME_MORE = 1,
    STOCK_ROTATION_TICKS = 0x7d, /* Sony home-screen name duration. */
    /* The 40ELK shows ten single-byte or five double-byte characters. */
    MAX_CHUNK = 42, MAX_STATIC = 28, LCD_BYTES = 10
};
static const uint8_t layouts[][3] = {
    {INFO, TITLE, ARTIST}, {INFO, TITLE, ALBUM}, {TITLE, ARTIST, ALBUM},
    {INFO, TITLE, CYCLE}
};

struct scroll_cursor {
    uint8_t offset, pause;
};

struct name_row {
    /* The native formatter keeps its metadata cursor in this record. */
    _Alignas(4) uint8_t record[0x74];
    uint32_t track;
    struct scroll_cursor scroll;
    uint8_t field, type, length;
    uint8_t text[MAX_CHUNK + LCD_BYTES];
};
static struct name_row rows[3];
static struct scroll_cursor menu_scroll[3];
static uint8_t cycle_ticks;

/* The native LCD timer calls this once per row. Keep its countdown intact. */
void cfw_remote_display_tick(uint32_t slot)
{
    STOCK(func1, 0x3a5cc)(slot);
    if (slot == LAST_ROW && cycle_ticks)
        --cycle_ticks;
}

typedef void name_reader(uint32_t, uint32_t, void *);
typedef uint8_t *get_row_record(uint32_t);
typedef int send_text(uint32_t, const void *, uint32_t, uint32_t, uint32_t,
                      uint32_t, uint32_t, uint32_t, uint32_t, const void *, uint32_t);

static unsigned glyph_bytes(const uint8_t *text, unsigned encoding)
{
    if (encoding == 0x90) {
        /* The stock Japanese scanner uses this ROM vector to classify lead bytes. */
        unsigned kind = (*(func1 *const *)0x70)(*text);
        return 1 + (kind == 4 || kind == 6);
    }
    if (encoding == 0x84)
        return (*text & 0xfc) == 0xd8 ? 4 : 2;
    return encoding == 2 ? 2 : 1;
}

/* Keep using the stock static-text path. Each offset is a fresh frame,
 * independent of the remote's acknowledgement for native scrolling. */
static void show_window(uint32_t slot, const uint8_t *rec, const uint8_t *text,
                        unsigned length)
{
    if (length > MAX_STATIC)
        length = MAX_STATIC;
    STOCK(send_text, 0x35b80)(slot - FIRST_ROW, text, rec[0x63], 0, 0,
                              rec[0x62], 0, 0, 0, rec + 0x6c, length);
}

static __attribute__((noinline)) void advance(struct scroll_cursor *scroll, unsigned remaining,
                                               unsigned step, unsigned width)
{
    if (scroll->pause) {
        --scroll->pause;
    } else if (remaining > width) {
        scroll->offset += step;
        scroll->pause = remaining - step <= width ? FIRST_PAUSE : STEP_PAUSE;
    } else if (scroll->offset) {
        scroll->offset = 0;
        scroll->pause = FIRST_PAUSE;
    }
}

static void append_chunk(struct name_row *row)
{
    uint8_t *rec = row->record;
    rec[0x61] = 0;
    STOCK(name_reader, 0x3bfb8)(REMOTE, DATA8(0x00800665), rec);
    unsigned length = rec[0x61];
    if (!length || length > MAX_CHUNK) {
        rec[0x56] = 0;
        return;
    }
    memcpy(row->text + row->length, rec, length);
    row->length += length;
}

static void restart_name(struct name_row *row)
{
    uint8_t *rec = row->record;
    memset(rec, 0, sizeof(row->record));
    ((void (*)(void *))THUMB_ADDR(0x3c974))(rec);
    __builtin_memcpy(rec + 0x58, &row->track, sizeof(row->track));
    rec[0x60] = row->type;
    rec[0x63] = row->type == TITLE ? 2 : row->type + 6;
    row->length = 0;
    row->scroll.offset = 0;
    row->scroll.pause = FIRST_PAUSE;
    append_chunk(row);
}

static void restart_cycle(struct name_row *row, uint8_t type)
{
    cycle_ticks = STOCK_ROTATION_TICKS;
    row->type = type;
    restart_name(row);
    if (!row->length) {
        row->type = type ^ (ARTIST ^ ALBUM);
        restart_name(row);
    }
}

static void render_name(uint32_t slot, uint8_t field)
{
    struct name_row *row = &rows[slot - FIRST_ROW];
    uint32_t track = DATA32(0x008020f0);
    if (row->track != track || row->field != field) {
        row->track = track;
        row->field = field;
        if (track) {
            if (field == CYCLE) {
                restart_cycle(row, ARTIST);
            } else {
                row->type = field;
                restart_name(row);
            }
        }
    }
    if (!track || !row->length) {
        STOCK(func1, 0xb3d94)(slot);
        return;
    }
    struct scroll_cursor *scroll = &row->scroll;
    if ((unsigned)(row->length - scroll->offset) <= LCD_BYTES && row->record[0x56] == NAME_MORE) {
        row->length -= scroll->offset;
        /* Sony's copy routine supports overlap with a lower destination. */
        memcpy(row->text, row->text + scroll->offset, row->length);
        scroll->offset = 0;
        append_chunk(row);
    }
    unsigned remaining = row->length - scroll->offset;
    show_window(slot, row->record, row->text + scroll->offset, remaining);
    /* Keep the final window on screen until both the tail pause and timer expire. */
    if (field == CYCLE && remaining <= LCD_BYTES) {
        if (scroll->pause)
            --scroll->pause;
        else if (!cycle_ticks && row->record[0x56] != NAME_MORE)
            restart_cycle(row, row->type ^ (ARTIST ^ ALBUM));
        return;
    }
    unsigned offset = scroll->offset;
    advance(scroll, remaining,
            glyph_bytes(row->text + offset, row->record[0x62]), LCD_BYTES);
    if (offset && !scroll->offset)
        restart_name(row);
}

void cfw_remote_menu_scroll(uint32_t slot, uint32_t id)
{
    if (slot >= FIRST_ROW && slot <= LAST_ROW &&
        (id & 0xffff) == MENU_RESOURCE_ID(REMOTE_DISPLAY) &&
        (id >> 16) - 2 < 4 && cfw_is_40elk()) {
        uint8_t *rec = STOCK(get_row_record, 0x38d6c)(slot);
        struct scroll_cursor *scroll = &menu_scroll[slot - FIRST_ROW];
        unsigned length = rec[0x61];
        if (length && length <= MAX_CHUNK) {
            if ((rec[0x73] & 2) || scroll->offset >= length) {
                scroll->offset = 0;
                scroll->pause = FIRST_PAUSE;
            }
            unsigned remaining = length - scroll->offset;
            show_window(slot, rec, rec + scroll->offset, remaining);
            advance(scroll, remaining, 1, LCD_BYTES);
            rec[0x73] &= ~2;
            return;
        }
    }
    STOCK(func1, 0xb3e0e)(slot);
}

/* The native menu renderer retains the resource ID in r6 at this callsite. */
__attribute__((naked)) void cfw_remote_menu_scroll_hook(void)
{
    __asm__ volatile (
        ".syntax unified\n"
        "push {r4, lr}\n"
        "movs r1, r6\n"
        "bl cfw_remote_menu_scroll\n"
        "pop {r4, pc}\n"
    );
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
        if (layout >= 1 && layout <= sizeof(layouts) / sizeof(layouts[0]) &&
            device == REMOTE && page == HOME_PAGE &&
            *disc_type == DISC_TYPE_HIMD &&
            id == home_ids[index] && DATA8(0x00800590) == 0 && DATA8(0x00800661) == 3 &&
            !DATA8(0x0080069e) && !DATA8(0x008006a0) && !DATA8(0x008006a2) &&
            STOCK(func, 0x2cc6a)() && STOCK(func, 0x2d07c)() && cfw_is_40elk()) {
            uint8_t field = layouts[layout - 1][index];
            if (field == INFO) {
                rows[index].track = 0;
                STOCK(renderer, 0xb13e6)(device, slot, 0x18);
            } else {
                render_name(slot, field);
            }
            return;
        }
        rows[index].track = 0;
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
