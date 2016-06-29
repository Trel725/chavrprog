/*
 * (C) 2013, 2014
 *           Martin Helmich <martin.helmich@hs-osnabrueck.de>
 *           Oliver Erxleben <oliver.erxleben@hs-osnabrueck.de>
 * 
 *           University of Applied Sciences Osnabrück
 * 
 *           Andre Colomb <src@andre.colomb.de>
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


#include "cintelhex.h"

#include <stdio.h>


#define IHEX_SET_ERROR(errnum, error, ...) \
	{ char *e = malloc(512); \
	  snprintf(e, 512, error, ##__VA_ARGS__); \
	  ihex_set_error(errnum, e); }
#define IHEX_SET_ERROR_RETURN(errnum, error, ...) \
	{ IHEX_SET_ERROR(errnum, error, ##__VA_ARGS__); \
	  return errnum; }


void ihex_set_error(ihex_error_t errnum, char* error);

ulong_t ihex_rs_get_size(ihex_recordset_t* rs)
{
	ulong_t s = 0;
	uint_t  i = 0;
	
	for (i = 0; i < rs->ihrs_count; i ++)
	{
		s += rs->ihrs_records[i].ihr_length;
	}
	
	return s;
}

int ihex_rs_iterate_data(ihex_recordset_t* rs, uint_t *i, ihex_record_t **rec, uint32_t *off)
{
	uint32_t offset;
	
	ihex_record_t *x;
	
	if (*i == 0 && off)
	{
		*off = 0x00;
	}
	
	for (; *i < rs->ihrs_count; ++(*i))
	{
		x = (rs->ihrs_records + *i);
		if (rec)
		{
			*rec = x;	//point the caller to current record
		}
		
		switch (x->ihr_type)
		{
			case IHEX_DATA:
				++(*i);		//proceed to next record in next call
				return 0;	//let the caller process the data

			case IHEX_EOF:
				if (*i < rs->ihrs_count - 1)
				{
					IHEX_SET_ERROR_RETURN(IHEX_ERR_PREMATURE_EOF,
						"Premature EOF in record %i", *i + 1);
				}
				else
				{
					//FIXME signal end of records
					*i = 0;
					if (rec)
					{
						*rec = 0;
					}
					return 0;
				}
			case IHEX_ESA:
				offset = *(x->ihr_data) << 4;
				if (off)
				{
					*off = offset;
				}
				
				#ifdef IHEX_DEBUG
				printf("Switched offset to 0x%08x.\n", offset);
				#endif
				
				break;
			case IHEX_ELA:
				offset = (x->ihr_data[0] << 24) + (x->ihr_data[1] << 16);
				if (off)
				{
					*off = offset;
				}
				
				#ifdef IHEX_DEBUG
				printf("Switched offset to 0x%08x.\n", offset);
				#endif
				
				break;
			case IHEX_SSA:
			case IHEX_SLA:
				//skip body; next
				break;
			default:
				IHEX_SET_ERROR_RETURN(IHEX_ERR_UNKNOWN_RECORD_TYPE,
					"Unknown record type in record %i: 0x%02x",
					*i+1, x->ihr_type);
		}
	}

	IHEX_SET_ERROR_RETURN(IHEX_ERR_NO_EOF, "Missing EOF record");
}

int ihex_rs_get_address_range(ihex_recordset_t* rs, uint32_t *min, uint32_t *max)
{
	int r;
	uint_t   i = 0;
	uint32_t offset = 0x00, address = 0x00, dummy_min, dummy_max;
	
	ihex_record_t *x;
	
	// Initialize range boundaries
	if (min == NULL)
	{
		min = &dummy_min;
	}
	if (max == NULL)
	{
		max = &dummy_max;
	}

	*min = UINT32_MAX;
	*max = 0x00;
	
	do {
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
		
		if (address < *min)
		{
			*min = address;
		}
		if (address + x->ihr_length > *max)
		{
			*max = address + x->ihr_length;
		}
	}
	while (i > 0);

	return 0;
}

void ihex_rs_free(ihex_recordset_t* rs)
{
	uint_t i = 0;
	
	if (rs == NULL)
	{
		return;
	}
	
	if (rs->ihrs_records != NULL)
	{
		for (i = 0; i < rs->ihrs_count; i++)
		{
			free(rs->ihrs_records[i].ihr_data);
		}
	}
	
	free(rs->ihrs_records);
	free(rs);
}
