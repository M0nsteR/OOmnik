/*******************************************************
 * OOmnik Denotation Map aka Tree2Tree Dictionary 
 *
 * SVN FILE: $Id: oomindmap.h 6 2009-05-27 10:38:12Z dmitri $
 *
 * $URL: svn+ssh://oomnik.iling.spb.ru/var/repos/oomnik/trunk/oodenotmap.h $
 *
 * $Revision: 6 $
 * $Date: 2009-05-27 14:38:12 +0400 (Срд, 27 Май 2009) $
 * $LastChangedBy: dmitri $
 * $LastChangedDate: 2009-05-27 14:38:12 +0400 (Срд, 27 Май 2009) $
 * 
 *  Project Name : OOmnik
 *
 * $Author: dmitri $
 * $copyright $
 * $license $
 ********************************************************/

#ifndef OODENOTMAP_H
#define OODENOTMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <db.h>

#include "ooconcept.h"

typedef struct ooDenotMap 
{
    unsigned long id;


    /***********  public methods ***********/
    int (*del)(struct ooDenotMap *self);
    wchar_t* (*str)(struct ooDenotMap *self);

    int (*open)(struct ooDenotMap* self, const char *name);

    int (*close)(struct ooDenotMap *self);


    /***********  private attributes ***********/

    /* long-term memory:
       persistent storage implemented as
       Berkeley DB backend */
    DB *_storage;

    /* fixed-size short-term operational memory */
    struct ooConcept *_cache_storage;
    size_t num_cache_items;

} ooDenotMap;

extern int ooDenotMap_init(struct ooDenotMap**); 


#endif /* OODENOTMAP_H */
