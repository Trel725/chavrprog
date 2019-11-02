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
#include "config.h"

int file_exists(const char * filename){
  FILE *file= fopen(filename, "r");
  if (file!=0){
    fclose(file);
    return 1;
  }
  return 0;
}

int delay = 4500; //default value of writing delay

int main(int argc, char * argv[]){
  if(argc == 1) {

    print_help();
    exit(0);
  }

  int opts;


  while ( (opts = getopt(argc,argv,"d:r:w:ea:c:fl:H:x:t:Lp:")) != -1){
    switch (opts){


      case 'd':
      for(int i=0; i<(sizeof confset / sizeof confset[0]); i++){
        if(strcmp(optarg, confset[i].name)==0){
          assign_cfg(i);
          break;
        }
        if(i==((sizeof confset / sizeof confset[0])-1)){
          printf("No such chip\n");
          printf("Available devices:\n");
          for(int i=0; i<CONF_LENGTH; i++){
            printf("%s\n", confset[i].name);
          }
          exit(0);
        }
      }

      break;



      case 'r':


      if(optarg[0]=='f'){
        if(argv[optind]!=NULL) {
          ch_init();
          printf("Reading to %s...\n", argv[optind]);
          read_flash(TOT_MEM*2);
          FILE *fp;
          fp = fopen(argv[optind], "w");
          fwrite(data_buffer, 1, (TOT_MEM*2), fp);
          fclose(fp);

        }
        else
        printf("Please, specify file\n");
      }
      else {

        ch_init();
        read_flash(atoi(optarg));
        for(int i=0; i<atoi(optarg); i++){
          printf("%x ", data_buffer[i]);
        }
      }

      break;

      case 'w':
      if(file_exists(optarg)){
        ch_init();
        printf("Write device using %s\n",optarg);
        main_write_stream(optarg);
      }
      else printf("No such file\n");
      break;

      case 'e':
      ch_init();
      printf("Erasing chip\n");
      chip_erace();
      break;

      case 'c':
      if(file_exists(optarg)){
        printf("Checking...\n");
        ch_init();
        check_flash_strict(optarg);
      }
      else  printf("No such file\n");
      break;

      case 't':
        delay = (int)strtol(optarg, NULL, 0);
        debug_call();

      case 'a':
      if(argv[optind]==NULL) {
        if(file_exists(optarg)){
          printf("%s", argv[optind]);
          printf("Erasing...\n");
          ch_init();
          chip_erace();
          printf("Writing...\n");
          main_write_stream(optarg);
          printf("Checking...\n");
          check_flash(optarg);
        }
        else printf("No such file\n");
      }
      else if(*optarg=='s'){
        if(file_exists(argv[optind])){
          printf("Erasing...\n");
          ch_init();
          chip_erace();
          printf("Writing...\n");
          main_write_stream(argv[optind]);
          printf("Checking strictly...\n");
          check_flash_strict(argv[optind]);
        }
        else printf("No such file\n");
      }
      break;

      case 'f':
      ch_init();
      printf("Fuses: low high ext\n");
      read_fuses();
      printf("\n");
      break;


      int fuse_val = -1;
      case 'l':
      ch_init();
      fuse_val = (int)strtol(optarg, NULL, 0);
      if ((fuse_val >= 0) && (fuse_val < 256)){
        write_fuses(fuse_val,0);
      }
      else{
        printf("Wrong fuse value, refusing to write...\n");
      }
      break;

      case 'H':
      ch_init();
      fuse_val = (int)strtol(optarg, NULL, 0);
      if ((fuse_val >= 0) && (fuse_val < 256)){
        write_fuses(fuse_val,1);
      }
      else{
        printf("Wrong fuse value, refusing to write...\n");
      }
      break;

      case 'x':
      ch_init();
      fuse_val = (int)strtol(optarg, NULL, 0);
      if ((fuse_val >= 0) && (fuse_val < 256)){
        write_fuses(fuse_val,2);
      }
      else{
        printf("Wrong fuse value, refusing to write...\n");
      }
      break;

      case 'L':
      ch_init();
      read_locks();
      break;

      case 'p':
      if(optarg[0]=='r'){
        if(argv[optind]!=NULL) {
          ch_init();
          read_eeprom();
          printf("Reading to %s...\n", argv[optind]);
          FILE *fp;
          fp = fopen(argv[optind], "w");
          fwrite(data_buffer, 1, cfg_eeprom, fp);
          fclose(fp);
        }
        else{
          ch_init();
          read_eeprom();
          for (int i=0; i<cfg_eeprom; i++){
            printf("%x ", data_buffer[i]);
          }
        }
      }
      else if(optarg[0]=='w' && argv[optind]!=NULL){
        printf("Writing eeprom using %s\n", argv[optind]);
        write_eeprom(argv[optind]);
      }
      else{
        print_help();
      }
      break;

      case 'v':
      if(file_exists(optarg)){
        ch_init();
        printf("Write eeprom using %s\n",optarg);
        write_eeprom(optarg);
      }
      else printf("No such file\n");
      break;

      default:
      print_help();
      exit(0);
    };


  }


  ch_exit();

}
