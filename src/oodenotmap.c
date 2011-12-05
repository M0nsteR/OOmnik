/******************************************************* 
 * OOmnik Persistent Storage implementation
 *
 * SVN FILE: $Id: oomindmap.c 6 2009-05-27 10:38:12Z dmitri $
 *
 * $URL: svn+ssh://oomnik.iling.spb.ru/var/repos/oomnik/trunk/oodenotmap.c $
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <db.h>
#include <libxml/parser.h>

#include "ooconfig.h"
#include "oodenotmap.h"

/*  DenotMap Destructor */
static int
ooDenotMap_del(ooDenotMap *self)
{
    int ret;
    if (self->_storage != NULL) {
	ret = self->_storage->close(self->_storage, 0);
    }
    return oo_OK;
}

/*  string representation */
static wchar_t *
ooDenotMap_str(ooDenotMap *self)
{
    return L"Denotation Map string representation";
}

/*  opening the DenotMap Database */
static int
ooDenotMap_open(ooDenotMap *self, const char *dbpath)
{
    int ret;
    DB *dbp = self->_storage;

    /* open the database */
    if ((ret = dbp->open(dbp,        /* DB structure pointer */ 
			 NULL,       /* Transaction pointer */ 
			 dbpath,     /* On-disk file that holds the database. */
			 NULL,       /* Optional logical database name */
			 DB_BTREE,   /* Database access method */
			 DB_CREATE,  /* Open flags */
			 0664))      /* File mode (using defaults) */
                      != oo_OK) {
	dbp->err(dbp, ret, "%s: open", dbpath);
	(void)dbp->close(dbp, 0);
	return FAIL;
    }

    return 0;
}

/*  closing the DenotMap Database */
static int
ooDenotMap_close(ooDenotMap *self)
{
    int ret;
    if (self->_storage != NULL)
	ret = self->_storage->close(self->_storage, 0);
    if (ret != 0) {
	fprintf(stderr, "DenotMap database close failed: %s\n",
		db_strerror(ret));
	return FAIL; }

    self->_storage = NULL;

    return oo_OK;
}

/**
 *  Retrieving results:
 *  if the entry is already cached in memory,
 *  return a reference to it,
 *  otherwise check the DenotMap DB
 *  and initialize a new concept.
 */
static int 
ooDenotMap_get(struct ooDenotMap *self, struct ooComplex *complex)
{
    DB *dbp;
    DBT key, data;
    int ret;


    return oo_OK;
}

/* import Concepts from XML file */
static int
ooDenotMap_import(struct ooDenotMap *self, const char *filename)
{
    int ret, errcode;
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;

    if(DEBUG_LEVEL_1)
	printf(" -- DenotMap: reading XML file \"%s\"...\n", filename);

    doc = xmlParseFile(filename);
    if (doc == NULL) {
	fprintf(stderr,"Document not parsed successfully :( \n");
	errcode = -1;
	goto error;
    }

    root = xmlDocGetRootElement(doc);
	
    if (root == NULL) {
	fprintf(stderr,"empty document\n");
	xmlFreeDoc(doc);
	errcode = -2;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "conceptlist")) {
	fprintf(stderr,"Document of the wrong type, root node != conceptlist");
	errcode = -3;
	goto error;
    }

    /* read the children of "conceptlist" */
    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;


        /* TODO: read the concepts per se */

    }

    errcode = oo_OK;

error:
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return errcode;
}

/* export concepts to file */
static int
ooDenotMap_export(struct ooDenotMap *self, 
		 const char *filename)
{
    printf("Exporting concepts to file %s...\n", filename);

    return oo_OK;
}


/*  DenotMap Initializer */
extern int
ooDenotMap_init(struct ooDenotMap **mm)
{
    DB *dbp;
    int ret;
    struct ooDenotMap *self = malloc(sizeof(struct ooDenotMap));

    /* Create and initialize database object */
    if ((ret = db_create(&dbp, NULL, 0)) != 0) {
	fprintf(stderr,
		"db_create: %s\n", db_strerror(ret));
	return FAIL;
    }
    dbp->set_errfile(dbp, stderr);
    dbp->set_errpfx(dbp, "DenotMap");

    if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
	dbp->err(dbp, ret, "set_pagesize");
	goto error;
    }
    if ((ret = dbp->set_cachesize(dbp, 0, 32 * 1024, 0)) != 0) {
	dbp->err(dbp, ret, "set_cachesize");
	goto error;
    }

    /* saving the DB reference */
    self->_storage = dbp;

    /* initialize the cache */
    self->num_cache_items = STORAGE_CACHE_SIZE;
    self->_cache_storage = malloc(sizeof(struct ooConcept) * self->num_cache_items);

    /* binding our methods */
    self->del = ooDenotMap_del;
    self->str = ooDenotMap_str;

    self->open = ooDenotMap_open;
    self->close = ooDenotMap_close;

    *mm = self;
    return oo_OK;

error:	(void)dbp->close(dbp, 0);
	return FAIL;
}
