#ifndef __util_h__
#define __util_h__

#include <stdint.h>
#include <string.h>

#define IS_BIG_ENDIAN 0

#define LEN(a) \
  (sizeof((a)) / sizeof((a)[0]))


typedef enum SpClientPacketType_e : uint8_t {
  SP_CLPKT_JOIN,
  SP_CLPKT_LEAVE,
  SP_CLPKT_MOVE,
} SpClientPacketType;

typedef enum SpServerPacketType_e : uint8_t {
  SP_SVPKT_BADREQ,
  SP_SVPKT_INIT,
  SP_SVPKT_SNAPSHOT,
  SP_SVPKT_GOODBYE,
} SpServerPacketType;

typedef enum SpServerPacketGoodbyeReason_e {
  SP_SVPKT_GOODBYE_TOOMANY,
  SP_SVPKT_GOODBYE_ALREADYHERE,
  SP_SVPKT_GOODBYE_LEAVE,
} SpServerPacketGoodbyeReason;


#if !IS_BIG_ENDIAN
static inline void
encode_bigend_u64 (uint64_t value, void* dest)
{
  value =
      ((value & 0xFF00000000000000u) >> 56u) |
      ((value & 0x00FF000000000000u) >> 40u) |
      ((value & 0x0000FF0000000000u) >> 24u) |
      ((value & 0x000000FF00000000u) >>  8u) |
      ((value & 0x00000000FF000000u) <<  8u) |      
      ((value & 0x0000000000FF0000u) << 24u) |
      ((value & 0x000000000000FF00u) << 40u) |
      ((value & 0x00000000000000FFu) << 56u);
  memcpy(dest, &value, sizeof(uint64_t));
}

static inline void
encode_bigend_u16 (uint16_t value, void *dest)
{
  value =
      ((value && 0xFF00) >> 8) |
      ((value && 0x00FF) << 8);
  memcpy(dest, &value, sizeof(uint16_t));
}

#else
static inline void
encode_bigend_u64 (uint64_t value, void *dest)
{
  *(uint64_t *)dest = value;
}

static inline void
encode_bigend_u16 (uint16_t value, void *dest)
{
  *(uint16_t *)dest = value;
}
#endif /* !IS_BIG_ENDIAN */

#define MAX_PLAYERS 16
#endif
