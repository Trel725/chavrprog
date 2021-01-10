/*
* This file is part of the chavrprog project.
*
* Copyright (C) 2016 Andrii Trelin (andrei.trel2010@gmail.com)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ch341a.h"
#include "cintelhex.h"
#include <unistd.h>
#include "chavrprog.h"

#define CMD_READ 0x20
#define CMD_READ_EEPR 0xA0

extern int delay;

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
unsigned char write_ep[4]={0b11000000,0x00,0x00,0x00};
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

unsigned char * data_buffer;
unsigned char spi_data[4];
unsigned char device_sign[3];
int cfg_pagesize;
int cfg_num_of_pages;
int cfg_eeprom;
unsigned cfg_pageshift;
unsigned cfg_pagemask;
int cfg_pagemsq;

void debug_call(void){
    printf("Using delay of %d us\n", delay);
}


void assign_cfg(int index){
  cfg_pagesize=confset[index].pagesize;
  cfg_num_of_pages=confset[index].num_of_pages;
  cfg_eeprom=confset[index].cfg_eeprom;
  memcpy(device_sign, confset[index].signature, 3);

  cfg_pageshift = 0;
  cfg_pagemask = 0;
  for(int i=cfg_pagesize; i>1; i>>=1){
    cfg_pageshift++;
    cfg_pagemask <<= 1;
    cfg_pagemask |= 1;
  }

}

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

  data_buffer=malloc(TOT_MEM*2);//buffer for reading chip

  unsigned char * tmp_buf=malloc(mem*4);
  unsigned char * tmp_buf_in=malloc(mem*4);
  unsigned char command=CMD_READ;
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


void write_page(unsigned addr_word) {
  write_pg[1]=(addr_word>>8);
  write_pg[2]=addr_word&0xff;
  printf("Writing page # %d, addr=0x%x\n", addr_word>>PAGE_SHIFT, addr_word);
  ch341SpiStream(write_pg, spi_data,4);
  usleep(delay);
}

void main_write_stream(const char * filename){
  //yes, it is messy :(
  
  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  unsigned addr=0, addr_word=0;
  #define LENGTH 0x10 //for most hex files standart length of string =0x10, necessary to detect gap in file.
  //otherwise it will just take some longer time

  for(unsigned int i=0; i<(*rs).ihrs_count;i++){ //iterate over strings in HEX
    for(unsigned int j=0; j<((*(*rs).ihrs_records).ihr_length) && (*(*rs).ihrs_records).ihr_type!= IHEX_SSA ; j++){ //iterate over data in string

      addr=(*(*rs).ihrs_records).ihr_address+j; //address from hex
      addr_word = addr >> 1;

      load_pg[0] = 0x40 | ((addr & 1) << 3); //switch between high/low byte
      load_pg[2] = addr_word & PAGE_MASK; //LSB in address
      load_pg[3] = *(*(*rs).ihrs_records).ihr_data; //single byte of data

      ch341SpiStream(load_pg, spi_data,4);

      (*(*rs).ihrs_records).ihr_data++; //get next byte of data

      if( j==((*(*rs).ihrs_records).ihr_length-1)){ //is this last byte in that record?
        if((*((*rs).ihrs_records+1)).ihr_address>((*(*rs).ihrs_records).ihr_address+LENGTH)){ //check isn't there a gap
          //if it is - write a page
          write_page(addr_word);
        }
      }

      if((addr+1)%(PAGESIZE*2)==0){//write page in the end of page
        write_page(addr_word);
      }
    }

    if( (*(*rs).ihrs_records).ihr_type == IHEX_EOF){//write page in the end of file
      write_page(addr_word);
    }

    (*rs).ihrs_records++;
  }

}



void check_flash(const char * filename){
  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  unsigned  char* dst = malloc(TOT_MEM*2);
  memset(dst, 0xff, TOT_MEM*2);
  ihex_mem_copy(rs, dst, TOT_MEM*2, IHEX_WIDTH_16BIT, IHEX_ORDER_LITTLEENDIAN);

  read_flash(ihex_rs_get_size(rs));

  for(int i=(*((*rs).ihrs_records)).ihr_address; i<ihex_rs_get_size(rs); i++){
    if(data_buffer[i]==*dst){

    }
    else{
      printf("\nByte 0x%x is corrupted, read: 0x%x, expected:0x%x\n", i, data_buffer[i], *dst);
      ch_exit();
    }
    dst++;
  }
  printf("%ld bytes successfully checked\n", ihex_rs_get_size(rs));
}



void check_flash_strict(const char * filename){
  ihex_recordset_t* rs = ihex_rs_from_file(filename);

  read_flash(TOT_MEM*2);

  int addr=0;
  int num=0;

  for(unsigned int i=0; i<(*rs).ihrs_count && (*(*rs).ihrs_records).ihr_type!= IHEX_SSA;i++){ //iterate over strings in HEX

    for(unsigned int j=0; j<((*(*rs).ihrs_records).ihr_length); j++){ //iterate over data in string
      num++;
      addr=(*(*rs).ihrs_records).ihr_address+j;
      if(data_buffer[addr]!=*(*(*rs).ihrs_records).ihr_data){
        printf("Error at address 0x%x, expected %x, read %x\n", addr, *(*(*rs).ihrs_records).ihr_data, data_buffer[addr]);
        exit(0);
      }
      (*(*rs).ihrs_records).ihr_data++;
    }
    (*rs).ihrs_records++;
  }
  printf("%d bytes successfully verified!\n", num);
}


void read_eeprom(){

  data_buffer=malloc(cfg_eeprom);//buffer for reading chip, eepr is already in bytes

  unsigned char * tmp_buf=malloc(cfg_eeprom*4);
  unsigned char * tmp_buf_in=malloc(cfg_eeprom*4);
  unsigned char command=CMD_READ_EEPR;
  for(uint16_t i=0; i<cfg_eeprom;i++){
    *tmp_buf=command;
    tmp_buf++;
    *tmp_buf=(i>>8);
    tmp_buf++;
    *tmp_buf=(i & 0xff);
    tmp_buf++;
    *tmp_buf=0x00;
    tmp_buf++;
  }
  tmp_buf-=(cfg_eeprom*4);

  ch341SpiStream(tmp_buf, tmp_buf_in,cfg_eeprom*4);


  for(int i=3, j=0; i<(cfg_eeprom*4); i+=4,j++){
    data_buffer[j]=tmp_buf_in[i];
  }
}



void write_eeprom(const char * filename){

  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  int addr=0;

  for(unsigned int i=0; i<(*rs).ihrs_count;i++){ //iterate over strings in HEX

    for(unsigned int j=0; j<((*(*rs).ihrs_records).ihr_length) && (*(*rs).ihrs_records).ihr_type!= IHEX_SSA ; j++){ //iterate over data in string

      addr=(*(*rs).ihrs_records).ihr_address+j;
      write_ep[1]=(addr>>8);
      write_ep[2]=(addr&0xff);
      write_ep[3]=*(*(*rs).ihrs_records).ihr_data;
      ch341SpiStream(write_ep, spi_data,4);
      printf("Addr: %d, hex addr: %d ", addr, (*(*rs).ihrs_records).ihr_address);
      printf("%d %d %d %x \n", write_ep[0], write_ep[1], write_ep[2], write_ep[3]);
      usleep(15000);
      (*(*rs).ihrs_records).ihr_data++;

    }

    (*rs).ihrs_records++;
  }
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
  "-d DEVICE - specifies AVR chip\n"
  "-r ADDR - read flash memory from start to ADDR in bytes, or\n"
  "-r f FILE read whole flash memory to specified file\n"
  "-w HEX - write flash memory from intel HEX file\n"
  "-e - erase device\n"
  "-c HEX - read and compare HEX with flash memory (strictly)\n"
  "-a HEX - automatically erase, write and check data in chip, or\n"
  "-a s HEX - same with strict verification instead of fast default\n"
  "use if you're flashing complex HEX, like Arduino bootloader or if default fails\n"
  "-p r - read content of on-chip EEPROM, print in terminal, or\n"
  "-p r FILE - read EEPROM to FILE\n"
  "-p w HEX - write eeprom from intel hex file\n"
  "-f - read fuse bits\n"
  "-l BYTE - write low fuse, must be in hex format, e.g. 0x2A\n"
  "-H BYTE - write high fuse, must be in hex format, e.g. 0x2A\n"
  "-x BYTE - write extended fuse, must be in hex format, e.g. 0x2A\n"
  "-L - read lock bits\n"
  "-t TIME - overwrite default writing timeout (4500 us), useful for slower chips \n");
}
