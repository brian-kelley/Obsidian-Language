#include "stdio.h"
#include "stdint.h"

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;

typedef struct
{
  char* buf;
  u32 len;
  u32 cap;
} string;

string initEmptyString_()
{
  string s;
  s.buf = malloc(16);
  s.len = 0;
  s.cap = 16;
  return s;
}

string initString_(const char* init, u32 len)
{
  string s;
  s.cap = len > 16 : len : 16;
  s.buf = malloc(s.cap);
  memcpy(s.buf, init, len);
  s.len = len;
  return s;
}

void appendString_(string* lhs, string* rhs)
{
  u32 newLen = lhs->len + rhs->len;
  if(newLen > lhs->cap)
  {
    lhs->buf = realloc(lhs->buf, newLen);
    lhs->cap = newLen;
  }
  memcpy(lhs->buf + lhs->len, rhs->buf, rhs->len);
  lhs->len = newLen;
}

void prependStringLiteral_(string* str, const char* literal, u32 len)
{
  u32 newLen = str->len + len;
  if(newLen > str->cap)
  {
    str->buf = realloc(str->buf, newLen);
    str->cap = newLen;
  }
  memcpy(str->buf + str->len, str->buf, str->len);
  memcpy(str->buf, literal, len);
  str->len = newLen;
}

void appendStringLiteral_(string* str, const char* literal, u32 len)
{
  u32 newLen = str->len + len;
  if(newLen > str->cap)
  {
    str->buf = realloc(str->buf, newLen);
    str->cap = newLen;
  }
  memcpy(str->buf + str->len, literal, len);
  str->len = newLen;
}

void appendStringChar_(string* str, char c)
{
  if(str->len == str->cap)
  {
    str->buf = realloc(str->buf, str->cap + 1);
    str->cap++;
  }
  str->buf[str->len] = c;
  str->len++;
}

string disposeString_(string* s)
{
  free(s->buf);
}

