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
#include "chavrprog.h"


int main(int argc, char * argv[]){
  if(argc == 1) {

    print_help();
    exit(0);
  }

  int opts;


  while ( (opts = getopt(argc,argv,"r:w:ea:c:fl:H:x:L")) != -1){
    switch (opts){

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
      ch_init();
      printf("Write device using %s\n",optarg);
      main_write_stream(optarg);
      break;

      case 'e':
      ch_init();
      printf("Erasing chip\n");
      chip_erace();
      break;

      case 'c':
      printf("Checking...\n");
      ch_init();
      check_flash(optarg);
      break;

      case 'a': printf("Erasing...\n");
      ch_init();
      chip_erace();
      printf("Writing...\n");
      main_write_stream(optarg);
      printf("Checking...\n");
      check_flash(optarg);
      break;

      case 'f':
      ch_init();
      printf("Fuses: low high ext\n");
      read_fuses();
      printf("\n");
      break;

      case 'l':
      ch_init();
      write_fuses((int)strtol(optarg, NULL, 16),0);
      break;

      case 'H':
      ch_init();
      write_fuses((int)strtol(optarg, NULL, 16),1);
      break;

      case 'x':
      ch_init();
      write_fuses((int)strtol(optarg, NULL, 16),2);
      break;

      case 'L':
      ch_init();
      read_locks();
      break;

      default:
      print_help();
      exit(0);
    };


  }


  ch_exit();

}
