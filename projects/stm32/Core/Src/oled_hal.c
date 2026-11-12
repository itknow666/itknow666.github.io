/* SSD1306 initialization and font source: user's OLED_IIC_SSD1306_original.zip.
 * The transport is adapted to STM32F1 HAL; drawing now uses a bounded framebuffer.
 * Original F4 header and slow single-byte full-screen loops are not used. */
#include "display.h"
#include "board.h"
static uint16_t address;
static uint8_t type;
static int command(const uint8_t *p,uint16_t n) {
    return HAL_I2C_Mem_Write(&hi2c1,address,0x00,I2C_MEMADD_SIZE_8BIT,(uint8_t *)p,n,10)==HAL_OK;
}
int oled_init(uint8_t controller) {
    type=controller?1u:0u;address=0;
    const uint16_t candidates[]={0x3cu<<1,0x3du<<1};
    for(unsigned i=0;i<2;i++) if(HAL_I2C_IsDeviceReady(&hi2c1,candidates[i],2,5)==HAL_OK) {address=candidates[i];break;}
    if(!address)return 0;
    static const uint8_t init[]={0xae,0xd5,0x80,0xa8,0x3f,0xd3,0x00,0x40,
        0xa1,0xc8,0xda,0x12,0x81,0xcf,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0x2e};
    if(!command(init,sizeof(init)))return 0;
    const uint8_t ssd[]={0x20,0x02,0x8d,0x14,0xaf};
    const uint8_t sh[]={0xad,0x8b,0xaf};
    if(!command(type?sh:ssd,type?sizeof(sh):sizeof(ssd)))return 0;
    gfx_clear();return oled_flush();
}
int oled_flush(void) {
    if(!address)return 0;
    for(uint8_t p=0;p<8;p++) {
        uint8_t cmd[]={ (uint8_t)(0xb0u+p),(uint8_t)(type?2u:0u),0x10 };
        if(!command(cmd,sizeof(cmd)))return 0;
        if(HAL_I2C_Mem_Write(&hi2c1,address,0x40,I2C_MEMADD_SIZE_8BIT,&oled_frame[p*128u],128,10)!=HAL_OK)return 0;
    }
    return 1;
}
uint8_t oled_address(void) { return (uint8_t)(address>>1); }
