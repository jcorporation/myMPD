// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#ifndef __UTF8DECODE_H__
#define __UTF8DECODE_H__

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

#include <inttypes.h>
#include <stddef.h>

uint32_t decode_utf8(uint32_t* state, uint32_t* codep, uint32_t byte);
int check_utf8(uint8_t* s, size_t len);
#endif
