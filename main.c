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
#include <cintelhex.h>
#include <unistd.h>

//device parameters settings

const unsigned char device_sign[3]={0x1e,0x95,0x87};
#define PAGESIZE 64
#define NUM_OF_PAGES 256


//end of device parameters
#define TOT_MEM PAGESIZE*NUM_OF_PAGES//size of memory in words
#if PAGESIZE == 64
#define PAGE_SHIFT 7
#define PAGE_MSQ 0x7f
#elif PAGESIZE == 32
#define PAGE_SHIFT 6
#define PAGE_MSQ 0x3f
#elif PAGESIZE == 16
#define PAGE_MSQ 0x1f
#define PAGE_SHIFT 5
#endif

unsigned char data_buffer[TOT_MEM*2];
unsigned char spi_data[4];

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
    exit(-1);
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
    exit(-1);
  }
}

void chip_erace(){
  ch341SpiStream(chip_er,spi_data,4);
  usleep(10000);
}

void read_chip(int start, int end){
  int j = 0;
  for(uint16_t i=start; i<end;i++){
    read_pgl[1]=(i>>8);  //direct bits to appropriate registers
    read_pgl[2]=(i & 0xff);
    ch341SpiStream(read_pgl,spi_data,4);
    data_buffer[j]=spi_data[3];
    j++;

    read_pgh[1]=(i>>8);
    read_pgh[2]=(i & 0xff);
    ch341SpiStream(read_pgh,spi_data,4);
    data_buffer[j]=spi_data[3];
    j++;
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
      usleep(5000);

    }

    (*rs).ihrs_records++;
  }

}

void check_device(const char * filename){
  ihex_recordset_t* rs = ihex_rs_from_file(filename);
  unsigned  char* dst = malloc(TOT_MEM*2);
  ihex_mem_copy(rs, dst, TOT_MEM*2, IHEX_WIDTH_16BIT, IHEX_ORDER_LITTLEENDIAN);

  read_chip(0, (ihex_rs_get_size(rs)/2));

  for(int i=0; i<ihex_rs_get_size(rs); i++){

    if(data_buffer[i]==*dst){
      //    printf("Byte %d is OK \n", i);

    }
    else{
      printf("Byte %d is corrupted, %x, %x\n", i, data_buffer[i], *dst);
      exit(-1);
    }
    dst++;
  }
  printf("%ld bytes successfully checked\n", ihex_rs_get_size(rs));
}


void read_locks(){
  ch341SpiStream(read_lk, spi_data,4);
  printf("Lock byte: %x\n", spi_data[3]);
}


void print_help(){
printf("Usage:\n"
"-r ADDR - read flash memory from start to ADDR in words (max addr = capacity in bytes/2)\n"
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

int main(int argc, char * argv[]){
  if(argc == 1) {

    print_help();
    exit(0);
  }

  ch341_init();
  chip_prog_enable();
  check_signature();

  int opts;


  while ( (opts = getopt(argc,argv,"r:w:ea:c:fl:h:x:L")) != -1){
    switch (opts){

      case 'r':
      read_chip(0, atoi(optarg));
      for(int i=0; i<atoi(optarg); i++){
        printf("%x ", data_buffer[i]);
      }

      break;

      case 'w':
      printf("Write device using %s\n",optarg);
      main_write_stream(optarg);
      break;

      case 'e': printf("Erasing chip\n");
      chip_erace();
      break;

      case 'c':
      printf("Checking...\n");
      check_device(optarg);
      break;

      case 'a': printf("Erasing...\n");
      chip_erace();
      printf("Writing...\n");
      main_write_stream(optarg);
      printf("Checking...\n");
      check_device(optarg);
      break;

      case 'f':
      printf("Fuses: low high ext\n");
      read_fuses();
      printf("\n");
      break;

      case 'l':
      write_fuses((int)strtol(optarg, NULL, 16),0);
      break;

      case 'h':
      write_fuses((int)strtol(optarg, NULL, 16),1);
      break;

      case 'x':
      write_fuses((int)strtol(optarg, NULL, 16),2);
      break;

      case 'L':
      read_locks();
      break;

      default:
      print_help();
    };

  }

}
