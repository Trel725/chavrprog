/*
 * (C) 2013  Martin Helmich <martin.helmich@hs-osnabrueck.de>
 *           Oliver Erxleben <oliver.erxleben@hs-osnabrueck.de>
 * 
 *           University of Applied Sciences Osnabrück
 * 
 * This file is part of the CIntelHex library (libcintelhex).
 * 
 * libcintelhex is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * libcintelhex is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libcintelhex.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#define IHEX_PARSE_C

#include "cintelhex.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#define IHEX_CHR_RECORDMARK 0x3A

#define IHEX_SET_ERROR(errnum, error, ...) \
	{ char *e = malloc(512); \
	  snprintf(e, 512, error, ##__VA_ARGS__); \
	  ihex_set_error(errnum, e); }
#define IHEX_SET_ERROR_RETURN(errnum, error, ...) \
	{ IHEX_SET_ERROR(errnum, error, ##__VA_ARGS__); \
	  return errnum; }

// Flags for open() syscall
#ifndef O_BINARY
#define O_BINARY 0
#endif


void ihex_set_error(ihex_error_t errnum, char* error);

static int ihex_parse_single_record(ihex_rdata_t data, unsigned int length, ihex_record_t* record);

ihex_recordset_t* ihex_rs_from_file(const char* filename)
{
	struct stat s;
	int         fd;
	ulong_t     l;
	char*       c;
	
	ihex_recordset_t* r;
	
	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
	{
		IHEX_SET_ERROR(IHEX_ERR_NO_INPUT, "Input file %s does not exist", filename);
		goto open_failed;
	}
	
	if (fstat (fd, &s) != 0)
	{
		IHEX_SET_ERROR(IHEX_ERR_NO_INPUT, "Could not stat input file %s", filename);
		goto stat_failed;
	}
	
	l = s.st_size;
#ifdef HAVE_MMAP
	if ((c = (char*) mmap(NULL, l, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*) -1)
	{
		IHEX_SET_ERROR(IHEX_ERR_MMAP_FAILED, "Could not map file %s", filename);
		goto mmap_failed;
	}
#else
	if ((c = (char*) malloc(l)) == NULL)
	{
		IHEX_SET_ERROR(IHEX_ERR_READ_FAILED, "Could not allocate memory for reading file %s", filename);
		goto malloc_failed;
	}

	ulong_t rest = l;
	while (rest > 0)
	{
		ssize_t bytes = read(fd, c + (l - rest), rest);
		if (bytes < 0)
		{
			IHEX_SET_ERROR(IHEX_ERR_READ_FAILED, "Could not read file %s", filename);
			goto read_failed;
		}
		else if (bytes == 0)
		{
			break;	//end of file
		}
		rest -= bytes;
	}
#endif
	
	r = ihex_rs_from_mem(c, l);
	
	// No special error treatment necessary, we need to unmap and close
	// the file anyway.
#ifdef HAVE_MMAP
	munmap((void*) c, l);
#else
	free(c);
#endif
	close(fd);
	
	return r;
	
	// Clean up on error.
#ifdef HAVE_MMAP
	mmap_failed:
#else
	read_failed:
		free(c);
	malloc_failed:
#endif
	stat_failed:
		close(fd);
	open_failed:
	
	return NULL;
}

ihex_recordset_t* ihex_rs_from_mem(const char* data, size_t size)
{
	uint_t i = 0;
	int    r = 0, c = 0;
	const char *end = data + size;
	
	ihex_last_errno = 0;
	ihex_last_error = NULL;

	ihex_record_t    *rec;
	ihex_recordset_t *recls;
	
	// Count number of record marks in input string.
	for (const char *p = data; p < end && *p != 0x00; p ++)
	{
		if (*p == IHEX_CHR_RECORDMARK)
		{
			c ++;
		}
	}
	
	// Allocate memory for the record container structure and for each
	// individual record.
	if ((rec = (ihex_record_t*) calloc(c, sizeof(ihex_record_t))) == NULL)
	{
		IHEX_SET_ERROR(IHEX_ERR_MALLOC_FAILED, "Could not allocate memory");
		goto malloc_rec_failed;
	}

	if ((recls = (ihex_recordset_t*) malloc(sizeof(ihex_recordset_t))) == NULL)
	{
		IHEX_SET_ERROR(IHEX_ERR_MALLOC_FAILED, "Could not allocate memory");
		goto malloc_recls_failed;
	}
	
	recls->ihrs_count   = c;
	recls->ihrs_records = rec;
	
	while (data < end && *(data) != 0x00)
	{
		i ++;
		
		if (data + 3 >= end)
		{
			break;
		}

		ihex_rlen_t l = ihex_fromhex8(((ihex_rdata_t) data) + 1);
		if (data + 9 + l * 2 >= end ||
		    (r = ihex_parse_single_record((ihex_rdata_t) data, l, rec + i - 1)) != 0)
		{
			IHEX_SET_ERROR(r, "Line %i: %s", i, ihex_last_error);
			goto parse_single_failed;
		}
		
		data += (rec[i - 1].ihr_length * 2) + 10;
		while (data < end && *(data) != IHEX_CHR_RECORDMARK && *(data) != 0x00)
		{
			data ++;
		}
	}
	
	if (recls->ihrs_records[recls->ihrs_count - 1].ihr_type != IHEX_EOF)
	{
		IHEX_SET_ERROR(IHEX_ERR_NO_EOF, "Missing EOF record");
		goto missing_eof_record;
	}
	
	return recls;

	missing_eof_record:
		for (i = 0; i < recls->ihrs_count; i ++)
		{
			free(recls->ihrs_records[i].ihr_data);
		}
	parse_single_failed:
		free(recls);
	malloc_recls_failed:
		free(rec);
	malloc_rec_failed:

	return NULL;
}

ihex_recordset_t* ihex_rs_from_string(const char* data)
{
	return ihex_rs_from_mem(data, strlen(data));
}

static int ihex_parse_single_record(ihex_rdata_t data, unsigned int length, ihex_record_t* record)
{
	uint_t i;
	
	// Records needs to begin with record mark (usually ":")
	if (*(data) != IHEX_CHR_RECORDMARK)
	{
		IHEX_SET_ERROR_RETURN(IHEX_ERR_PARSE_ERROR, "Missing record mark");
	}
	
	// Record layout:
	//               1         2         3         4
	// 0 12 3456 78 90123456789012345678901234567890 12
	// : 10 0100 00 214601360121470136007EFE09D21901 40
	
	record->ihr_length   = (ihex_rlen_t)  length;
	record->ihr_address  = (ihex_addr_t)  ihex_fromhex16(data + 3);
	record->ihr_type     = (ihex_rtype_t) ihex_fromhex8 (data + 7);
	record->ihr_checksum = (ihex_rchks_t) ihex_fromhex8 (data + 9 + record->ihr_length * 2);

	if ((record->ihr_data = (ihex_rdata_t) malloc(record->ihr_length)) == NULL)
	{
		IHEX_SET_ERROR_RETURN(IHEX_ERR_MALLOC_FAILED, "Could not allocate memory");
	}
	
	// Records needs to end with CRLF or LF.
	if (   (   data[11 + record->ihr_length * 2] != 0x0D
	        || data[12 + record->ihr_length * 2] != 0x0A)
	    && (data[11 + record->ihr_length * 2] != 0x0A))
	{
		free(record->ihr_data);
		IHEX_SET_ERROR_RETURN(IHEX_ERR_WRONG_RECORD_LENGTH, "Incorrect record length");
	}
	
	for (i = 0; i < record->ihr_length; i ++)
	{
		if (data[9 + i*2] == 0x0A || data[9 + i*2] == 0x0D)
		{
			free(record->ihr_data);
			IHEX_SET_ERROR_RETURN(IHEX_ERR_WRONG_RECORD_LENGTH, "Unexpected end of line");
		}
		record->ihr_data[i] = ihex_fromhex8(data + 9 + i*2);
	}
	
	if (ihex_check_record(record) != 0)
	{
		free(record->ihr_data);
		IHEX_SET_ERROR_RETURN(IHEX_ERR_INCORRECT_CHECKSUM, "Checksum validation failed");
	}
	
	return 0;
}

int ihex_check_record(ihex_record_t *r)
{
	uint_t  i;
	uint8_t t = 0;
	
	t += r->ihr_length + ((r->ihr_address >> 8) & 0xFF)
		+ (r->ihr_address & 0xFF) + r->ihr_type + r->ihr_checksum;
	
	for (i = 0; i < r->ihr_length; i ++)
	{
		t += r->ihr_data[i];
	}
	
	return ((t & 0xFF) == 0) ? 0 : 1;
}

ihex_error_t ihex_errno()
{
	return ihex_last_errno;
}

char* ihex_error()
{
	return ihex_last_error;
}

void ihex_set_error(ihex_error_t errnum, char* error)
{
	ihex_last_errno = errnum;
	ihex_last_error = error;
				
	#ifdef IHEX_DEBUG
	printf(error);
	#endif
}

static inline uint8_t ihex_fromhex4(uint8_t i)
{
	if      (i >= 0x61 && i <= 0x66) return i - 0x61 + 10;
	else if (i >= 0x41 && i <= 0x46) return i - 0x41 + 10;
	else if (i >= 0x30 && i <= 0x39) return i - 0x30;
	else return 0;
}

uint8_t ihex_fromhex8(uint8_t *i)
{
	return (ihex_fromhex4(i[0]) << 4) + ihex_fromhex4(i[1]);
}

uint16_t ihex_fromhex16(uint8_t *i)
{
	return (ihex_fromhex4(i[0]) << 12) + (ihex_fromhex4(i[1]) << 8) + 
	       (ihex_fromhex4(i[2]) << 4) + ihex_fromhex4(i[3]);
}
