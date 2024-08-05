/*
 * parser.h
 *
 *  Created on: 14 Tem 2024
 *      Author: Harun
 */

#ifndef INC_PARSER_H_
#define INC_PARSER_H_




#include "main.h"
#include "stdio.h"
#include "math.h"
#include <vector>
#include "string.h"


#include "usbd_cdc_if.h"

 // USBDEN GELEN INTEL HEX SATIRININ PARSELENMESI ICIN GEREKEN DEGISKENLER
typedef struct {
char startcode;
uint8_t bytecount;
uint16_t address;
uint8_t recordtype;
char data[35];
uint8_t checksum;
uint16_t doubletotal;

}usbgelendata;

//SOURCE KODDA KULLANILMAK UZERE MAIN.CPPDE OLUSTURULAN DEGISKENLERIN BURADA KULLANILMASININ SAGLANMASI (EXTERN)

extern uint16_t hatalimesajsayisi;
extern uint16_t hatasizmesajsayisi;
extern char txbuffer[80];
extern usbgelendata usbayiklanmisdata;
extern uint32_t extended_linear_address;
extern uint32_t start_linear_address;
extern uint32_t resultaddress;


//FONKSIYON PROTOTIPLERININ TANIMLANMASI
uint32_t fourbit_to_uint8x_t(char* anlamlimesaj,uint8_t x,uint8_t baslangicindex );
void datayiayiklavekullan(char* ayiklanacak,uint16_t oankisatir);
void resetusbayiklanmisdata();
void flashayaz (uint16_t address,uint8_t recordtype,char* alinandata,uint8_t datalength);
void systemresetandrunnewfirmware();


#endif /* INC_PARSER_H_ */
