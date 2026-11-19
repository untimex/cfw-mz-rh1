#pragma once
#include <stdint.h>

enum { CHINESE_OFF, CHINESE_PINYIN, CHINESE_ORIGINAL, CHINESE_SKIP };
#define OLED_NAME_LIMIT 128

void chinese_display_invalidate(void);
int chinese_display_prepare(void);
const char *chinese_display_field(unsigned item);
