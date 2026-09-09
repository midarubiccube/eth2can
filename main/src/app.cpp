#include "cmsis_os2.h"
#include "main.h"
#include "cmsis_os.h"
#include "socket.h"
#include "usb_device.h"
#include "lwip.h"

#include "FullColorLED.hpp"
#include "CANFD.hpp"

#include "UDPPacket_format.h"

#define F7_ADDR "192.168.10.103"
#define PC_ADDR "192.168.10.102"
#define F7_PORT 4001
#define PC_PORT 4001

extern osTimerId_t ReceivetimerHandle;

int socket;
bool socket_ready = false;
struct sockaddr_in rxAddr,txAddr;

FullColorLED led{&htim1, TIM_CHANNEL_1};
CANFD* canfd1;
CANFD* canfd2;
uint8_t canid_map[16][16];


void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if (hfdcan->Instance == FDCAN1) {
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
	    canfd1->rx_interrupt_task();
    }
  }
  
  if (hfdcan->Instance == FDCAN3) {
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
      canfd2->rx_interrupt_task();
    } 
  }
}

extern "C" void ReceiveCallback(void const * argument)
{
  if (canfd1->rx_available() > 0) {
    CANFD_Frame rx_data;
    UdpPacket tx_packet{};
    tx_packet.header[0] = 'C';
    tx_packet.header[1] = 'A';
    tx_packet.header[2] = 'N';

    canfd1->rx(rx_data);
    if (rx_data.is_remote) {
      canid_map[(rx_data.id>>4) & 0xf][(rx_data.id) & 0xf] = 1;
    } else {
      tx_packet.id = rx_data.id;
      tx_packet.size = rx_data.size;
      memcpy(tx_packet.data, rx_data.data, rx_data.size);
      lwip_sendto(socket, (uint8_t*) &tx_packet, sizeof(tx_packet), 0, (struct sockaddr*) &txAddr, sizeof(txAddr)); //受信したら送信する
    }
  }
  while (canfd2->rx_available() > 0) {
    CANFD_Frame rx_data;
    UdpPacket tx_packet{};
    tx_packet.header[0] = 'C';
    tx_packet.header[1] = 'A';
    tx_packet.header[2] = 'N';

    canfd2->rx(rx_data);
    if (rx_data.is_remote) {
      canid_map[(rx_data.id>>4) & 0xf][(rx_data.id) & 0xf] = 2;
    } else {
      tx_packet.id = rx_data.id;
      tx_packet.size = rx_data.size;
      memcpy(tx_packet.data, rx_data.data, tx_packet.size);
      lwip_sendto(socket, (uint8_t*) &tx_packet, sizeof(tx_packet), 0, (struct sockaddr*) &txAddr, sizeof(txAddr)); //受信したら送信する
    }
  }
}

extern "C" void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  MX_USB_DEVICE_Init();

  
  canfd1 = new CANFD(&hfdcan1);
	canfd1->start();

  canfd2 = new CANFD(&hfdcan3);
	canfd2->start();
  
  led.start();
  led.set_rgb(255, 255, 255);

  CANFD_Frame remote_frame;
  remote_frame.id = 0x00000000;
  remote_frame.size = 0;
  remote_frame.is_remote = true;  
  canfd1->tx(remote_frame);
  canfd2->tx(remote_frame);

  //データを格納する配列
  uint8_t rxbuf[sizeof(UdpPacket)];

  socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
  socket_ready = true;
  //アドレスのメモリを確保
  memset((char*) &txAddr, 0, sizeof(txAddr));
  memset((char*) &rxAddr, 0, sizeof(rxAddr));
  //アドレスの構造体のデータを定義
  rxAddr.sin_family = AF_INET; //プロトコルファミリの設定(IPv4に設定)
  rxAddr.sin_len = sizeof(rxAddr); //アドレスのデータサイズ
  rxAddr.sin_addr.s_addr = INADDR_ANY; //アドレスの設定(今回はすべてのアドレスを受け入れるためINADDR_ANY)
  rxAddr.sin_port = lwip_htons(PC_PORT); //ポートの指定
  txAddr.sin_family = AF_INET; //プロトコルファミリの指定(IPv4に設定)
  txAddr.sin_len = sizeof(txAddr); //アドレスのデータのサイズ
  txAddr.sin_addr.s_addr = inet_addr(PC_ADDR); //アドレスの設定
  txAddr.sin_port = lwip_htons(PC_PORT); //ポートの指定
  (void)lwip_bind(socket, (struct sockaddr*)&rxAddr, sizeof(rxAddr)); //IPアドレスとソケットを紐付けて受信をできる状態に
  socklen_t len = sizeof(rxAddr); //rxAddrのサイズ

  osTimerStart(ReceivetimerHandle, 1);
  /* Infinite loop */
  for(;;)
  {
    lwip_recvfrom(socket, (uint8_t*) rxbuf, sizeof(rxbuf), (int) NULL, (struct sockaddr*) &rxAddr, &len); //受信処理(blocking)
    UdpPacket& packet = (UdpPacket&)rxbuf;
    if (memcmp(packet.header, "CAN", 3) == 0)
    { 
      CANFD_Frame can_tx;
	    can_tx.id = packet.id;
	    can_tx.size = packet.size;
	    memcpy(can_tx.data, packet.data, (uint8_t)packet.size);
      if (canid_map[(packet.id>>4) & 0xf][(packet.id) & 0xf] == 1) {
        canfd1->tx(can_tx);
      } else if (canid_map[(packet.id>>4) & 0xf][(packet.id) & 0xf] == 2) {
        canfd2->tx(can_tx);
      }
    }
  }
}
