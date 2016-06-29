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


#include "config.h"
#include "cintelhex.h"

#include <stdio.h>
#include <string.h>


#define IHEX_SET_ERROR(errnum, error, ...) \
	{ char *e = malloc(512); \
	  snprintf(e, 512, error, ##__VA_ARGS__); \
	  ihex_set_error(errnum, e); }
#define IHEX_SET_ERROR_RETURN(errnum, error, ...) \
	{ IHEX_SET_ERROR(errnum, error, ##__VA_ARGS__); \
	  return errnum; }


void ihex_set_error(ihex_error_t errnum, char* error);

int ihex_mem_copy(ihex_recordset_t *rs, void* dst, ulong_t n,
                  ihex_width_t w, ihex_byteorder_t o)
{
	int      r;
	uint_t   i = 0, j, l;
	uint32_t offset = 0x00, address = 0x00;
	
	ihex_rdata_t   d = (ihex_rdata_t) dst;
	ihex_record_t *x;
	
	if ((r = ihex_mem_zero(dst, n)) != 0)
	{
		return r;
	}
	
	do
	{
		r = ihex_rs_iterate_data(rs, &i, &x, &offset);
		if (r)
		{
			return r;
		}
		else if (x == 0)
		{
			break;
		}

		address = (offset + x->ihr_address);
		
		if (address >= n)
		{
			IHEX_SET_ERROR_RETURN(IHEX_ERR_ADDRESS_OUT_OF_RANGE,
				"Address 0x%08x is out of range", address);
		}
		
		for (j = 0; j < x->ihr_length; j += w)
		{
			uint32_t  v      = 0;
			uint32_t *target = (uint32_t*) (d + address + j);
			
			for (l = 0; (l < w) && (j + l < x->ihr_length); l ++)
			{
				v += x->ihr_data[j+l] << (8 * ((o == IHEX_ORDER_BIGENDIAN) ? ((w - 1) - l) : l));
			}
			*(target) = v;
			
			#ifdef IHEX_DEBUG
			printf("%08x -> %08x = %08x\n", address + j, v, *target);
			#endif
		}
	}
	while (i > 0);
	
	return 0;
}

int ihex_mem_zero(void* dst, ulong_t n)
{
#ifdef HAVE_MEMSET
	memset(dst, 0 , n);
#else
	ulong_t i = 0;
	
	for (i = 0; i < n; i ++)
	{
		((uint8_t*) dst)[i] = 0x00;
	}
#endif
	
	return 0;
}
