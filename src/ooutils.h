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
 *   ooutils.h
 *   various OOmnik utilities
 */

#ifndef OOUTILS_H
#define OOUTILS_H

#include <libxml/parser.h>

extern int
oo_copy_xmlattr(xmlNode    *input_node,
		const char *attr_name,
		char       **result,
		size_t     *result_size);

extern int
oo_extract_dirpath(const char *filename,
		   char **path,
		   size_t *path_size);

extern int
oo_concat_paths(const char *parent_path,
		const char *child_filename,
		char       **result);
#endif
