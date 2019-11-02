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

#ifndef CHAVRPROG_H
#define CHAVRPROG_H
#include "config.h"

#define PAGESIZE cfg_pagesize
#define NUM_OF_PAGES cfg_num_of_pages
#define TOT_MEM cfg_pagesize*cfg_num_of_pages //size of memory in words
#define PAGE_SHIFT cfg_pageshift
#define PAGE_MASK cfg_pagemask



unsigned char * data_buffer;
unsigned char spi_data[4];
unsigned char device_sign[3];
int cfg_pagesize;
int cfg_num_of_pages;
int cfg_eeprom;
unsigned cfg_pageshift;
unsigned cfg_pagemask;
int cfg_pagemsq;

void debug_call(void);
void toggle_reset(short stat);
void ch_exit(void);
void ch341init(void);
void chip_prog_enable(void);
void write_fuses(unsigned char fuse, int fuse_type);
char* read_fuses(void);
void check_signature(void);
void chip_erace(void);
void main_write_stream(const char * filename);
void check_flash(const char * filename);
void read_locks(void);
void print_help(void);
void ch_init(void);
void read_flash(int mem);
void assign_cfg(int index);
void check_flash_strict(const char * filename);
void read_eeprom(void);
void write_eeprom(const char * filename);

#endif
