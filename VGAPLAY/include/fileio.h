/*
 * fileio.h
 *
 *  Created on: 2017
 *      Author: netdex
 * 
 *  Distributed under the MIT License. See README.TXT for details.
 */


#ifndef INCLUDE_FILEIO_H_
#define INCLUDE_FILEIO_H_

#include "types.h"
#include "error.h"

#define ERROR_FILE "could not read file"

void read_byte(FILE *fp, byte *i);
void read_word(FILE *fp, word *i);
void read_int(FILE *fp, int *i);
void read_bin(FILE *fp, void *array, size_t arrayBytes);

#endif /* INCLUDE_FILEIO_H_ */
