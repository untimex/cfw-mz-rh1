#include <stdint.h>
#include "fw/tracks.h"
#include "remote_sjis.h"
#include "remote_sjis_data.inc"
#include "remote_traditional.inc"

enum { TRAILS = 188, STANDARD_ROWS = 42, UTF16BE = 0x84, SHIFT_JIS = 0x90 };

static unsigned find_in_row(unsigned row, unsigned value, int by_codepoint)
{
    unsigned offset = sjis_rows[row];
    const uint8_t *p = offset & 0x8000 ? sjis_prefix + (offset & 0x7fff)
                                       : sjis_suffix + offset;
    unsigned cp = 0;
    for (unsigned col = 0; col < TRAILS; ++col) {
        unsigned code = *p++;
        if (code >= 0xc0) {
            cp += code - 0xd0;
        } else if (code >= 0x80) {
            cp += ((code >> 3) & 7) + 1;
            if (by_codepoint ? cp == value : col == value)
                return by_codepoint ? col + 1 : cp;
            ++col;
            cp += (code & 7) + 1;
        } else if (code == 0x7e) {
            unsigned skip = *p++;
            if (!by_codepoint && value < col + skip)
                return 0;
            col += skip - 1;
            cp = 0;
        } else if (code == 0x7f) {
            cp = (unsigned)*p << 8 | p[1];
            p += 2;
        } else {
            cp = (code << 8 | *p++) + 0x4dff;
        }
        if (by_codepoint ? cp == value : col == value)
            return by_codepoint ? col + 1 : cp;
    }
    return 0;
}

unsigned cfw_sjis_to_unicode(unsigned pair)
{
    unsigned lead = pair >> 8;
    unsigned trail = pair & 255;
    unsigned col = trail - 0x40 - (trail > 0x7f);
    if (lead >= 0xfa) {
        /* CP932's IBM extension duplicates its NEC-selected IBM characters. */
        unsigned index = (lead - 0xfa) * TRAILS + col - 28;
        if (index < 188)
            return find_in_row(42, index, 0);
        return index < 376 ? find_in_row(43, index - 188, 0) : 0;
    }
    unsigned row = lead <= 0x9f ? lead - 0x81 : lead - 0xe0 + 31;
    if (lead >= 0xed)
        row -= 2;
    return find_in_row(row, col, 0);
}

static unsigned unicode_to_sjis(unsigned cp)
{
    if (cp >= 0x20 && cp < 0x7f)
        return cp;
    if (cp >= 0xff61 && cp <= 0xff9f)
        return cp - 0xff61 + 0xa1;
    /* JIS and Windows assign different Unicode names to these symbols. */
    switch (cp) {
    case 0x301c: return 0x8160;
    case 0x2016: return 0x8161;
    case 0x2212: return 0x817c;
    case 0x00a2: return 0x8191;
    case 0x00a3: return 0x8192;
    case 0x00ac: return 0x81ca;
    }
    unsigned page = cp >> 8;
    for (unsigned row = 0; row < STANDARD_ROWS; ++row) {
        if (page < sjis_pages[2*row] || page > sjis_pages[2*row + 1])
            continue;
        unsigned position = find_in_row(row, cp, 1);
        if (position) {
            unsigned lead = row < 31 ? row + 0x81 : row - 31 + 0xe0;
            unsigned trail = position + 0x3f + (position >= 64);
            return lead << 8 | trail;
        }
    }
    unsigned low = 0, high = sizeof(traditional_sjis) / sizeof(traditional_sjis[0]);
    while (low < high) {
        unsigned mid = (low + high) >> 1;
        unsigned source = traditional_sjis[mid][0];
        if (cp == source)
            return traditional_sjis[mid][1];
        if (cp < source)
            high = mid;
        else
            low = mid + 1;
    }
    return '.';
}

typedef void format_name(unsigned, uint8_t *, uint16_t *, uint8_t *, uint8_t *);

void cfw_format_remote_name(unsigned caps, uint8_t *text, uint16_t *length,
                            uint8_t *state, uint8_t *encoding, unsigned device)
{
    /* Keep the raw capabilities untouched. 55ELK also reports one line. */
    if (device == 2 && *encoding == UTF16BE && *length && *length <= 0x44 &&
        (*(volatile const uint8_t *)0x8004dd & 1) &&
        *(volatile const uint16_t *)0x800506 == 0x4d52 &&
        *(volatile const uint32_t *)0x800508 == 0x42323043) {
        unsigned written = 0;
        for (unsigned i = 0; i + 1 < *length; i += 2) {
            unsigned cp = (unsigned)text[i] << 8 | text[i + 1];
            if (!cp)
                break;
            if (cp >= 0xd800 && cp < 0xdc00 && i + 3 < *length &&
                (text[i + 2] & 0xfc) == 0xdc)
                i += 2;
            unsigned pair = unicode_to_sjis(cp);
            if (pair > 0xff)
                text[written++] = pair >> 8;
            text[written++] = pair;
        }
        text[written] = 0;
        *length = written;
        *encoding = SHIFT_JIS;
        return;
    }
    ((format_name *)THUMB_ADDR(0x3c0d0))(caps, text, length, state, encoding);
}

/* Both native call sites save the display device at sp+0x28. */
__attribute__((naked)) void cfw_format_remote_name_hook(void)
{
    __asm__ volatile (
        ".syntax unified\n"
        "push {r4, lr}\n"
        "ldr r4, [sp, #0x30]\n"
        "push {r4}\n"
        "ldr r4, [sp, #12]\n"
        "push {r4}\n"
        "bl cfw_format_remote_name\n"
        "add sp, #8\n"
        "pop {r4, pc}\n"
    );
}
