#include "cmsis_os2.h"
#include "main.h"
#include "cmsis_os.h"
#include "socket.h"
#include "lwip.h"

#define F7_ADDR "192.168.21.111"
#define PC_ADDR "192.168.21.100"
#define F7_PORT 4001
#define PC_PORT 4001

extern "C" void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();

  /* USER CODE BEGIN 5 */
  //データを格納する配列
  uint8_t rxbuf[16];
  uint8_t txbuf[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
  //アドレスを宣言
  struct sockaddr_in rxAddr,txAddr;
  //ソケットを作成
  int socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
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
  socklen_t n; //受信したデータのサイズ
  socklen_t len = sizeof(rxAddr); //rxAddrのサイズ
  /* Infinite loop */
  for(;;)
  {
    n = lwip_recvfrom(socket, (uint8_t*) rxbuf, sizeof(rxbuf), (int) NULL, (struct sockaddr*) &rxAddr, &len); //受信処理(blocking)
    lwip_sendto(socket, (uint8_t*) txbuf, sizeof(txbuf), 0, (struct sockaddr*) &txAddr, sizeof(txAddr)); //受信したら送信する
  }
  /* USER CODE END 5 */
}