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


unsigned char data_buffer[TOT_MEM*2];//buffer for reading chip
unsigned char spi_data[4];

void toggle_reset(short stat);
void ch_exit(void);
void ch341init(void);
void chip_prog_enable(void);
void write_fuses(unsigned char fuse, int fuse_type);
char* read_fuses(void);
void check_signature(void);
void chip_erace();
void main_write_stream(const char * filename);
void check_flash(const char * filename);
void read_locks();
void print_help();
void ch_init(void);
void read_flash(int mem);


#endif
