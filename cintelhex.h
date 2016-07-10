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


#ifndef CINTELHEX_H
#define CINTELHEX_H

#define IHEX_DEBUG_OFF

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// CONSTANT DEFINITIONS

#define IHEX_ERR_INCORRECT_CHECKSUM   0x01
#define IHEX_ERR_NO_EOF               0x02
#define IHEX_ERR_PARSE_ERROR          0x03
#define IHEX_ERR_WRONG_RECORD_LENGTH  0x04
#define IHEX_ERR_NO_INPUT             0x05
#define IHEX_ERR_UNKNOWN_RECORD_TYPE  0x06
#define IHEX_ERR_PREMATURE_EOF        0x07
#define IHEX_ERR_ADDRESS_OUT_OF_RANGE 0x08
#define IHEX_ERR_MMAP_FAILED          0x09
#define IHEX_ERR_READ_FAILED          0x0B
#define IHEX_ERR_MALLOC_FAILED        0x0A

// TYPE DEFINITIONS

typedef unsigned int  uint_t;
typedef unsigned long ulong_t;

typedef enum { IHEX_DATA = 0x00, IHEX_EOF = 0x01, IHEX_ESA = 0x02,
               IHEX_SSA = 0x03, IHEX_ELA = 0x04, IHEX_SLA = 0x05 } ihex_rtype_t;
typedef enum { IHEX_WIDTH_8BIT = 1, IHEX_WIDTH_16BIT = 2,
               IHEX_WIDTH_32BIT = 4, IHEX_WIDTH_64BIT = 8 } ihex_width_t;
typedef enum { IHEX_ORDER_BIGENDIAN, IHEX_ORDER_LITTLEENDIAN,
	       IHEX_ORDER_NATIVE } ihex_byteorder_t;

typedef uint8_t* ihex_rdata_t;
typedef uint8_t  ihex_rlen_t;
typedef uint8_t  ihex_rchks_t;
typedef uint_t   ihex_error_t;
typedef uint16_t ihex_addr_t;

// STRUCTURE DEFINITIONS

/// Models a single Intel HEX record.
/** This structure models a single Intel HEX record, i.e. a single line
 *  in an Intel HEX file. Each record basically consists of a type, a
 *  base address and a data segment of variable length. */
typedef struct ihex_record {
	unsigned int ihr_length;   //!< Length of the record in bytes.
	ihex_rtype_t ihr_type;     //!< Record type (see ihex_rtype_t).
	ihex_addr_t  ihr_address;  //!< Base address offset.
	ihex_rdata_t ihr_data;     //!< Record data.
	ihex_rchks_t ihr_checksum; //!< The record's checksum.
} ihex_record_t;

/// Models a set of Intel HEX records.
/** This structure models an entire set of Intel HEX record, i.e. a
 *  complete Intel HEX input file. Basically, it just consists of a list
 *  of ihex_record_t structures.
 *  The last record must be a special EOF record. */
typedef struct ihex_recordset {
	uint_t         ihrs_count;   //!< Amount of records.
	ihex_record_t *ihrs_records; //!< A list of record (with ihrs_count elements).
} ihex_recordset_t;

// GLOBAL VARIABLES

#ifdef IHEX_PARSE_C
static ihex_error_t ihex_last_errno = 0;
static char*        ihex_last_error = NULL;
#else
extern ihex_error_t ihex_last_errno; //!< Error code of last error.
extern char*        ihex_last_error; //!< Description of last error.
#endif

// METHOD DECLARATIONS

/// Parse Intel HEX string from file.
/** This method parses an Intel HEX string from a file. The file will be
 *  mapped into memory and then parsed. This method returns a pointer to
 *  a newly generated ihex_recordset_t object.
 * 
 *  @param filename The filename of the input file.
 *  @return         A pointer to a newly generated recordset object. */
ihex_recordset_t* ihex_rs_from_file(const char* filename);

/// Parse Intel HEX string from memory input.
/** This method parses an Intel HEX string stored in memory. This
 *  method returns a pointer to a newly generated ihex_recordset_t
 *  object.
 * 
 *  @param data The input start address.
 *  @param size The input size in bytes.
 *  @return         A pointer to a newly generated recordset object. */
ihex_recordset_t* ihex_rs_from_mem(const char* data, size_t size);

/// Parse Intel HEX string from string input.
/** This method parses an Intel HEX string from a string input. This
 *  method returns a pointer to a newly generated ihex_recordset_t
 *  object.
 * 
 *  @param data The input string (NUL-terminated).
 *  @return         A pointer to a newly generated recordset object. */
ihex_recordset_t* ihex_rs_from_string(const char* data);

/// Iterate over all records in a record set.
/** This method should be called repeatedly to process all data
 *  records in a record set.  A counter variable must be provided to
 *  track the progress, initially set to zero.  When reaching the end
 *  of the record set, the counter is reset to zero and NULL is
 *  returned as the current record.  End-Of-File and address offset
 *  records are automatically handled, returning the offset address to
 *  be applied to the current record.  Example:
 * 
 *  @code
 *    uint_t i = 0;
 *    ihex_record_t *record;
 *    uint32_t offset;
 *    int err;
 *    do {
 *       err = ihex_rs_iterate_data(rs, &i, &record, &offset);
 *       if (err || record == 0) break;
 *       printf("record %u at %lu with length %u\n",
 *              i, offset + record->ihr_address, record->ihr_length);
 *    } while (i > 0);
 *  @endcode
 *
 *  @param rs  [in] A pointer to the record set.
 *  @param i   [in,out] Track the number of the next record to process
 *  @param rec [out] A pointer to the current record, not updated if NULL is passed
 *  @param off [out] Offset to apply to the record's address field
 *  @return    0 on success, an error code otherwise. */
int ihex_rs_iterate_data(ihex_recordset_t* rs, uint_t *i, ihex_record_t **rec, uint32_t *off);

/// Gets a record set's size.
/** This method determines a record set's size. This is done by adding
 *  the length of all records, however without regard to address offsets
 *  etc.
 * 
 *  @param rs A pointer to the record set.
 *  @return   The record set's size. */
ulong_t ihex_rs_get_size(ihex_recordset_t* rs);

/// Finds the address range which is contained in a record set.
/** This method determines a record set's size with regard to address
 *  offsets and record lengths.  The boundaries specify a memory
 *  region large enough to hold all contained data, for example:
 * 
 *  @code
 *    uint32_t min, max;
 *    ihex_rs_get_address_range(rs, &min, &max);
 *    void *data = malloc(max - min);
 *  @endcode
 * 
 *  @param rs  A pointer to the record set.
 *  @param min [out] Lowest address contained.
 *  @param max [out] Upper address boundary (just past highest address).
 *  @return    0 on success, an error code otherwise. */
int ihex_rs_get_address_range(ihex_recordset_t* rs, uint32_t *min, uint32_t *max);

/// Frees resources associated with a record set.
/** This method frees all memory allocated in one of the
 *  ihex_rs_from_*() functions.  Passing NULL as input is allowed and
 *  has no effect.
 * 
 *  @param rs A pointer to the record set. */
void ihex_rs_free(ihex_recordset_t* rs);

/// Return error code, or 0 if no error occurred.
/** This method returns the error code of the latest error.
 *  @return The error code of the latest error. */
ihex_error_t ihex_errno(void);

/// Checks if a record's checksum is valid.
/** Validate the record by adding up all bytes of a record.
 *  Including the checksum, the lower 8 bits of the sum of all
 *  bytes must be 0x00.
 * 
 *  @param r The record that is to be validated.
 *  @return  0 if the record is correct, otherwise 1. */
int ihex_check_record(ihex_record_t *r);

/// Copy the content of a record set.
/** This method copies the content of a record set to a certain
 *  location in memory.
 * 
 *  @param rs  The record set that is to be copied.
 *  @param dst A pointer to the destination address.
 *  @param n   The size of the allocated target area.
 *  @param w   The width of data words to be copied.
 *  @param o   Defines whether data words are big or little endian.
 *  @return    0 on success, an error code otherwise. */
int ihex_mem_copy(ihex_recordset_t *rs, void* dst, ulong_t n, ihex_width_t w, ihex_byteorder_t o);

/// Fill a memory area with zeroes.
/** This method fills a whole memory area with zeros.
 * 
 *  @param dst Target area.
 *  @param n   The size of the target area.
 *  @return    0 on success, an error code otherwise. */
int ihex_mem_zero(void* dst, ulong_t n);

/// Return error string, or NULL if no error occurred.
char* ihex_error(void);

/// Parse 8-bit hex input.
uint8_t ihex_fromhex8(uint8_t *input);

/// Parse 16-bit hex input.
uint16_t ihex_fromhex16(uint8_t *input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
