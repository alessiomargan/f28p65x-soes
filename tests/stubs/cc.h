#ifndef TEST_CC_H
#define TEST_CC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
/* Model C28 octet containers (one octet per 16-bit addressable word). */
#define uint8_t uint16_t
#define CC_PACKED
#define DPRINT(...) ((void)0)
#endif
