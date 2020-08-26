#ifndef __FLASH_H__
#define __FLASH_H__

#include "stdio.h"
#include "stm32f1xx.h"

#define STATUS_REG1             0x05
#define STATUS_REG2             0x35        
#define WRITE_ENABLE            0x06		//Ð´³äÐí
#define WRITE_DISABLE           0x04		//Ð´½ûÖ¹
#define READ_MANUID             0x90
#define READ_JEDEC_ID           0x9f
#define ERASE_ALL               0xc7   
#define SECTOR_ERASE            0x20
#define BLOCK_32K_ERASE         0x52
#define BLOCK_64K_ERASE         0xD8
#define READ_DATA               0x03
#define PAGE_PROGRAM            0x02       

/*
void w25x16_ReadJEDECID(void);
void w25x16_ReadDeviceID(void);
void w25x16_ReadManufacturer(void);
uint8_t w25x16_readstatus1(void);
void w25x16_readstatus2(void);
void w25x16_sector_erase(uint32_t address);
void w25x16_read_data(uint32_t address, uint8_t *buf, uint8_t len);
void w25x16_write_data(uint32_t address, uint8_t *buf, uint8_t len);
*/
void w25x16_ReadUniqueID(uint8_t *buf);

#endif