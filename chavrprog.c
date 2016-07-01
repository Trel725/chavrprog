
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ch341a.h"
#include <cintelhex.h>
#include <unistd.h>
#include "chavrprog.h"



//SPI commands from datasheet

unsigned char read_fuse[4]={0b01010000, 0b00000000,0x00, 0x00};
unsigned char read_fuseH[4]={0b01011000, 0b00001000,0x00, 0x00};
unsigned char load_ext_addr[4]={0b01001101, 0x00, 0x00, 0x00};
unsigned char load_pg[4]={0b01000000, 0x00, 0x00, 0x00};
unsigned char load_pgh[4]={0b01001000, 0x00, 0x00, 0x00};
unsigned char write_pg[4]={0b01001100, 0x00, 0x00, 0x00};
unsigned char read_pgl[4]={0b00100000, 0x00, 0x00, 0x00};
unsigned char read_pgh[4]={0b00101000, 0x00, 0x00, 0x00};
unsigned char load_ext[4]={0b01001101, 0b00000000, 0x00, 0x00};
unsigned char chip_er[4]={0b10101100, 0b10000000, 0x00, 0x00};
unsigned char prog_en[4]={0b10101100, 0b01010011, 0x00, 0x00};
unsigned char read_sign[4]={0b00110000, 0x10, 0b00000000, 0x00};
unsigned char write_fsl[4]={0b10101100,  0b10100000, 0x00, 0x00};
unsigned char write_fsh[4]={0b10101100,  0b10101000, 0x00, 0x00};
unsigned char write_fse[4]={0b10101100,  0b10100100, 0x00, 0x00};
unsigned char read_lk[4]={0b01011000, 0b00000000, 0x00,0x00};
unsigned char read_fs[3][4]={{0b01010000, 0b00000000,  0x00, 0x00},
{0b01011000, 0b00001000,  0x00, 0x00},
{0b01010000, 0b00001000,  0x00, 0x00}};


unsigned char fuses[3];



void ch_exit(void){
  toggle_reset(0);
  ch341Release();
  exit(0);
}

void ch341_init(){
  ch341Configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
  ch341SetStream(1);
}

void chip_prog_enable() {
  ch341SpiStream(prog_en,spi_data,4);
  if(spi_data[2]==0x53){
    printf("AVR answered!\n");
  }
  else{
    printf("No answer, sorry\n");
    printf("Exiting...\n");
    ch_exit();
  }
}


void write_fuses(unsigned char fuse, int fuse_type){
  switch(fuse_type){
    case 0:
    write_fsl[3]=fuse;
    ch341SpiStream(write_fsl, spi_data,4);
    printf("Low fuse byte was written, %x\n",fuse);
    break;
    case 1:
    write_fsh[3]=fuse;
    ch341SpiStream(write_fsh, spi_data,4);
    printf("High fuse byte was written %x\n",fuse);
    break;
    case 2:
    write_fse[3]=fuse;
    ch341SpiStream(write_fse, spi_data,4);
    printf("Extended fuse byte was written %x\n",fuse);
    break;
  }

}

char* read_fuses(){
  static char fuses[3];
  for(int i=0; i<3; i++){
    ch341SpiStream(read_fs[i], spi_data,4);
    fuses[i]=spi_data[3];
    printf("%x ", spi_data[3]);
  }
  return fuses;
}



void check_signature() {
  unsigned char signature[3];


  for(int i=0; i<3; i++){
    ch341SpiStream(read_sign,spi_data,4);
    signature[i]=spi_data[3];
    read_sign[2]=((i+1)<<0);
  }

  printf("Signature read as ");
  for(int i=0; i<3; i++){
    printf("%x ", signature[i]);
  }
  printf("\n");

  if(signature[0]==device_sign[0] && signature[1]==device_sign[1] && signature[2]==device_sign[2]){
    printf("Signature is correct\n");

  }
  else{
    printf("Signature mismatch, expected ");
    for(int i=0; i<3; i++){
      printf("%x ", device_sign[i]);
    }
    printf("\nExiting...\n");
    ch_exit();
  }
}

void chip_erace(){
  ch341SpiStream(chip_er,spi_data,4);
  usleep(10000);
}

void read_flash(int mem){//mem - addr in bytes from begining

  unsigned char * tmp_buf=malloc(mem*4);
  unsigned char * tmp_buf_in=malloc(mem*4);
  unsigned char command=0x20;
  for(uint16_t i=0; i<mem;i++){
    *tmp_buf=command;
    tmp_buf++;
    *tmp_buf=((i/2)>>8);
    tmp_buf++;
    *tmp_buf=((i/2) & 0xff);
    tmp_buf++;
    *tmp_buf=0x00;
    tmp_buf++;
    command^=(1<<3);

  }
  tmp_buf-=(mem*4);

  ch341SpiStream(tmp_buf, tmp_buf_in,mem*4);


  for(int i=3, j=0; i<(mem*4); i+=4,j++){
    data_buffer[j]=tmp_buf_in[i];
  }
}



void main_write_stream(const char * filename){
  //TODO: implement writing for bytes starting in the middle of page;
  //yes, it is messy :(
  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  int addr=0;


  for(int i=0; i<(*rs).ihrs_count;i++){ //iterate over strings in HEX

    int  addr_pg=addr>>PAGE_SHIFT;
    for(int j=0; j<((*(*rs).ihrs_records).ihr_length); j++){ //iterate over data in string


      addr=(*(*rs).ihrs_records).ihr_address+j;
      int  addr_pg=addr>>PAGE_SHIFT;
      load_pg[2]=((addr&PAGE_MSQ)/2);
      load_pg[3]=*(*(*rs).ihrs_records).ihr_data;
      ch341SpiStream(load_pg, spi_data,4);

      (*(*rs).ihrs_records).ihr_data++;
      load_pg[0]^=(1<<3);


      if((addr&PAGE_MSQ)==(PAGESIZE*2-1)){//write page in the end of page

        write_pg[1]=(addr_pg>>2);
        write_pg[2]=((addr_pg<<6)&0xC0);
        ch341SpiStream(write_pg, spi_data,4);
        usleep(5000);
        printf("Writing page # %d\n", addr_pg );
      }
    }

    if( (*(*rs).ihrs_records).ihr_type == IHEX_EOF){//write page in the end of file

      write_pg[1]=(addr_pg>>2);
      write_pg[2]=((addr_pg<<6)&0xC0);

      ch341SpiStream(write_pg, spi_data,4);
      printf("Writing page # %d\n", addr_pg );
      usleep(5000);//delay for write completion

    }

    (*rs).ihrs_records++;
  }

}

void check_flash(const char * filename){
  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  unsigned  char* dst = malloc(TOT_MEM*2);
  ihex_mem_copy(rs, dst, TOT_MEM*2, IHEX_WIDTH_16BIT, IHEX_ORDER_LITTLEENDIAN);

  read_flash(ihex_rs_get_size(rs));

  for(int i=0; i<ihex_rs_get_size(rs); i++){

    if(data_buffer[i]==*dst){
      printf("Byte %d is OK \n", i);

    }
    else{
      printf("\nByte %d is corrupted, read: %x, expected:%x\n", i, data_buffer[i], *dst);
      ch_exit();
    }
    dst++;
  }
  printf("%ld bytes successfully checked\n", ihex_rs_get_size(rs));
}

void toggle_reset(short stat){

  ch341SpiCs(spi_data, stat);
  usbTransfer(__func__, BULK_WRITE_ENDPOINT, spi_data, 4);
}

void read_locks(){
  ch341SpiStream(read_lk, spi_data,4);
  printf("Lock byte: %x\n", spi_data[3]);
}



void ch_init(void){
  ch341_init();
  toggle_reset(1);
  chip_prog_enable();
  check_signature();
}

void print_help(){
  printf("Usage:\n"
  "-r ADDR - read flash memory from start to ADDR in bytes, or\n"
  "-r f FILE read whole flash memory to specified file\n"
  "-w HEX - write flash memory from intel HEX file\n"
  "-e - erase devices\n"
  "-c HEX - read and compare HEX with flash memory\n"
  "-a HEX - automatically erase, write and check data in chip\n"
  "-f - read fuse bits\n"
  "-l BYTE - write low fuse\n"
  "-h BYTE - write high fuse\n"
  "-x BYTE - write extended fuse\n"
  "-L read lock bits\n");
}
