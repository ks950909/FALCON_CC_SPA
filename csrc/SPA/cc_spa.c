#include <stdint.h>
#include <stdlib.h>
#include "simpleserial.h"
#include "hal.h"

void loaduint32(uint8_t *arr,int idx,uint32_t *num)
{
    int i;
    *num = 0;
    for(i = 0 ; i < 4 ; i++)
    {
        *num += (uint32_t)arr[idx + i] << (8 * (3 - i));
    }
}
void saveuint32(uint8_t *arr,int idx,uint32_t num)
{
    int i;
    for(i = 0 ; i < 4 ; i++)
    {
        arr[i+idx] = ((num >> (8 * (3-i))) & 0xff);
    }
}

uint8_t cc(uint8_t* key_buf, uint8_t len)
{
    uint32_t x,y,a,output;
    loaduint32(key_buf,0,&x);
    loaduint32(key_buf,4,&y);
    loaduint32(key_buf,8,&a);
    trigger_high();
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    output = x ^ ((x ^ y) & (a));
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    trigger_low();
    saveuint32(key_buf,0,output);
    simpleserial_put('r', 4, key_buf);
    return 0;
}

uint8_t cc_m1(uint8_t* key_buf, uint8_t len)
{
    uint32_t x,y,a,output;
    loaduint32(key_buf,0,&x);
    loaduint32(key_buf,4,&y);
    loaduint32(key_buf,8,&a);
    trigger_high();
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    output = 2 * x - y + ((y - x) << a);
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    trigger_low();
    saveuint32(key_buf,0,output);
    simpleserial_put('r', 4, key_buf);
    return 0;
}

uint8_t cc_m2(uint8_t* key_buf, uint8_t len)
{
    uint32_t x,y,a,b,c,output;
    loaduint32(key_buf,0,&x);
    loaduint32(key_buf,4,&y);
    loaduint32(key_buf,8,&a);
    loaduint32(key_buf,12,&b);
    c = b ^ (-1);
    trigger_high();
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    output = (x ^ ((x ^ y) & b)) ^ ((x ^ y) & (2 * b - c + ((c-b) << a)));
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    __asm__ __volatile__ ("NOP");
    trigger_low();
    saveuint32(key_buf,0,output);
    simpleserial_put('r', 4, key_buf);
    return 0;
}


int main(void)
{
    platform_init();
	init_uart();
	trigger_setup();

	simpleserial_init();  // 'v': check_version, 'y': ss_num_commands, 'w': ss_get_commands
	simpleserial_addcmd('a', 12, cc);
	simpleserial_addcmd('b', 12, cc_m1);
	simpleserial_addcmd('c', 16, cc_m2);

	while(1)
		simpleserial_get();
}