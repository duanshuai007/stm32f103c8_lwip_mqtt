#include "flash.h"
#include "stdio.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"

#define TYPE    W25X16
/*
*       2M Byte(16M bit)  
*       256 bytes page program   
*       sector erase for 4k
*       block erase for 32k
*       block erase for 64k
*       chip erase for all
*/
#if 0
#define SET_CS_ENABLE()         HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
#define SET_CS_DISABLE()        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);        
#else
#define SET_CS_ENABLE()         LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12)
#define SET_CS_DISABLE()        LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12)
#endif

static uint8_t spi_ll_wr(uint8_t dat)
{
  uint8_t retry=0;
  while(!LL_SPI_IsActiveFlag_TXE(SPI2)){
    retry++;
    if (retry > 200)
      return 0;
  }
  LL_SPI_TransmitData8(SPI2, dat);
  
  retry = 0;
  while(!LL_SPI_IsActiveFlag_RXNE(SPI2)){
    retry++;
    if (retry > 200)
      return 0;
  }
  return LL_SPI_ReceiveData8(SPI2);
}

/*
void w25x16_ReadJEDECID(void)       // FlashID is 0xEF3015,  Manufacturer Device ID is 0x14
{
    uint8_t cmd = READ_JEDEC_ID;
    uint8_t buffer[3];

    SET_CS_ENABLE();

    spi_ll_wr(cmd);
    buffer[0] = spi_ll_wr(0xaa);
    buffer[1] = spi_ll_wr(0xaa);
    buffer[2] = spi_ll_wr(0xaa);

    SET_CS_DISABLE();
    
//    printf("jedec id: %02x %02x %02x\r\n", buffer[0], buffer[1], buffer[2]);
}

void w25x16_ReadManufacturer(void)       // FlashID is 0xEF3015,  Manufacturer Device ID is 0x14
{
  uint8_t cmd[4] = {READ_MANUID, 0, 0, 0};
  uint8_t buffer[2];
  uint8_t i;
  
  SET_CS_ENABLE();
  for(i = 0; i < 4; i++) {
    spi_ll_wr(cmd[i]);
  }
  buffer[0] = spi_ll_wr(0);
  buffer[1] = spi_ll_wr(0);
  
  SET_CS_DISABLE();
  
//  printf("recv: %02x %02x\r\n", buffer[0], buffer[1]);
}
*/

void w25x16_ReadUniqueID(uint8_t *buf)
{
  uint8_t cmd[5] = {0x4b, 0, 0, 0, 0};
  uint8_t i;
     
  SET_CS_ENABLE();
  
  for(i=0;i<5;i++) {
    spi_ll_wr(cmd[i]);
  }
  for(i=0;i<8;i++) {
    buf[i] = spi_ll_wr(0);
  }

  SET_CS_DISABLE();

//  printf("unique id:");
//  for (i=0;i<8;i++) {
//    printf("%02x ", buf[i]); 
//  }
//  printf("\r\n");
}

/*
void w25x16_ReadDeviceID(void)
{
  uint8_t cmd[5] = {0xab, 0, 0, 0};
  uint8_t id;
     uint8_t i;
     
  SET_CS_ENABLE();
  
  for(i=0;i<4;i++) {
    spi_ll_wr(cmd[i]);
  }
  id = spi_ll_wr(0);

  SET_CS_DISABLE();
  
//  printf("device id:%02x \r\n", id);
}

//         7 ==> 0     
//      SRP0    SEC     TB      BP2     BP1     BP0     WEL     BUSY
uint8_t w25x16_readstatus1(void)
{
    uint8_t status;
    SET_CS_ENABLE();
    spi_ll_wr(STATUS_REG1);
    status = spi_ll_wr(0);
    SET_CS_DISABLE();
    return status;
}
static void set_write_enable(void)
{
  SET_CS_ENABLE();
  spi_ll_wr(WRITE_ENABLE);
  SET_CS_DISABLE();
}

static void set_write_disable(void)
{
  SET_CS_ENABLE();
  spi_ll_wr(WRITE_DISABLE);
  SET_CS_DISABLE();
}

void w25x16_eraseall(void)
{
  set_write_enable();
  SET_CS_ENABLE();
  spi_ll_wr(ERASE_ALL);
  SET_CS_DISABLE();
  set_write_disable();
}

void w25x16_readstatus2(void)
{
    uint8_t status;
    
    SET_CS_ENABLE();
    spi_ll_wr(STATUS_REG2);
    status = spi_ll_wr(0);
    SET_CS_DISABLE();
    
//    printf("status2 = %02x \r\n", status);
}

static void w25x16_is_busy(void)
{
  uint8_t status;
  SET_CS_ENABLE();
  spi_ll_wr(STATUS_REG1);
  do {
    status = spi_ll_wr(0);
  } while(status & 0x1);
  SET_CS_DISABLE();
}

void w25x16_sector_erase(uint32_t address)
{
  uint8_t head[4] = {SECTOR_ERASE, 0,0,0};
  uint8_t i;

  set_write_enable();
  head[1] = (address >> 16) & 0xff;
  head[2] = (address >> 8) & 0xff;
  head[3] = address & 0xff;
  w25x16_is_busy();
  
  SET_CS_ENABLE();
  
  for(i=0;i<4;i++) {
    spi_ll_wr(head[i]);
  }
  
  SET_CS_DISABLE();
  w25x16_is_busy();
}

void w25x16_read_data(uint32_t address, uint8_t *buf, uint8_t len)
{
  uint8_t head[4];
  uint8_t i;
  
  w25x16_is_busy();
  
  head[0] = READ_DATA;
  head[1] = (address >> 16) & 0xff;
  head[2] = (address >> 8) & 0xff;
  head[3] = address & 0xff;
  SET_CS_ENABLE();
//  HAL_Delay(1);
  LL_mDelay(1);
  for(i=0;i<4;i++) {
      spi_ll_wr(head[i]);
  }
  for(i=0;i<len;i++){
    buf[i] = spi_ll_wr(0xaa);
  }
  SET_CS_DISABLE();
  
//  printf("read:");
//  for (i=0;i<len;i++) {
//    printf("%02x ", buf[i]);
//  }
//  printf("\r\n");
}

void w25x16_write_data(uint32_t address, uint8_t *buf, uint8_t len)
{
  uint8_t head[4];
  uint8_t i;
  
  head[0] = PAGE_PROGRAM;
  head[1] = (address >> 16 ) & 0xff;
  head[2] = (address >> 8) & 0xff;
  head[3] = address & 0xff;
  
  w25x16_is_busy();
  set_write_enable();

  SET_CS_ENABLE();
//  HAL_Delay(1);
  LL_mDelay(1);
  for(i=0;i<4;i++) {
    spi_ll_wr(head[i]);
  }
  for(i=0;i<len;i++) {
    spi_ll_wr(buf[i]);
  }
  SET_CS_DISABLE();

//  printf("write:");
//  for (i=0;i<len;i++) {
//    printf("%02x ", buf[i]);
//  }
//  printf("\r\n");
}

*/
