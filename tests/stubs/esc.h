#ifndef TEST_ESC_H
#define TEST_ESC_H
#include "cc.h"
typedef uint16_t esc_octet_t;
#define ESC_MBXHSIZE 6U
#define ESC_FOEHSIZE 6U
#define ESC_MBXSIZE 128U
#define ESC_FOE_DATA_SIZE 116U
#define MBXFOE 4U
#define FOE_OP_RRQ 1U
#define FOE_OP_WRQ 2U
#define FOE_OP_DATA 3U
#define FOE_OP_ACK 4U
#define FOE_OP_ERR 5U
#define FOE_READY 0U
#define FOE_WAIT_FOR_DATA 1U
#define FOE_ERR_NOTFOUND 0x8001U
#define FOE_ERR_NORIGHTS 0x8002U
#define FOE_ERR_NOTINBOOTSTRAP 0x8003U
#define FOE_ERR_PROGERROR 0x8004U
#define FOE_ERR_ACCESS 0x8005U
#define FOE_ERR_ILLEGAL 0x8006U
#define FOE_ERR_PACKETNO 0x8007U
#define FOE_ERR_DISKFULL 0x8008U
#define FOE_ERR_NOTDEFINED 0x8009U
#define MBXERR_SIZETOOSHORT 0x8010U
#define ESCboot 3U
#define ESCpreop 2U
#define MBXstate_inclaim 1U
#define MBXstate_outreq 2U
#define MBXstate_idle 0U
extern struct test_esc {uint16_t MBXrun, xoe, ALstatus;} ESCvar;
extern struct test_mbx {uint16_t state;} MBXcontrol[3];
extern esc_octet_t MBX[3 * ESC_MBXSIZE];
extern uint16_t ESC_claimbuffer(void);
static inline uint16_t esc_octet_get(const esc_octet_t *p, uint32_t n) {return p[n] & 255U;}
static inline void esc_octet_set(esc_octet_t *p,uint32_t n,uint16_t v) {p[n]=v & 255U;}
static inline uint32_t esc_get_le32(const esc_octet_t *p,uint32_t n) {
 return (uint32_t)p[n] | ((uint32_t)p[n+1]<<8) | ((uint32_t)p[n+2]<<16) | ((uint32_t)p[n+3]<<24);
}
static inline void esc_put_le32(esc_octet_t *p,uint32_t n,uint32_t v) {for(uint16_t i=0;i<4;i++) p[n+i]=(v>>(i*8))&255U;}
static inline uint16_t esc_mbx_length(const esc_octet_t *p) {return p[0]|(p[1]<<8);}
static inline void esc_mbx_set_length(esc_octet_t *p,uint16_t n) {p[0]=n&255U;p[1]=n>>8;}
static inline uint16_t esc_mbx_type(const esc_octet_t *p) {return p[5];}
static inline void esc_mbx_set_type(esc_octet_t *p,uint16_t n) {p[5]=n;}
#endif
