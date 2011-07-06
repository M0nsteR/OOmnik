/**
 *   Copyright (c) 2011 by Dmitri Dmitriev
 *   All rights reserved.
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   Project homepage:
 *   <http://www.oomnik.ru>
 *
 *   Initial author and maintainer:
 *         Dmitri Dmitriev aka M0nsteR <dmitri@globbie.net>
 *
 *   -----------
 *   ooconfig.h
 *   OOmnik configuration settings
 */

#ifndef OO_CONFIG_H
#define OO_CONFIG_H

/* return error codes */
enum { oo_OK, oo_FAIL, FAIL, oo_NOMEM, oo_NO_RESULTS } oo_err_codes;


#ifdef WIN32
  #ifndef NO_BUILD_DLL
      #define EXPORT __declspec(dllexport)
  #else
      #define EXPORT __declspec(dllimport)
  #endif
#else
  #define EXPORT
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

#include <stdbool.h>

#else

#undef bool
#undef true
#undef false

#define bool unsigned char
#define true  1
#define false 0

#endif


#define INPUT_BUF_SIZE 256
/*#define SEGM_SIZE 512*/

#define AGENDA_CONCUNIT_STORAGE_SIZE 1024
#define AGENDA_INDEX_SIZE 1024

/* max number of complex solutions in a batch */
#define AGENDA_COMPLEX_POOL_SIZE 24
#define CONCUNIT_COMPLEX_POOL_SIZE 4

#define INTERP_POOL_SIZE 4

#define TOPIC_POOL_SIZE 12

#define DEFAULT_CONCEPT_COMPLEXITY 1.0


#define AGENDA_COMPLEX_INDEX_SIZE \
     AGENDA_CONCUNIT_STORAGE_SIZE * CONCUNIT_COMPLEX_POOL_SIZE

#define NUM_GROUP_ITEMS 1

#define OUTPUT_BUF_SIZE 1024 * 1024
#define TEMP_BUF_SIZE 1024

#define INDEX_REALLOC_FACTOR 2
#define DEFAULT_INDEX_SIZE 1024

#define NUM_CACHE_TAILS 128

/* hash table */
#define DICT_INIT_SIZE 10000

#define STORAGE_CACHE_SIZE 1024

/* debugging output levels */
#define DEBUG_LEVEL_1 0
#define DEBUG_LEVEL_2 0
#define DEBUG_LEVEL_3 0
#define DEBUG_LEVEL_4 0

#define DEBUG_CS_LEVEL_1 0
#define DEBUG_CS_LEVEL_2 0
#define DEBUG_CS_LEVEL_3 0
#define DEBUG_CS_LEVEL_4 0

#define DEBUG_SEGM_LEVEL_1 0
#define DEBUG_SEGM_LEVEL_2 0
#define DEBUG_SEGM_LEVEL_3 0
#define DEBUG_SEGM_LEVEL_4 0

#define DEBUG_CACHE_LEVEL_1 0
#define DEBUG_CACHE_LEVEL_2 0
#define DEBUG_CACHE_LEVEL_3 0
#define DEBUG_CACHE_LEVEL_4 0

#define DEBUG_CONC_LEVEL_1 0
#define DEBUG_CONC_LEVEL_2 0
#define DEBUG_CONC_LEVEL_3 0
#define DEBUG_CONC_LEVEL_4 0

#define DEBUG_COMPLEX_LEVEL_1 0
#define DEBUG_COMPLEX_LEVEL_2 0
#define DEBUG_COMPLEX_LEVEL_3 0
#define DEBUG_COMPLEX_LEVEL_4 0

#define DEBUG_AGENDA_LEVEL_1 0
#define DEBUG_AGENDA_LEVEL_2 0
#define DEBUG_AGENDA_LEVEL_3 0
#define DEBUG_AGENDA_LEVEL_4 0

#define NUM_TEST_RUNS 1

/* grades from 0 to 1:  total 256 grades */
#define grade_t unsigned char

/* max number of atrributes in a Concept */
#define attr_size_t unsigned short
#define ATTR_MAX (unsigned short)-1

#define NAME_ATOMS_MAX 255

/* max number of offsets 
 * in string representation of concepts */
#define STR_DEPTH_LIMIT 12

/* num of whitespace chars */
#define OFFSET_SIZE 2
#define SPACE_CHAR 32

/* max number of concepts in the MindMap */
#define mindmap_size_t size_t /*unsigned long*/


#define ooATOM const unsigned char
#define MAX_UNREC_ATOMS 3


/* penalty for each missing unit in coverage */
#define COMPLEX_PENALTY 10

/* weight gain bonus for correct linking */
#define OPER_SUCCESS_BONUS 5


/* RAG binary packing: size in bytes */
#define CONCID_BIN_SIZE 8
#define ATTR_BIN_SIZE 1
#define POS_BIN_SIZE 4
#define CONCUNIT_TYPE_SIZE 2

/* syntax */
#define MAX_PARENTS 16

#define MAX_CONTACT_DISTANCE 4

#define ATOM_BYTE 1

/* caching in bytes */
#define MAX_MEMCACHE_SIZE 160 * 1024 * 1024
#define DEFAULT_MATRIX_DEPTH 3
#define DEFAULT_MAX_UNREC_CHARS 10

#define UCS2_MAX 65535
#define NUMERIC_CODE_TYPE size_t

typedef enum pack_type { PACK_COMPACT, 
                         PACK_RAG,
                         PACK_ROBUST,
			 PACK_SECURE,
			 PACK_XML
                       } pack_type;

typedef enum output_type {  FORMAT_JSON, 
			    FORMAT_XML
} output_type;


typedef int (*oo_compar_func)(const void *item, 
			      const void *another_item);

#endif
