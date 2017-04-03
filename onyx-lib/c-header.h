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

void appendString(string lhs, string rhs)
{
  u32 newLen = lhs.len + rhs.len;
  if(newLen > lhs.cap)
  {
    lhs.buf = realloc(lhs.buf, newLen);
    lhs.cap = newLen;
  }
  memcpy(lhs.buf + lhs.len, rhs.buf, rhs.len);
  lhs.len = newLen;
}

string disposeString_(string s)
{
  free(s.buf);
}

