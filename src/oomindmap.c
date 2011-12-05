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

#include "oocodesystem.h"
#include "oomindmap.h"
#include "ootopic.h"
#include "ooconcept.h"
#include "oodomain.h"
#include "ooutils.h"
#include "oodict.h"

#include "ooconfig.h"

/* forward */
static int 
ooMindMap_import(struct ooMindMap *self, 
		 const char *filename, 
		 struct ooDomain *parent_domain);

static int 
ooMindMap_read_domain(struct ooMindMap *self, 
		      xmlNode *input_node, 
		      const char *path,
		      size_t path_size,
		      struct ooDomain *parent);



/*  MindMap Destructor */
static int
ooMindMap_del(struct ooMindMap *self)
{
    struct ooConcept *c;
    int i, ret;

    if (self->_storage)
	ret = self->_storage->close(self->_storage, 0);

    /* remove codesystems */
    for (i = 0; i < self->num_codesystems; i++)
	if (self->codesystems[i]) 
	    self->codesystems[i]->del(self->codesystems[i]);

    if (self->codesystems)
	free(self->codesystems);

    if (self->concept_index) {
	for (i = 0; i < self->num_concepts; i++) {
	    c = self->concept_index[i];
	    if (!c) continue;
	    c->del(c);
	}
	free(self->concept_index);
    }

    if (self->_name_index)
	self->_name_index->del(self->_name_index);

    /* domains */
    if (self->num_domains) {
	for (i = 0; i < self->num_domains; i++)
	    if (self->domains[i]) self->domains[i]->del(self->domains[i]);
	free(self->domains);
    }


    /* topics */
    free(self->topic_index);
    if (self->topics) {
	for (i = 0; i < self->num_topics; i++)
	    if (self->topics[i]) self->topics[i]->del(self->topics[i]);
	free(self->topics);
    }

    free(self);

    return oo_OK;
}

/*  string representation */
static const char*
ooMindMap_str(struct ooMindMap *self)
{
    struct ooTopicIngredient *ingr;
    struct ooDomain *domain;
    int i;

    printf("\n\n MindMap  num_concepts: %d\n", 
	   self->num_concepts);

    for (i = 0; i < self->num_concepts; i++) {
	ingr = self->topic_index[i];
	if (!ingr) continue;
	printf("%d) ingredient: %p\n", i, ingr);
    }
    
    for (i = 0; i < self->num_domains; i++) {
	domain = self->domains[i];
	if (!domain) continue;
	domain->str(domain);
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

    key_buffer = malloc(idsize);
    if (!key_buffer) return NULL;

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
    ret = ooConcept_new(&conc);
    if (ret != oo_OK) return NULL;

    conc->numid = id;
    conc->bytecode = (char*)data.data;
    conc->bytecode_size = (size_t)data.size;
    conc->unpack(conc);

    /* update the cache */
    self->concept_index[id] = conc;

    free(key_buffer);

    return conc;
}

static int
ooMindMap_keys(struct ooMindMap *self,
	       mindmap_size_t  **ids,
	       mindmap_size_t    batch_size,
	       mindmap_size_t    batch_start)
{   DBT key, data;
    DB *dbp;
    DBC *dbcp;
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

/*  read XML source and create a new concept */
static int
ooMindMap_read_concept(struct ooMindMap *self, 
		      xmlNode *input_node,
		      struct ooDomain *domain)
{
    struct ooConcept *c;
    int ret;

    ret = ooConcept_new(&c);
    if (ret != oo_OK) return ret;

    ret = c->read(c, input_node, self, domain);
    if (ret != oo_OK) {
	c->del(c);
	return ret;
    }

    return oo_OK;
}

static int
ooMindMap_add_concept(struct ooMindMap *self, 
		      struct ooConcept *c)
{
    struct ooConcept **index;
    struct ooConcept **concepts;
    size_t concepts_size;
    char concid[MAX_CONC_ID_SIZE];
    size_t offset = 0;

    int i, ret;

    if (!self->_name_index) return oo_FAIL;

    /* inform the name index */

    /* prepare the unique name:
     * concept name + domain id 
     */
    if (c->name_size < MAX_CONC_ID_SIZE) {
	offset += c->name_size;
	strncpy(concid,
		(const char*)c->name, c->name_size);
    }
    if (offset + 1 < MAX_CONC_ID_SIZE) {
	strncpy(concid + offset, ":", 1);
	offset++;
    }
    if (offset + c->id_size < MAX_CONC_ID_SIZE) {
	strncpy(concid + offset, 
		(const char*)c->id, c->id_size);
	offset += c->id_size;
    }
    concid[offset] = '\0';

    /*printf("CONC ID: %s (%d)\n", concid, offset);*/
 
    if (offset) {
	ret = self->_name_index->set(self->_name_index, 
				     (const char*)concid, (void*)c);
    }

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

    c->numid = self->num_concepts;
    self->concept_index[self->num_concepts] = c;
    self->num_concepts++;

    return oo_OK;
}

static int
ooMindMap_read_codesystem(struct ooMindMap *self, 
			 xmlNode *xmlcur,
			  const char *path,
			  size_t path_size)
{
    struct ooCodeSystem **csp;
    size_t cs_size;
    struct ooCodeSystem *cs = NULL;
    int ret;

    ret = ooCodeSystem_new(&cs);
    if (ret != oo_OK) return oo_FAIL;

    cs->mindmap = self;
    cs->filename = path;

    ret = cs->read(cs, xmlcur);
    if (ret != oo_OK) {
	fprintf(stderr,
		"Failed to initialize the CodeSystem :(\n");
	return ret;
    }

    cs_size = (self->num_codesystems + 1) * 
	sizeof(struct ooCodeSystem*);
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
		    const char *path,
		    size_t path_size)
{
    size_t topics_size;
    struct ooTopic **topics;
    struct ooTopic *topic;
    int ret;

    ret = ooTopic_new(&topic);
    if (ret != oo_OK) return oo_FAIL;

    if (topic->read(topic, xmlcur) != oo_OK) {
	fprintf(stderr,
		"Failed to read the topic in %s :(\n", path);
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


static int
ooMindMap_read_subdomains(struct ooMindMap *self, 
			  xmlNode *input_node,
			  const char *parent_path,
			  size_t parent_path_size,
			  struct ooDomain *parent)
{
    xmlDocPtr doc;
    xmlNodePtr root_node;
    xmlNodePtr cur_node;
    char *filename = NULL;
    size_t filename_size;
    char *path = NULL;
    size_t path_size;
    int ret;

    printf("Reading subdomains of \"%s\"...\n", parent_path);

    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	    /* nested includes */
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) {

	    ret = oo_copy_xmlattr(cur_node, "filename", &filename, &filename_size);
	    if (ret != oo_OK) return ret;

	    path_size = parent_path_size + filename_size;
	    path = malloc(path_size + 1);
	    if (!path) {
		free(filename);
		return oo_NOMEM;
	    }

	    strncpy(path, parent_path, parent_path_size);
	    strncpy(path +  parent_path_size, filename, filename_size);
	    path[path_size] = '\0';

	    ret = ooMindMap_import(self, (const char*)path, parent);

	    free(path);
	    free(filename);
	    filename = NULL;
	}

    }
    

    return oo_OK;
}


static int
ooMindMap_read_domain(struct ooMindMap *self, 
		      xmlNode *input_node,
		      const char *parent_path,
		      size_t parent_path_size,
		      struct ooDomain *parent)
{
    xmlNodePtr cur_node;
    struct ooDomain *domain;
    size_t domains_size;
    struct ooDomain **domains;
    int ret;

    ret = ooDomain_new(&domain);
    if (ret != oo_OK) return ret;

    domain->parent = parent;

    if (!parent)
	domain->depth = 1;
    else
	domain->depth = parent->depth + 1;

    ret = oo_copy_xmlattr(input_node, "name", &domain->name, &domain->name_size);
    if (ret != oo_OK) return ret;

    ret = oo_copy_xmlattr(input_node, "id", &domain->id, &domain->id_size);
    if (ret != oo_OK) return ret;


    if (DEBUG_LEVEL_2) {
	printf(" -- MindMap: DOMAIN \"%s\"...\n", domain->name);
    }

    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"title"))) {
	    ret = oo_copy_xmlattr(cur_node, "text", &domain->title, &domain->title_size);
	    if (ret == oo_OK && DEBUG_LEVEL_2)
		printf(" -- MindMap: domain's title: \"%s\"\n", domain->title);
	    continue;
	}
	
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"concept"))) {
	    ooMindMap_read_concept(self, cur_node, domain);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"subdomains")))
	    ooMindMap_read_subdomains(self, cur_node, 
				      parent_path, parent_path_size, domain);
    }
 

    /* root domain */
    if (!parent)
	self->root_domain = domain;
    else {
	/* registration within parent domain */
	/*printf("PARENT %s accepts child %s\n",
	  parent->title, domain->title);*/

	domains_size = (parent->num_subdomains + 1) * sizeof(struct ooDomain*);
	domains = realloc(parent->subdomains, domains_size);
	if (!domains) return oo_NOMEM;
	domains[parent->num_subdomains] = domain;
	parent->subdomains = domains;
	parent->num_subdomains++;
    }

    /* global registration */
    domains_size = (self->num_domains + 1) * sizeof(struct ooDomain*);
    domains = realloc(self->domains, domains_size);
    if (!domains) return oo_NOMEM;

    domain->numid = self->num_domains;

    domains[self->num_domains] = domain;
    self->domains = domains;
    self->num_domains++;
    
    return oo_OK;
}

static int
ooMindMap_read_include(struct ooMindMap *self, 
		       xmlNode *input_node,
		       const char *parent_path,
		       size_t parent_path_size)
{
    char *child_filename;
    char *path;
    size_t child_filename_size;
    size_t path_size;
    int ret;

    ret = oo_copy_xmlattr(input_node, "filename", &child_filename, &child_filename_size);
    if (ret != oo_OK) return ret; 

    path_size = parent_path_size + child_filename_size + 1;
    path = malloc(path_size);
    if (!path) return oo_NOMEM;

    strcpy(path, parent_path);
    strcpy(path + parent_path_size, child_filename);
    path[path_size] = '\0';

    if (DEBUG_LEVEL_2)
	fprintf(stderr, " .. reading include: %s\r", path);

    ret = ooMindMap_import(self, (const char*)path, NULL);

    free(child_filename);
    free(path);

    return ret;
}





/**
 * import Concepts from XML file 
 */
static int
ooMindMap_import(struct ooMindMap *self, 
		 const char *filename,
		 struct ooDomain *parent_domain)
{
    xmlDocPtr doc;
    xmlNodePtr root_node, cur_node;
    char *path = NULL;
    size_t path_size;
    size_t chunk_size;
    int ret = oo_OK;

    if (DEBUG_LEVEL_1)
	printf(" -- MindMap: reading XML file \"%s\"...\n", filename);

    doc = xmlParseFile(filename);
    if (!doc) {
	fprintf(stderr,"Document not parsed successfully :( \n");
	ret = -1;
	goto final;
    }

    root_node = xmlDocGetRootElement(doc);
    if (!root_node) {
	fprintf(stderr,"empty document\n");
	ret = -2;
	goto final;
    }

    ret = oo_extract_dirpath(filename, &path, &path_size);
    if (ret != oo_OK) goto final;

    if (DEBUG_LEVEL_1)
	printf(" -- MindMap: successfully parsed XML file in \"%s\"\n", path);

   /* concepts and codes */
    if (!xmlStrcmp(root_node->name, (const xmlChar *) "conceptlist")) {
	for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
	    if (cur_node->type != XML_ELEMENT_NODE) continue;

	    /* nested includes */
	    /*if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) 
	      ooMindMap_read_include(self, cur_node, path, path_size);*/

	    /* read the codesystem */
	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"codesystem"))) 
		ooMindMap_read_codesystem(self, cur_node, path, path_size);
	}
	goto final;
    } 

    /* read conceptual domains */
    if (!xmlStrcmp(root_node->name, (const xmlChar *)"domain")) {
	ooMindMap_read_domain(self, root_node, path, path_size, parent_domain);
	goto final;
    }

    /*  topics */
    if (!xmlStrcmp(root_node->name, (const xmlChar *) "topics")) {
	for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
	    if (cur_node->type != XML_ELEMENT_NODE) continue;
	    /* read the topics */
	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"topic")))
		ooMindMap_add_topic(self, cur_node, path, path_size);
	}
	goto final;
    }

    fprintf(stderr,
	    "The root element \"%s\" is not supported :(\n",
	    root_node->name);
    ret = -3;

final:

    if (path)
	free(path);

    if (doc)
	xmlFreeDoc(doc);

    return ret;
}


static int
ooMindMap_resolve_refs(struct ooMindMap *self)
{
    struct ooCodeSystem *cs, *provider;
    struct ooTopic *topic;
    struct ooConcept *conc;
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
		printf("Register %s at %p...\n", provider->name, (void*)provider);

	    cs->providers[j] = provider;
	}

    }


   /* prepare topic index */
    self->topic_index = malloc(sizeof(struct ooTopicIngredient*) * 
			       self->num_concepts);
    if (!self->topic_index) return oo_NOMEM;

    for (i = 0; i < self->num_concepts; i++) {
	self->topic_index[i] = NULL;
	conc = self->concept_index[i];
	if (!conc) continue;
    }

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


    self->root_domain = NULL;
    self->domains = NULL;
    self->num_domains = 0;

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

    self->add_concept = ooMindMap_add_concept;


    *mm = self;
    return oo_OK;

error:
    ooMindMap_del(self);
    return ret;
}
