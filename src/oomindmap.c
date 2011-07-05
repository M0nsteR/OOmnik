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
 *   oomindmap.c
 *   OOmnik MindMap implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <db.h>
#include <libxml/parser.h>

#include "ooconfig.h"
#include "ooconcept.h"
#include "oomindmap.h"


/*  MindMap Destructor */
static int
ooMindMap_del(struct ooMindMap *self)
{
    int i, ret;

    if (self->_storage)
	ret = self->_storage->close(self->_storage, 0);

    /* remove codesystems */
    for (i = 0; i < self->num_codesystems; i++)
	if (self->codesystems[i]) 
	    self->codesystems[i]->del(self->codesystems[i]);

    if (self->codesystems)
	free(self->codesystems);

    if (self->concept_index)
	free(self->concept_index);

    if (self->_name_index) {
	self->_name_index->del(self->_name_index);
	free(self->_name_index);
    }

    for (i = 0; i < self->num_topics; i++)
	if (self->topics[i]) self->topics[i]->del(self->topics[i]);

    free(self->topics);

    free(self);

    return oo_OK;
}

/*  string representation */
static const char*
ooMindMap_str(struct ooMindMap *self)
{
    struct ooTopicIngredient *ingr;
    int i;

    printf("\n\n MindMap  num_concepts: %d\n", 
	   self->num_concepts);

    for (i = 0; i < self->num_concepts; i++) {
	ingr = self->topic_index[i];
	if (!ingr) continue;
	printf("%d) ingredient: %p\n", i, ingr);
    }
    

    return NULL;
}

/*  opening the MindMap Database */
static int
ooMindMap_open(struct ooMindMap *self, const char *dbpath)
{
    int ret;
    DB *dbp = self->_storage;

    if (!dbp) return oo_FAIL;

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
	return oo_FAIL;
    }

    return 0;
}

/*  closing the MindMap Database */
static int
ooMindMap_close(struct ooMindMap *self)
{
    int ret = -1;

    if (self->_storage)
	ret = self->_storage->close(self->_storage, 0);

    if (ret != oo_OK) {
	fprintf(stderr, "MindMap database close failed: %s\n",
		db_strerror(ret));
	return oo_FAIL; 
    }

    self->_storage = NULL;

    return oo_OK;
}

/*  generate a new unique id */
static mindmap_size_t
ooMindMap_newid(struct ooMindMap *self)
{
    static mindmap_size_t n = 0;
    /* n = (mindmap_size_t)rand() % 1000000; */
    return ++n;
}



/*  adding a concept to the MindMap DB */
static int
ooMindMap_put(struct ooMindMap *self, struct ooConcept *conc)
{
    return conc->put(conc, self->_storage);
}

/*  removing a concept from the MindMap DB */
static int
ooMindMap_drop(struct ooMindMap *self, mindmap_size_t concid)
{
    DBT key, data;
    DB *dbp;
    size_t len;
    int ret;
    dbp = self->_storage;

    /* TODO */

    return 0;
}


/**
 *  Retrieving a concept:
 *  if it's already cached in memory,
 *  return a reference to it,
 *  otherwise check the MindMap DB
 *  and initialize a new concept.
 */
static struct ooConcept* 
ooMindMap_get(struct ooMindMap *self, mindmap_size_t id)
{
    DB *dbp;
    DBT key, data;
    int ret;

    struct ooConcept *conc = NULL;
    char *key_buffer;
    static mindmap_size_t idsize = sizeof(mindmap_size_t);

    if (id > self->num_concepts) return NULL;

    /* check the memory cache */
    conc = (struct ooConcept*)self->concept_index[id];
    if (conc) return conc;

    dbp = self->_storage;

    /* initialize the DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key_buffer = (char*)malloc(idsize);
    if (!key_buffer) return oo_NOMEM;

    memcpy(key_buffer, &id, idsize);

    /* set the search key with the Concept id */
    key.data = key_buffer;
    key.size = idsize;

    /* get the record */
    ret = dbp->get(dbp, NULL, &key, &data, 0);
    if (ret != 0) {
        dbp->err(dbp, ret,
                 "Error searching for id: %ld", id);
        return NULL;
    }

    /* create concept */
    ret = ooConcept_init(&conc);
    if (ret != oo_OK) return NULL;

    conc->id = id;
    conc->bytecode = (char*)data.data;
    conc->bytecode_size = (size_t)data.size;
    conc->unpack(conc);

    /* update the cache */
    ret = self->concept_index[id] = conc;

    free(key_buffer);

    return conc;
}

static int
ooMindMap_keys(struct ooMindMap *self,
		    mindmap_size_t **ids,
		    mindmap_size_t   batch_size,
		    mindmap_size_t   batch_start)
{   DBT key, data;
    DB *dbp;
    DBC *dbcp;
    size_t len;
    int ret;

    dbp = self->_storage;

    /* initialize the key/data pair */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* acquire a cursor for the database */
    if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
	dbp->err(dbp, ret, "DB->cursor");
	goto error;
    }

    /* TODO: batching */

    /* walk through the database and print out the key/data pairs. */
    while ((ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT)) == 0)
	printf("%.*s : %.*s\n",
	       (int)key.size, (char *)key.data,
	       (int)data.size, (char *)data.data);
    if (ret != DB_NOTFOUND)
    {	dbp->err(dbp, ret, "DBcursor->get");
	goto error;
    }

    /* close everything down */
    if ((ret = dbcp->c_close(dbcp)) != 0) {
	dbp->err(dbp, ret, "DBcursor->close");
	goto error;
    }

    return oo_OK;

error:	
    (void)dbp->close(dbp, 0);
    return oo_FAIL;
}

/*  looking up a concept by its atomic name */
static struct ooConcept* 
ooMindMap_lookup(struct ooMindMap *self, 
		 const char *name)
{
    /*printf("  ** MindMap: looking up \"%s\"...\n", name); */

    return (struct ooConcept *)self->_name_index->get(self->_name_index, name);
}

static struct ooCodeSystem* 
ooMindMap_get_codesystem(struct ooMindMap *self, 
			 const char *cs_uri)
{
    /*if (DEBUG_LEVEL_2)
      printf("  ** MindMap: looking up Code System \"%s\"...\n", cs_uri);*/
    
    return (struct ooCodeSystem*)self->_name_index->get(self->_name_index, 
                                                      cs_uri);
}
/*  add a new concept */
static int
ooMindMap_add_concept(struct ooMindMap *self, 
		      xmlNode *input_node,
		      const char *filename)
{  
    struct ooConcept *c;
    struct ooConcept **index;
    int i, ret;

    ret = ooConcept_init(&c);
    if (ret != oo_OK) return ret;

    c->read(c, input_node);


    /* inform the name index */
    if (self->_name_index) 
	ret = self->_name_index->set(self->_name_index, 
				     c->name, c);

    /* update the index */
    if ((self->num_concepts + 1) > self->concept_index_size) {
	index = realloc(self->concept_index, 
			sizeof(struct ooConcept*) * 
			self->concept_index_size * 
			INDEX_REALLOC_FACTOR);
	if (!index) return oo_NOMEM;
	self->concept_index = index;
	self->concept_index_size = self->concept_index_size * 
	    INDEX_REALLOC_FACTOR;

	for (i = self->num_concepts; i < self->concept_index_size; i++)
	    self->concept_index[i] = NULL;

    }

    c->id = self->num_concepts;
    self->concept_index[self->num_concepts++] = c;
    return oo_OK;
}


static int
ooMindMap_add_codesystem(struct ooMindMap *self, 
			 xmlNode *xmlcur,
			 const char *filename)
{
    size_t cs_size;
    struct ooCodeSystem ** csp;
    int ret = oo_FAIL;

    struct ooCodeSystem *cs = NULL;
    ret = ooCodeSystem_new(&cs);
    if (ret != oo_OK) return oo_FAIL;

    cs->mindmap = self;
    cs->filename = filename;

    if (cs->read(cs, xmlcur) != oo_OK) {
	fprintf(stderr,
		"Failed to initialize the CodeSystem :(\n");
	return oo_FAIL;
    }

    cs_size = (self->num_codesystems + 1) * sizeof(struct ooCodeSystem*);
    csp = realloc(self->codesystems, cs_size);
    if (!csp) return oo_NOMEM;

    csp[self->num_codesystems] = cs;
    self->codesystems = csp;
    self->num_codesystems++;

    cs->id = self->num_codesystems;

    /* update the name index */
    if (self->_name_index && cs->name)
	ret = self->_name_index->set(self->_name_index, cs->name, cs);

    return oo_OK;
}


static int
ooMindMap_add_topic(struct ooMindMap *self, 
		    xmlNode *xmlcur,
		    const char *filename)
{
    size_t topics_size;
    struct ooTopic **topics;
    struct ooTopic *topic;
    int ret;

    ret = ooTopic_new(&topic);
    if (ret != oo_OK) return oo_FAIL;

    if (topic->read(topic, xmlcur) != oo_OK) {
	fprintf(stderr,
		"Failed to read the topic in %s :(\n", filename);
	return oo_FAIL;
    }

    topics_size = (self->num_topics + 1) * sizeof(struct ooTopic*);
    topics = realloc(self->topics, topics_size);
    if (!topics) return oo_NOMEM;

    topic->id = self->num_topics;

    topics[self->num_topics] = topic;
    self->topics = topics;

    self->num_topics++;

    return oo_OK;
}


/* import Concepts from XML file */
static int
ooMindMap_import(struct ooMindMap *self, 
		 const char *filename)
{
    int ret, errcode;
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;

    if (DEBUG_LEVEL_1)
	printf(" -- MindMap: reading XML file \"%s\"...\n", filename);

    doc = xmlParseFile(filename);
    if (!doc) {
	fprintf(stderr,"Document not parsed successfully :( \n");
	errcode = -1;
	goto error;
    }

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	errcode = -2;
	goto error;
    }

    /* concepts and codes */
    if (!xmlStrcmp(root->name, (const xmlChar *) "conceptlist")) {
	for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
	    if (cur_node->type != XML_ELEMENT_NODE) continue;

	    /* read the codesystem */
	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"codesystem"))) 
		ooMindMap_add_codesystem(self, cur_node, filename);

	    /* read the concept */
	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"concept"))) 
		ooMindMap_add_concept(self, cur_node, filename);
	    
	}
    }  /*  topics */
    else if (!xmlStrcmp(root->name, (const xmlChar *) "topics")) {
	for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
	    if (cur_node->type != XML_ELEMENT_NODE) continue;
	    /* read the topics */
	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"topic"))) 
		ooMindMap_add_topic(self, cur_node, filename);
	}
    }
    else {
	fprintf(stderr, "Document of the wrong type, the root node is not \"conceptlist\"\n");
	errcode = -3;
	goto error;
    }

    errcode = oo_OK;

error:
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return errcode;
}


static int
ooMindMap_resolve_refs(struct ooMindMap *self)
{
    struct ooCodeSystem *cs, *provider;
    struct ooTopic *topic;
    int ret;
    char *provider_name;
    size_t i, j;

    /* codesystems */
    for (i = 0; i < self->num_codesystems; i++) {
	cs = self->codesystems[i];
	if (!cs) continue;

	/* each CodeSystem must resolve its references */
	cs->resolve_refs(cs);

	cs->providers = malloc(sizeof(struct ooCodeSystem*) *
			       cs->num_providers);
	if (!cs->providers) return oo_NOMEM;

	for (j = 0; j < cs->num_providers; j++) {
	    provider_name = cs->provider_names[j];
	    provider = ooMindMap_get_codesystem(self, provider_name);

	    if (!provider) {
		printf("No such CodeSystem: %s :((\n", provider_name);
		return oo_FAIL;
	    }

	    /* coordinate the codesets:
	     * use direct references to memory instead of string names */
	    ret = cs->coordinate_codesets(cs, provider);

	    if (DEBUG_LEVEL_1)
		printf("Register %s at %p...\n", provider->name, provider);

	    cs->providers[j] = provider;
	}

    }


   /* prepare topic index */
    self->topic_index = malloc(sizeof(struct ooTopicIngredient*) * 
			       self->num_concepts);
    if (!self->topic_index) return oo_NOMEM;

    for (i = 0; i < self->num_concepts; i++) 
	self->topic_index[i] = NULL;

    for (i = 0; i < self->num_topics; i++) {
	topic = self->topics[i];
	if (!topic) continue;
	topic->resolve_refs(topic, self);
    }

    return oo_OK;
}

/* prepare cache matrix */
static int
ooMindMap_build_cache(struct ooMindMap *self)
{
    struct ooCodeSystem *cs, *provider;
    int ret;
    char *provider_name;
    size_t i, j;

    for (i = 0; i < self->num_codesystems; i++) {
	cs = self->codesystems[i];
	if (!cs) continue;
	ret = cs->build_cache(cs);
    }

    return oo_OK;
}



/* export concepts to file */
static int
ooMindMap_export(struct ooMindMap *self, 
		 const char *filename)
{
    printf("Exporting concepts to file %s...\n", filename);
    return oo_OK;
}



/*  MindMap Initializer */
extern int
ooMindMap_new(struct ooMindMap **mm)
{
    DB *dbp;
    int ret = oo_FAIL;
    int i;

    struct ooMindMap *self = malloc(sizeof(struct ooMindMap));
    if (!self) return oo_NOMEM;

    self->_currid = 0;
    self->num_codesystems = 0;
    self->codesystems = NULL;
    self->_storage = NULL;



    /* Create and initialize database object */
    if ((ret = db_create(&dbp, NULL, 0)) != 0) {
	fprintf(stderr,
		"db_create: %s\n", db_strerror(ret));
	goto error;
    }

    self->_storage = dbp;

    dbp->set_errfile(dbp, stderr);
    dbp->set_errpfx(dbp, "MindMap");

    if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
	dbp->err(dbp, ret, "set_pagesize");
	goto error;
    }
    if ((ret = dbp->set_cachesize(dbp, 0, 32 * 1024, 0)) != 0) {
	dbp->err(dbp, ret, "set_cachesize");
	goto error;
    }


     self->_name_index = NULL;

    self->concept_index = malloc(sizeof(struct ooConcept*) * 
				 DEFAULT_INDEX_SIZE);
    if (!self->concept_index) {
	ret = oo_NOMEM;
	goto error;
    }

    for (i = 0; i < DEFAULT_INDEX_SIZE; i++)
	self->concept_index[i] = NULL;

    /* first concept [0] has a special meaning: "Unrecognized" */
    self->num_concepts = 1;
    self->concept_index_size = DEFAULT_INDEX_SIZE; 

    ret = ooDict_new(&self->_name_index);
    if (ret != oo_OK) goto error;

    /* 0 has a special value of "Any Other Topic" */
    self->topics = malloc(sizeof(struct ooTopic*));
    self->topics[0] = NULL;
    self->num_topics = 1;

    /* binding our methods */
    self->del = ooMindMap_del;
    self->str = ooMindMap_str;

    self->open = ooMindMap_open;
    self->close = ooMindMap_close;

    self->import_file = ooMindMap_import;
    self->export_file = ooMindMap_export;

    self->newid = ooMindMap_newid;

    self->put = ooMindMap_put;
    self->drop = ooMindMap_drop;

    self->get = ooMindMap_get;
    self->get_codesystem = ooMindMap_get_codesystem;
    self->resolve_refs = ooMindMap_resolve_refs;
    self->build_cache = ooMindMap_build_cache;
    self->lookup = ooMindMap_lookup;
    self->keys = ooMindMap_keys;



    *mm = self;
    return oo_OK;

error:
    ooMindMap_del(self);
    return ret;
}
