/*
 * parser.cpp
 *
 *  Created on: 14 Tem 2024
 *      Author: Harun
 */
#include <parser.h>



void datayiayiklavekullan(char* ayiklanacak,uint16_t oankisatir)
{
    if(ayiklanacak[0]!=':') // INTEL HEX SATIRI : ILE BASLAMALIDIR
    {
        ++hatalimesajsayisi;
        return;
    }
    resetusbayiklanmisdata();

    // PARSELENME ISLEMLERININ YAPILMASI
    uint8_t control;
    usbayiklanmisdata.startcode=ayiklanacak[0];
    usbayiklanmisdata.bytecount=fourbit_to_uint8x_t(ayiklanacak, 1, 1);
    usbayiklanmisdata.address=fourbit_to_uint8x_t(ayiklanacak, 2, 3);
    usbayiklanmisdata.recordtype=fourbit_to_uint8x_t(ayiklanacak, 1, 7);
    for(uint8_t index =9 ; index <(9+(2*usbayiklanmisdata.bytecount)) ;++index) {
    	usbayiklanmisdata.data[index-9] = ayiklanacak[index];
    }


    usbayiklanmisdata.checksum= fourbit_to_uint8x_t(ayiklanacak, 1, 9+(2*usbayiklanmisdata.bytecount));
    for(uint8_t index =1; index <strlen(ayiklanacak)-2 ; index+=2)
    {
        usbayiklanmisdata.doubletotal+=fourbit_to_uint8x_t(ayiklanacak, 1, index);
    }

    // PARSELEME ISLEMI SONRASI GELEN DATANIN DOGRULUGUNUN KONTROL EDILMESI
    control=(256-(usbayiklanmisdata.doubletotal % 256) )-(usbayiklanmisdata.checksum) ;
    if(!control ) // GELEN SATIR DOGRUYSA ISLENMEK UZERE GONDERILMESI
    {
        ++hatasizmesajsayisi;
        flashayaz(usbayiklanmisdata.address,usbayiklanmisdata.recordtype,usbayiklanmisdata.data,usbayiklanmisdata.bytecount);
        return ;
    }
    else // GELEN SATIRDA HATA VARSA SERI PORTTAN BILGILENDIRILMESI VE ISLEM YAPILMASININ ENGELLENMESI
    {
        ++hatalimesajsayisi;
        memset(txbuffer,0,sizeof(txbuffer));
        sprintf(txbuffer,"hatalisatir:%d hatali data:%s dt:%d cs:%d control:%d\r\n",oankisatir,ayiklanacak,usbayiklanmisdata.doubletotal,usbayiklanmisdata.checksum,control);
        CDC_Transmit_FS((uint8_t*)txbuffer, strlen(txbuffer));
        return;
    }

}

// USBDEN GELEN HER SATIR INCELER VE EGER PROBLEM YOKSA GELEN SATIRI UYGULAR
void flashayaz (uint16_t address,uint8_t recordtype,char* alinandata,uint8_t datalength)
{
	uint8_t data[datalength]={}; // CHAR DIZISINDEN YAZILACAK BYTELERIN DIZISINI OLUSTURMA
    for(uint8_t index=0;index<datalength;++index) {
    	data[index]= fourbit_to_uint8x_t(alinandata, 1, index*2);
    }

    switch(recordtype)
    {
    case 0: // ILGILI FLASH ADDRESINE BYTE BYTE VERI YAZAN KISIM.
    	resultaddress = address + extended_linear_address;
    	__disable_irq(); // YAZMA YAPMADAN ONCE INTERRUPTLARI KAPATMA
    	HAL_FLASH_Unlock(); // YAZMA ISLEMI ICIN KILIT ACILIR
    	for(uint8_t index=0;index<datalength;++index) {
    		if(resultaddress+index >= 0x08000000 && resultaddress+index <= 0x08007FFF) { //SECTOR 1 VE SECTOR 2 HARIC YAZMA YAPILMASINI ISTEMIYORUM
    			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, resultaddress +index, data[index]) != HAL_OK) {
    			// ILGILI BYTE YAZILIRKEN BIR PROBLEM OLUSURSA KOD DURDURULMAK UZERE ERROR_HANDLERE ATLIYOR
    			    HAL_FLASH_Lock();
    			    __enable_irq();
    			    Error_Handler();
    			    return;
    			}
    		}
    	}
    	HAL_FLASH_Lock(); // YAZMA ISLEMI ICIN KILIT KAPANIR
    	__enable_irq(); // YAZMA YAPILDIKTAN SONRA KESMELERIN AKTİF HALE GETIRILMESI
        break;


    case 1 : // 01 END OF FILEDIR. EGER YAZMADA OKUMADA VS BIR SORUN YOKSA YENI FIRMWAREYI BASLATMAK ICIN FONKSIYON CAGIRIR
        if(hatalimesajsayisi==0 && satirsayisi==hatasizmesajsayisi)
        {
            //Sistemi resetle
        	systemresetandrunnewfirmware();
        }
        break;


    case 4: // BASLANGIC ADDRESININ HANGI SECTOR OLACAGINI BELIRLEYECEK OLAN ADDRESS (4BYTE DATA)
    	extended_linear_address= ((data[0] << 8) + data[1]) << 16 ;
        break;

    case 5: //BU ISLEMDE HER ZAMAN 4 BYTE GONDERİLİR. BU 4 BYTE UINT32_T'YE DONUSTURULUR.
    	for(uint8_t index=0;index<datalength;++index) {
    		start_linear_address<<=8; // BU ADDRESS SONUÇ OLARAK YENI vector listesinin baslangicini ayarlayacak
    		start_linear_address|=fourbit_to_uint8x_t(alinandata, 1, index*2);;
    	}

    	break;

    default : //0X00 0X01 0X04 0X05 HARICI GELIRSE
        hatalimesajsayisi++;
        break;
    }
}

// FLASHA YAZIM ISLEMI TAMAMLANDIKTAN SONRA  HERHANGI BIR SORUN YOKSA BU FONKSIYON CAGIRILIR
void systemresetandrunnewfirmware() {

	    SCB->VTOR = start_linear_address; // YENI FIRMWARE BU ADDRESSTEN BASLAYACAK

	    __set_MSP(*(__IO uint32_t*) start_linear_address); // STACK POINTER AYARI

	    NVIC_SystemReset(); // YAZILIMSAL RESET SONRASI FIRMWARE ILGILI ADDRESSTEN BASLATILACAK

	    Error_Handler(); // RESET ISLEMI BASARISIZ OLURSA BU FONKSIYONA DALLANIR VE SONSUZ DONGU ICINDE BEKLER

}

// HEXADECIMAL BIR SAYI 4 BITTIR. UINT8_T (2 HEX) 2NIN KATLARI SEKLINDE DEVAM EDER
uint32_t fourbit_to_uint8x_t(char* anlamlimesaj,uint8_t x,uint8_t baslangicindex ) // DIZIDEN OKUNAN DEGERLERIN X DEGERINE GORE INT DEGERINE DONUSTURULMESI
{
    uint32_t dondurulecek=0;

    for(uint8_t k=baslangicindex; k<baslangicindex+(pow(2,x)); ++k) // tum karakterleri okuyup int degere cevirme
    {
        dondurulecek=dondurulecek<<4; // her dongude 16 ile carpim (hexadecimal carpan)
        if(anlamlimesaj[k]<='F' && anlamlimesaj[k]>='A')
            dondurulecek|=((anlamlimesaj[k]-'A'+10) );
        if(anlamlimesaj[k]<='f' && anlamlimesaj[k]>='a')
            dondurulecek|=((anlamlimesaj[k]-'a'+10) );
        else if (anlamlimesaj[k]<='9' && anlamlimesaj[k]>='0')
            dondurulecek|=((anlamlimesaj[k]-'0') );
    }
    return dondurulecek;
}


// HER SATIR PARSELENMEDEN ONCE, ONCEKI VERILER TEMIZLENIYOR
void resetusbayiklanmisdata()
{
    usbayiklanmisdata.startcode=0;
    usbayiklanmisdata.bytecount=0;
    usbayiklanmisdata.address=0;
    usbayiklanmisdata.recordtype=0;
    usbayiklanmisdata.checksum=0;
    usbayiklanmisdata.doubletotal=0;
    memset(usbayiklanmisdata.data,0,sizeof(usbayiklanmisdata.data));
}


