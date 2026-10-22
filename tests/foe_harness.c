#include <string.h>
#include <assert.h>
#include "esc.h"
#include "boot/boot_image.h"

struct test_esc ESCvar;
struct test_mbx MBXcontrol[3];
esc_octet_t MBX[3 * ESC_MBXSIZE];
uint16_t ESC_claimbuffer(void) {return 1U;}

/* Include production units so tests exercise the real callbacks/dispatcher. */
#include "esc_foe.c"
#include "foe_utils.c"

foe_cfg_t gFOE_config;
uint8_t foe_buffer[0x400];
static uint16_t flash_words[BOOT_SLOT_WORDS];
static int fail_program, fail_erase, program_calls, erase_calls, close_calls;
static int marker_cut = -1;
static uint16_t read_word(uint32_t a) {assert(a>=BOOT_APP_WORD_ADDR && a<BOOT_APP_WORD_ADDR+BOOT_SLOT_WORDS);return flash_words[a-BOOT_APP_WORD_ADDR];}
Fapi_StatusType Program_dataFlashSector(const uint16_t *s,uint32_t a,uint32_t n) {
    ++program_calls;
    assert(a>=BOOT_APP_WORD_ADDR && a+n<=BOOT_APP_WORD_ADDR+BOOT_SLOT_WORDS);
    if (fail_program) {fail_program=0;return 1U;}
    for(uint32_t i=0;i<n;i++) {
        if (a==BOOT_COMMIT_WORD_ADDR && marker_cut>=0 && (int)i>=marker_cut) {return 1U;}
        flash_words[a-BOOT_APP_WORD_ADDR+i] &= s[i];
    }
    return 0U;
}
Fapi_StatusType Erase_dataFlashSector(uint32_t a,uint32_t n) {
    ++erase_calls;
    assert(a>=BOOT_APP_WORD_ADDR && a+n<=BOOT_APP_WORD_ADDR+BOOT_SLOT_WORDS);
    if(fail_erase) {fail_erase=0;return 1U;}
    for(uint32_t i=0;i<n;i++) flash_words[a-BOOT_APP_WORD_ADDR+i]=0xFFFFU;
    return 0U;
}
static bool program(const uint16_t*s,uint32_t a,uint32_t n) {return Program_dataFlashSector(s,a,n)==0U;}
static bool erase(uint32_t a,uint32_t n) {return Erase_dataFlashSector(a,n)==0U;}
static const boot_flash_io_t io={read_word,program,erase};
#if FOE_UPDATE_INTEGRATED
uint32_t foe_update_prepare(void) {return boot_image_prepare(&io)?0U:1U;}
uint32_t foe_update_validate_and_stage(uint32_t a,uint32_t n) {
    ++close_calls;return a==BOOT_APP_WORD_ADDR && boot_image_finish(&io,n)?0U:1U;
}
#endif
void test_reset(int erase_flash) {
    if(erase_flash) memset(flash_words,255,sizeof(flash_words));
    memset(MBX,0,sizeof(MBX));memset(MBXcontrol,0,sizeof(MBXcontrol));
    ESCvar=(struct test_esc){1,0,ESCboot};
    firmware_write_open=false;programmed_octets=0;
    fail_program=fail_erase=program_calls=erase_calls=close_calls=0;marker_cut=-1;
    bootstrap_foe_init();FOE_init();
}
static uint32_t send(uint16_t op,uint32_t val,const unsigned char*d,uint32_t n) {
    assert(n<=ESC_FOE_DATA_SIZE);
    memset(MBX,0,sizeof(MBX));
    esc_mbx_set_length(MBX,n+ESC_FOEHSIZE);esc_mbx_set_type(MBX,MBXFOE);
    foe_set_header(MBX,op,val);
    for(uint32_t i=0;i<n;i++) MBX[FOE_O_DATA+i]=d[i];
    MBXcontrol[0].state=MBXstate_inclaim;
    ESC_foeprocess();
    return MBX[ESC_MBXSIZE+FOE_O_OPCODE];
}
uint32_t test_request(const char*n,uint32_t pass) {return send(FOE_OP_WRQ,pass,(const unsigned char*)n,strlen(n));}
uint32_t test_request_n(const char*n,uint32_t count) {return send(FOE_OP_WRQ,0xF280,(const unsigned char*)n,count);}
uint32_t test_data(const unsigned char*d,uint32_t n,uint32_t packet) {return send(FOE_OP_DATA,packet,d,n);}
uint32_t test_error(void) {return send(FOE_OP_ERR,0,NULL,0);}
uint32_t test_valid(void) {return boot_image_valid(&io,true);}
uint32_t test_finish(uint32_t n) {return boot_image_finish(&io,n);}
uint32_t test_count(int which) {return which==0?program_calls:which==1?erase_calls:close_calls;}
void test_fault(int which) {if(which==0)fail_program=1;else fail_erase=1;}
void test_corrupt(uint32_t word) {flash_words[word]^=1U;}
void test_marker_cut(int n) {marker_cut=n;}
void test_link_reset(void) {ESCvar.MBXrun=0;ESC_foeprocess();ESCvar.MBXrun=1;}
void test_state(uint16_t n) {ESCvar.ALstatus=n;}
