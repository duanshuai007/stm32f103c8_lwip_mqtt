#include "enc28j60.h"
#include "stdio.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"

static uint8_t Enc28j60Bank;
static uint32_t NextPacketPtr;

#define ENC28J60_ENABLE()       LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4)
#define ENC28J60_DISABLE()      LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4)

static uint8_t spi_ll_wr(uint8_t dat)
{
  uint8_t retry=0;
  while(!LL_SPI_IsActiveFlag_TXE(SPI1)){
    retry++;
    if (retry > 200)
      return 0;
  }
  LL_SPI_TransmitData8(SPI1, dat);
  
  retry = 0;
  while(!LL_SPI_IsActiveFlag_RXNE(SPI1)){
    retry++;
    if (retry > 200)
      return 0;
  }
  return LL_SPI_ReceiveData8(SPI1);
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
  uint8_t dat = 0;

  ENC28J60_ENABLE();
  
  dat = op | (address & ADDR_MASK);
  spi_ll_wr(dat);
  dat = spi_ll_wr(0xFF);
  // do dummy read if needed (for mac and mii, see datasheet page 29)
  if(address & 0x80)
  {
    dat = spi_ll_wr(0xFF);
  }
  // release CS
  ENC28J60_DISABLE();
  return dat;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
  uint8_t dat = 0;
  
  ENC28J60_ENABLE();
  // issue write command
  dat = op | (address & ADDR_MASK);
  spi_ll_wr(dat);
  // write data
  //  dat = data;
  spi_ll_wr(data);
  ENC28J60_DISABLE();
}

void enc28j60ReadBuffer(uint32_t len, uint8_t* data)
{
  ENC28J60_ENABLE();
  // issue read command
  spi_ll_wr(ENC28J60_READ_BUF_MEM);
  while(len)
  {
    len--;
    // read data
    *data = (uint8_t)spi_ll_wr(0);
    data++;
  }
  *data='\0';
  ENC28J60_DISABLE();
}

void enc28j60WriteBuffer(uint32_t len, uint8_t* data)
{
  ENC28J60_ENABLE();
  // issue write command
  spi_ll_wr(ENC28J60_WRITE_BUF_MEM);
  
  while(len)
  {
    len--;
    spi_ll_wr(*data);
    data++;
  }
  ENC28J60_DISABLE();
}

void enc28j60SetBank(uint8_t address)
{
  // set the bank (if needed)
  if((address & BANK_MASK) != Enc28j60Bank)
  {
    // set the bank
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
    Enc28j60Bank = (address & BANK_MASK);
  }
}

uint8_t enc28j60Read(uint8_t address)
{
  // set the bank
  enc28j60SetBank(address);
  // do the read
  return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(uint8_t address, uint8_t data)
{
  // set the bank
  enc28j60SetBank(address);
  // do the write
  enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(uint8_t address, uint32_t data)
{
  // set the PHY register address
  enc28j60Write(MIREGADR, address);
  // write the PHY data
  enc28j60Write(MIWRL, data);
  enc28j60Write(MIWRH, data>>8);
  // wait until the PHY write completes
  
  while(enc28j60Read(MISTAT) & MISTAT_BUSY)
  {
    //Del_10us(1);
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
  }
}

void enc28j60clkout(uint8_t clk)
{
  //setup clkout: 2 is 12.5MHz:
  enc28j60Write(ECOCON, clk & 0x7);
}

uint8_t enc28j60Init(uint8_t* macaddr)
{  
  uint16_t retry = 0;
  uint8_t address[6];
  
  ENC28J60_DISABLE();	      
  enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
  while(!(enc28j60Read(ESTAT)&ESTAT_CLKRDY)&&retry<500)
  {
    retry++;
    //		delay_ms(1);
  };
  if(retry>=500)
    return 1;//ENC28J60初始化失败
  //	Del_1ms(250);
  // check CLKRDY bit to see if reset is complete
  // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
  //while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
  // do bank 0 stuff
  // initialize receive buffer
  // 16-bit transfers, must write low byte first
  // set receive buffer start address	   设置接收缓冲区地址  8K字节容量
  NextPacketPtr = RXSTART_INIT;
  // Rx start
  //接收缓冲器由一个硬件管理的循环FIFO 缓冲器构成。
  //寄存器对ERXSTH:ERXSTL 和ERXNDH:ERXNDL 作
  //为指针，定义缓冲器的容量和其在存储器中的位置。
  //ERXST和ERXND指向的字节均包含在FIFO缓冲器内。
  //当从以太网接口接收数据字节时，这些字节被顺序写入
  //接收缓冲器。 但是当写入由ERXND 指向的存储单元
  //后，硬件会自动将接收的下一字节写入由ERXST 指向
  //的存储单元。 因此接收硬件将不会写入FIFO 以外的单
  //元。
  enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);	 //
  enc28j60Write(ERXSTH, RXSTART_INIT>>8);
  // set receive pointer address
  //ERXWRPTH:ERXWRPTL 寄存器定义硬件向FIFO 中
  //的哪个位置写入其接收到的字节。 指针是只读的，在成
  //功接收到一个数据包后，硬件会自动更新指针。 指针可
  //用于判断FIFO 内剩余空间的大小  8K-1500。 
  enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
  enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
  // RX end
  enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
  enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
  // TX start	  1500
  enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
  enc28j60Write(ETXSTH, TXSTART_INIT>>8);
  // TX end
  enc28j60Write(ETXNDL, TXSTOP_INIT&0xFF);
  enc28j60Write(ETXNDH, TXSTOP_INIT>>8);
  // do bank 1 stuff, packet filter:
  // For broadcast packets we allow only ARP packtets
  // All other packets should be unicast only for our mac (MAADR)
  //
  // The pattern to match on is therefore
  // Type     ETH.DST
  // ARP      BROADCAST
  // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
  // in binary these poitions are:11 0000 0011 1111
  // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
  //接收过滤器
  //UCEN：单播过滤器使能位
  //当ANDOR = 1 时：
  //1 = 目标地址与本地MAC 地址不匹配的数据包将被丢弃
  //0 = 禁止过滤器
  //当ANDOR = 0 时：
  //1 = 目标地址与本地MAC 地址匹配的数据包会被接受
  //0 = 禁止过滤器
  
  //CRCEN：后过滤器CRC 校验使能位
  //1 = 所有CRC 无效的数据包都将被丢弃
  //0 = 不考虑CRC 是否有效
  
  //PMEN：格式匹配过滤器使能位
  //当ANDOR = 1 时：
  //1 = 数据包必须符合格式匹配条件，否则将被丢弃
  //0 = 禁止过滤器
  //当ANDOR = 0 时：
  //1 = 符合格式匹配条件的数据包将被接受
  //0 = 禁止过滤器
  enc28j60Write(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_PMEN);
  //  enc28j60Write(ERXFCON, ERXFCON_UCEN | ERXFCON_PMEN);
  enc28j60Write(EPMM0, 0x3f);
  enc28j60Write(EPMM1, 0x30);
  enc28j60Write(EPMCSL, 0xf9);
  enc28j60Write(EPMCSH, 0xf7);
  // do bank 2 stuff
  // enable MAC receive
  //bit 0 MARXEN：MAC 接收使能位
  //1 = 允许MAC 接收数据包
  //0 = 禁止数据包接收
  //bit 3 TXPAUS：暂停控制帧发送使能位
  //1 = 允许MAC 发送暂停控制帧（用于全双工模式下的流量控制）
  //0 = 禁止暂停帧发送
  //bit 2 RXPAUS：暂停控制帧接收使能位
  //1 = 当接收到暂停控制帧时，禁止发送（正常操作）
  //0 = 忽略接收到的暂停控制帧
  enc28j60Write(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
  // bring MAC out of reset
  //将MACON2 中的MARST 位清零，使MAC 退出复位状态。
  enc28j60Write(MACON2, 0x00);
  // enable automatic padding to 60bytes and CRC operations
  //bit 7-5 PADCFG2:PACDFG0：自动填充和CRC 配置位
  //111 = 用0 填充所有短帧至64 字节长，并追加一个有效的CRC
  //110 = 不自动填充短帧
  //101 = MAC 自动检测具有8100h 类型字段的VLAN 协议帧，并自动填充到64 字节长。如果不
  //是VLAN 帧，则填充至60 字节长。填充后还要追加一个有效的CRC
  //100 = 不自动填充短帧
  //011 = 用0 填充所有短帧至64 字节长，并追加一个有效的CRC
  //010 = 不自动填充短帧
  //001 = 用0 填充所有短帧至60 字节长，并追加一个有效的CRC
  //000 = 不自动填充短帧
  //bit 4 TXCRCEN：发送CRC 使能位
  //1 = 不管PADCFG如何，MAC都会在发送帧的末尾追加一个有效的CRC。 如果PADCFG规定要
  //追加有效的CRC，则必须将TXCRCEN 置1。
  //0 = MAC不会追加CRC。 检查最后4 个字节，如果不是有效的CRC 则报告给发送状态向量。
  //bit 0 FULDPX：MAC 全双工使能位
  //1 = MAC工作在全双工模式下。 PHCON1.PDPXMD 位必须置1。
  //0 = MAC工作在半双工模式下。 PHCON1.PDPXMD 位必须清零。
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX);
  // set inter-frame gap (non-back-to-back)
  //配置非背对背包间间隔寄存器的低字节
  //MAIPGL。 大多数应用使用12h 编程该寄存器。
  //如果使用半双工模式，应编程非背对背包间间隔
  //寄存器的高字节MAIPGH。 大多数应用使用0Ch
  //编程该寄存器。
  enc28j60Write(MAIPGL, 0x12);
  enc28j60Write(MAIPGH, 0x0C);
  // set inter-frame gap (back-to-back)
  //配置背对背包间间隔寄存器MABBIPG。当使用
  //全双工模式时，大多数应用使用15h 编程该寄存
  //器，而使用半双工模式时则使用12h 进行编程。
  enc28j60Write(MABBIPG, 0x15);
  // Set the maximum packet size which the controller will accept
  // Do not send packets longer than MAX_FRAMELEN:
  // 最大帧长度  1500
  enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
  enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);
  // do bank 3 stuff
  // write MAC address
  // NOTE: MAC address in ENC28J60 is byte-backward
  enc28j60Write(MAADR5, macaddr[0]);	
  enc28j60Write(MAADR4, macaddr[1]);
  enc28j60Write(MAADR3, macaddr[2]);
  enc28j60Write(MAADR2, macaddr[3]);
  enc28j60Write(MAADR1, macaddr[4]);
  enc28j60Write(MAADR0, macaddr[5]);
  //配置PHY为全双工  LEDB为拉电流
  enc28j60PhyWrite(PHCON1, PHCON1_PDPXMD);
  // no loopback of transmitted frames	 禁止环回
  //HDLDIS：PHY 半双工环回禁止位
  //当PHCON1.PDPXMD = 1 或PHCON1.PLOOPBK = 1 时：
  //此位可被忽略。
  //当PHCON1.PDPXMD = 0 且PHCON1.PLOOPBK = 0 时：
  //1 = 要发送的数据仅通过双绞线接口发出
  //0 = 要发送的数据会环回到MAC 并通过双绞线接口发出
  enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
  // switch to bank 0
  //ECON1 寄存器
  //寄存器3-1 所示为ECON1 寄存器，它用于控制
  //ENC28J60 的主要功能。 ECON1 中包含接收使能、发
  //送请求、DMA 控制和存储区选择位。
  
  enc28j60SetBank(ECON1);
  // enable interrutps
  //EIE： 以太网中断允许寄存器
  //bit 7 INTIE： 全局INT 中断允许位
  //1 = 允许中断事件驱动INT 引脚
  //0 = 禁止所有INT 引脚的活动（引脚始终被驱动为高电平）
  //bit 6 PKTIE： 接收数据包待处理中断允许位
  //1 = 允许接收数据包待处理中断
  //0 = 禁止接收数据包待处理中断
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
  // enable packet reception
  //bit 2 RXEN：接收使能位
  //1 = 通过当前过滤器的数据包将被写入接收缓冲器
  //0 = 忽略所有接收的数据包
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
  
  address[0] = enc28j60Read(MAADR5);
  address[1] = enc28j60Read(MAADR4);
  address[2] = enc28j60Read(MAADR3);
  address[3] = enc28j60Read(MAADR2);
  address[4] = enc28j60Read(MAADR1);
  address[5] = enc28j60Read(MAADR0);
//  printf("%02x %02x %02x %02x %02x %02x\r\n",
//         address[0], address[1], address[2], address[3], address[4], address[5]);
//  
  for (retry = 0; retry < 6; retry++) {
    if ( address[retry] != macaddr[5-retry])
      return 0;
  }
  
  return 1;
}

// read the revision of the chip:
uint8_t enc28j60getrev(void)
{
  //在EREVID 内也存储了版本信息。 EREVID 是一个只读控
  //制寄存器，包含一个5 位标识符，用来标识器件特定硅片
  //的版本号
  return(enc28j60Read(EREVID));
}

void enc28j60PacketSend(uint32_t len, uint8_t* packet)
{
  // Set the write pointer to start of transmit buffer area
  enc28j60Write(EWRPTL, TXSTART_INIT&0xFF);
  enc28j60Write(EWRPTH, TXSTART_INIT>>8);
  
  // Set the TXND pointer to correspond to the packet size given
  enc28j60Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
  enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);
  
  // write per-packet control byte (0x00 means use macon3 settings)
  enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
  
  // copy the packet into the transmit buffer
  enc28j60WriteBuffer(len, packet);
  
  // send the contents of the transmit buffer onto the network
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
  
  // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
  if( (enc28j60Read(EIR) & EIR_TXERIF) )
  {
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
  }
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint32_t enc28j60PacketReceive(uint32_t maxlen, uint8_t* packet)
{
  uint32_t rxstat;
  uint32_t len;
  
  // check if a packet has been received and buffered
  //if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
  // The above does not work. See Rev. B4 Silicon Errata point 6.
  if( enc28j60Read(EPKTCNT) ==0 )  //收到的以太网数据包长度
  {
    return(0);
  }
  
  // Set the read pointer to the start of the received packet		 缓冲器读指针
  enc28j60Write(ERDPTL, (NextPacketPtr));
  enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
  
  // read the next packet pointer
  NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
  NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
  
  // read the packet length (see datasheet page 43)读取数据包长度
  len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
  len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
  
  len-=4; //remove the CRC count
  // read the receive status (see datasheet page 43)读接收状态
  rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
  rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
  // limit retrieve length
  if (len>maxlen-1)
  {
    len=maxlen-1;
  }
  
  // check CRC and symbol errors (see datasheet page 44, table 7-3):
  // The ERXFCON.CRCEN is set by default. Normally we should not
  // need to check this.
  if ((rxstat & 0x80)==0)
  {
    // invalid
    len=0;
  }
  else
  {
    // copy the packet from the receive buffer复制数据包接收缓冲区
    enc28j60ReadBuffer(len, packet);
  }
  // Move the RX read pointer to the start of the next received packet
  // This frees the memory we just read out
  enc28j60Write(ERXRDPTL, (NextPacketPtr));
  enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);
  
  // decrement the packet counter indicate we are done with this packet
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
  return(len);
}