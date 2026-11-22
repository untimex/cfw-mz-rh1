#include "chinese_display.h"
#include "cfw_config.h"
#include "track.h"
#include "han_readings.inc"
#include "remote_sjis.h"
#include "sjis_reading_exceptions.inc"

enum { UTF16BE = 0x84, SHIFT_JIS = 0x90 };
typedef int raw_begin(void *, uint8_t *, unsigned, unsigned, int, int);
typedef int raw_next(void *, uint8_t *);
typedef int md_wide(unsigned, unsigned, unsigned, char *, unsigned, int *, unsigned);
#define NATIVE(type, address) ((type*)THUMB_ADDR(address))

static struct {
    unsigned track, group, disc, layout[2];
    uint8_t mode, with_han, valid;
    char names[5][OLED_NAME_LIMIT + 1];
} cache;

struct writer {
    char *out;
    unsigned length;
    uint8_t found, full, after_han, mode;
    unsigned previous;
};

static unsigned packed_id(const uint8_t *data, unsigned i) {
    unsigned offset = i + (i >> 3);
    return ((data[offset] | (data[offset+1] << 8)) >> (i & 7)) & 511;
}

static unsigned popcount32(uint32_t v) {
    v -= (v >> 1) & 0x55555555;
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0f0f0f0f;
    return (v * 0x01010101) >> 24;
}

static unsigned unicode_reading(unsigned cp) {
    if (cp < 0x4e00 || cp > 0x9fff) return 0;
    unsigned n = cp - 0x4e00;
    uint32_t mask = han_mask[n >> 5], bit = 1u << (n & 31);
    if (!(mask & bit)) return 0;
    unsigned block = n >> 8;
    unsigned index = han_page[block] + popcount32(mask & (bit-1));
    for (unsigned i = block << 3; i < (n >> 5); ++i)
        index += popcount32(han_mask[i]);
    return packed_id(han_readings, index);
}

static unsigned sjis_reading(unsigned pair) {
    for (unsigned i = 0; i < sizeof(sjis_reading_exceptions) / sizeof(uint16_t); i += 2)
        if (sjis_reading_exceptions[i] == pair)
            return sjis_reading_exceptions[i+1];
    return unicode_reading(cfw_sjis_to_unicode(pair));
}

static void append(struct writer *w, const char *text, unsigned len) {
    if (w->full) return;
    if (len > OLED_NAME_LIMIT - 3 - w->length) {
        memcpy(w->out+w->length, "...", 4);
        w->full = 1;
        return;
    }
    memcpy(w->out+w->length, text, len);
    w->length += len;
    w->out[w->length] = 0;
}

static int ascii_word(unsigned c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static void emit_han(struct writer *w, unsigned key, const char *syllable) {
    w->found = 1;
    if (w->mode != CHINESE_PINYIN) return;
    /* A few common polyphonic words in music metadata, in both encodings.
       The identical-length replacement for zh/ch keeps the stream bounded. */
    if (!w->full && w->previous == 0x91cd && (key == 0x5e86 || key == 0x6176) && w->length>=5)
        w->out[w->length-5] = 'c';
    if (w->previous == 0x97f3 && (key==0x4e50 || key==0x6a02)) syllable="yue";
    if ((w->previous==0x94f6 || w->previous==0x9280) && key==0x884c) syllable="hang";
    w->previous=key;
    if (w->length && (w->after_han || ascii_word(w->out[w->length-1]))) append(w," ",1);
    append(w,syllable,strlen(syllable));
    w->after_han = 1;
}

static void emit_plain(struct writer *w, unsigned cp) {
    if (cp == 0xfeff) return;
    w->previous=0;
    if (cp >= 0xff01 && cp <= 0xff5e) cp -= 0xfee0;
    if (cp == 0x3000) cp = ' ';
    char c = cp >= 0x20 && cp <= 0x7e ? cp : '_';
    if (w->after_han && ascii_word(c)) append(w, " ", 1);
    append(w, &c, 1);
    w->after_han = 0;
}

static void emit_unicode(struct writer *w, unsigned cp) {
    if (cp == 0x3007) { emit_han(w, cp, "ling"); return; }
    if ((cp >= 0x3400 && cp <= 0x4dbf) || (cp >= 0x4e00 && cp <= 0x9fff) ||
        (cp >= 0xf900 && cp <= 0xfaff) || (cp >= 0x20000 && cp <= 0x2a6df) ||
        (cp >= 0x2a700 && cp <= 0x2ee5f) || (cp >= 0x2f800 && cp <= 0x2fa1f) ||
        (cp >= 0x30000 && cp <= 0x3347f)) {
        unsigned i = unicode_reading(cp);
        emit_han(w, cp, i ? spellings + spelling_offset[i] : "?");
    } else emit_plain(w, cp);
}

static int sjis_lead(unsigned c) {
    return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}

static void sjis_byte(struct writer *w, unsigned *lead, unsigned c) {
    if (*lead) {
        unsigned pair = (*lead << 8) | c;
        *lead = 0;
        if (c >= 0x40 && c <= 0xfc && c != 0x7f) {
            if (pair == 0x8157 || pair == 0x815a) { emit_han(w, 0, pair==0x8157?"tong":"ling"); return; }
            if ((pair >= 0x889f && pair <= 0x9872) || (pair >= 0x989f && pair <= 0x9ffc) ||
                (pair >= 0xe040 && pair <= 0xeaa4) || (pair >= 0xed40 && pair <= 0xeeec) ||
                (pair >= 0xfa5c && pair <= 0xfc4b)) {
                unsigned i = sjis_reading(pair);
                static const uint16_t phrase_keys[][2] = {
                    {0x8f64,0x91cd},{0x8c63,0x6176},{0x89b9,0x97f3},
                    {0x9ed9,0x6a02},{0x8be2,0x9280},{0x8d73,0x884c}
                };
                unsigned key=0;
                for (unsigned n=0; n<sizeof(phrase_keys)/sizeof(phrase_keys[0]); n++)
                    if (pair==phrase_keys[n][0]) key=phrase_keys[n][1];
                emit_han(w, key, i ? spellings + spelling_offset[i] : "?");
            } else if (pair >= 0x824f && pair <= 0x8258) emit_plain(w, '0'+pair-0x824f);
            else if (pair >= 0x8260 && pair <= 0x8279) emit_plain(w,'A'+pair-0x8260);
            else if (pair >= 0x8281 && pair <= 0x829a) emit_plain(w,'a'+pair-0x8281);
            else emit_plain(w, pair == 0x8140 ? ' ' : '_');
            return;
        }
        emit_plain(w, '_');
    }
    if (sjis_lead(c)) *lead = c;
    else emit_plain(w,c);
}

static void scan_himd(struct writer *w, unsigned track, unsigned type) {
    unsigned encoding = NATIVE(func2,0x31f46)(type,track);
    if (encoding != UTF16BE && encoding != SHIFT_JIS) return;
    /* Same raw 28-byte iterator that Sony's 0x31768 uses before converting
       the name. A private cursor avoids touching its shared display buffer. */
    uint32_t cursor[7];
    memset(cursor,0,sizeof(cursor));
    uint8_t source = 0, byte;
    if (NATIVE(raw_begin,0x31dac)(cursor,&source,track,type,0,1) < 0) return;
    unsigned lead = 0, high = 0, first = 0;
    for (unsigned n=0; n<223; n++) {
        int more = NATIVE(raw_next,0x31ea4)(cursor,&byte);
        if (encoding == SHIFT_JIS) {
            if (!byte) break;
            sjis_byte(w,&lead,byte);
        } else if (!(n & 1)) {
            first = byte;
        } else {
            unsigned cp = (first << 8) | byte;
            if (!cp) break;
            if (high && cp >= 0xdc00 && cp <= 0xdfff) {
                cp = 0x10000 + ((high-0xd800)<<10) + cp-0xdc00;
                high = 0;
            } else if (high) { emit_plain(w,'_'); high=0; }
            if (cp >= 0xd800 && cp <= 0xdbff) high=cp;
            else emit_unicode(w,cp);
        }
        if (more <= 0 || (w->found && w->mode == CHINESE_ORIGINAL)) break;
    }
}

static void scan_md(struct writer *w, unsigned track, unsigned group) {
    unsigned lead = 0;
    /* Sony's wide-title reader accepts byte offsets, a byte count and
       a destination capacity including its terminator. */
    for (unsigned offset=0; offset<2048; offset+=28) {
        char bytes[29];
        int more = 0;
        int result = NATIVE(md_wide,0x30e14)(track,offset,28,bytes,sizeof(bytes),&more,group);
        for (unsigned j=0; j<28 && bytes[j]; j++) sjis_byte(w,&lead,(uint8_t)bytes[j]);
        if (result || !more || !bytes[0] || (w->found && w->mode == CHINESE_ORIGINAL)) break;
    }
}

void chinese_display_invalidate(void) { cache.valid = 0; }

static unsigned visible_fields(unsigned layout, unsigned defaults) {
    if ((layout & OLED_HANDLER_MARKER) == OLED_HANDLER_MARKER) return 0;
    layout &= TRACK_ITEM_MASK;
    if (!layout) layout = defaults;
    unsigned fields=0;
    while (layout) {
        unsigned item = layout & 15;
        if (item >= TRACK_ITEM_TRACK_NAME && item <= TRACK_ITEM_ALBUM_NAME) fields |= 1u << (item-1);
        layout >>= 4;
    }
    return fields;
}

int chinese_display_prepare(void) {
    unsigned mode=cfw_config.chinese_mode;
    unsigned track=*current_track, group=*current_group, disc=*disc_type;
    unsigned left=cfw_config.track_layout_display[0], right=cfw_config.track_layout_display[1];
    if (!mode || !track || disc>DISC_TYPE_MD || *operation_state==1) {
        cache.valid=0;
        return 0;
    }
    if (!cache.valid || track!=cache.track || group!=cache.group || disc!=cache.disc ||
        mode!=cache.mode || left!=cache.layout[0] || right!=cache.layout[1]) {
        cache.track=track; cache.group=group; cache.disc=disc; cache.mode=mode;
        cache.layout[0]=left; cache.layout[1]=right; cache.with_han=0;
        unsigned fields=visible_fields(left,DEFAULT_DISPLAY_0_LAYOUT) | visible_fields(right,DEFAULT_DISPLAY_1_LAYOUT);
        static const uint8_t types[] = {0,3,4,1,2}; /* title, group, disc, artist, album */
        for (unsigned i=0; i<5; i++) {
            if (!(fields & (1u<<i))) continue;
            struct writer w;
            memset(&w,0,sizeof(w));
            w.out=cache.names[i]; w.mode=mode;
            w.out[0]=0;
            unsigned id = i==2 ? 0 : track;
            if (disc==DISC_TYPE_HIMD) {
                if (i==1) id=get_group_id_for_himd_track(track);
                scan_himd(&w,id,types[i]);
            } else if (i==0 || i==2 || (i==1 && group)) scan_md(&w,id,i==1);
            if (w.found) {
                cache.with_han |= 1u<<i;
                if (mode==CHINESE_ORIGINAL) break;
            }
        }
        cache.valid=1;
        track_view_state.invalidated=1;
    }
    return mode==CHINESE_ORIGINAL && cache.with_han;
}

const char *chinese_display_field(unsigned item) {
    if (!cache.valid || cfw_config.chinese_mode==CHINESE_OFF || !(cache.with_han & (1u<<(item-1)))) return 0;
    return cache.names[item-1];
}
