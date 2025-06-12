#ifndef __sp_util_h__
#define __sp_util_h__


#define LEN(a) \
  (sizeof((a)) / sizeof((a)[0]))

#define CLAMP(low, high, x) \
  _Generic((x), \
    int32_t: clamp_i32 \
  )((low), (high), (x))


static inline int32_t
clamp_i32 (int32_t low, int32_t high, int32_t x)
{
  if (x <= low)
    return low;
  if (x >= high)
    return high;
  return x;
}

#endif /* !defined(__sp_util_h__) */

