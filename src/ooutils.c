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
 *   ---------
 *   ooutils.c
 *   OOmnik helpers implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "ooutils.h"


/**
 * read XML attr value, allocate memory and copy 
 */
extern int
oo_copy_xmlattr(xmlNode    *input_node,
		const char *attr_name,
		char       **result,
		size_t     *result_size)
{
     char *value;
     char *val_copy;
     int ret;

     value = (char*)xmlGetProp(input_node,  (const xmlChar *)attr_name);
     if (!value) return oo_FAIL;
     
     /* overwrite the previous value if any */
     if ((*result))
       free((*result));

     (*result_size) = strlen(value);
     val_copy = malloc((*result_size) + 1);
     if (!val_copy) {
	 xmlFree(value);
	 return oo_NOMEM;
     }

     strcpy(val_copy, value);
     xmlFree(value);

     /*printf("VALUE: %s\n\n", val_copy);*/

     (*result) = val_copy;

     return oo_OK;
}

extern int
oo_concat_paths(const char *parent_path,
		const char *child_filename,
		char       **result)
{
    char *filename;


    return oo_OK;
}


/* extract the dir path from the current filename */
extern int
oo_extract_dirpath(const char *filename,
		   char **path,
		   size_t *path_size)
{
    const char *c;
    char *p;
    size_t p_size = 0;
    size_t chunk_size = 0;

    for (c = filename; *c; c++) {
	if ('\\' == *c || '/' == *c) {
	    p_size += chunk_size + 1;
	    chunk_size = 0;
	    continue;
	}
	chunk_size++;
    }

    p = malloc(p_size + 1);
    if (!p) 
	return oo_NOMEM;

    strncpy(p, filename, p_size);
    p[p_size] = '\0';

    (*path) = p;
    (*path_size) = p_size;

    return oo_OK;
}
