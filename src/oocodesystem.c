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
 *   --------------
 *   oocodesystem.h
 *   OOmnik Coding System implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>

#include "ooconfig.h"
#include "ooconcept.h"
#include "ooconstraint.h"
#include "oocode.h"
#include "oocodesystem.h"
#include "oosegmentizer.h"
#include "oocache.h"
#include "oodecoder.h"

static const char *ooCodeSystem_operids[] =\
{ "NONE", "IS_SUBCLASS", "AGGREGATES", "HAS_ATTR", "TAKES_ARG", 
  "DENOTES", "RUNS", "PRECEEDS", "NEXT_IN_GROUP",
  "START_REVERSE_OPERS",
  "SUBCLASSED_BY", "CONSTITUTES", "IS_ATTR_OF", "IS_ARG", 
  "DENOTED_BY", "IS_RUN", "FOLLOWS", "PREV_IN_GROUP", NULL };

/* forward declarations */
static int
ooCodeSystem_read_codeset(struct ooCodeSystem *self, xmlNode *input_node);

/*  destructor */
static
int ooCodeSystem_del(struct ooCodeSystem *self)
{
    int i, j;
    struct ooConstraintType *ct;
    struct ooCode *code;
    char *value;

    /* providers */
    for (i = 0; i < self->num_providers; i++)
	if (self->provider_names[i])
	    free(self->provider_names[i]);
    if (self->provider_names)
	free(self->provider_names);
    if (self->providers)
	free(self->providers);

    /* inheritors */
    for (i = 0; i < self->num_inheritors; i++)
	if (self->inheritor_names[i])
	    free(self->inheritor_names[i]);
    if (self->inheritor_names)
	free(self->inheritor_names);
    if (self->inheritors)
	free(self->inheritors);

    /* constraint names and objects */
    for (i = 0; i < self->num_constraints; i++) {
	ct = self->constraints[i];
	if (!ct) continue;
	if (ct->name)
	    free(ct->name);
	for (j = 0; j < ct->num_values; j++) {
	    value = ct->values[j];
	    if (value) free(value);
	}
	if (ct->values) free(ct->values);
	free(ct);
    }

    if (self->constraints) 
	free(self->constraints);

    if (self->cache)
	self->cache->del(self->cache);

    if (self->numeric_denotmap)
	free(self->numeric_denotmap);

    /*if (self->denotmap)
	self->numeric_denotmap->del(self->denotmap);
    */

    for (i = 0; i < self->num_codes; i++)
	if (self->code_index[i]) self->code_index[i]->del(self->code_index[i]);
   
    if (self->code_names) free(self->code_names);
    if (self->code_index) free(self->code_index);

    if (self->codes) {
	self->codes->del(self->codes);
	free(self->codes);
    }
    if (self->root_elem_name) free(self->root_elem_name);

    /* say bye to your unique beautiful name :(( */
    if (self->name) free(self->name);

    /* free up yourself */
    free(self);

    return oo_OK;
}



/**
 *  resolving all name references
 *  to the actual memory addresses
 */
static int
ooCodeSystem_resolve_refs(struct ooCodeSystem *self)
{
    int i;
    struct ooCode *code;

    if (DEBUG_CS_LEVEL_2)
	printf("  Setting inner cross-references...\n"); 

    for (i = 1; i < self->num_codes; i++) {
	code = self->code_index[i];
	code->resolve_refs(code);
    }

    return oo_OK;
}

/** set the cross-references 
 *  between a CS and its provider CS 
 */
static int
ooCodeSystem_coordinate_codesets(struct ooCodeSystem *self, 
                                   struct ooCodeSystem *cs)
{
    size_t code_value, denot_value, i, j, c;
    char *code_name, *denot_name, *implied_name;
    struct ooCode *code, *denot, *implied;
    int ret;

    if (DEBUG_CS_LEVEL_3)
	printf("  Setting coordination between \"%s\" (total codes: %u)\n\
                           and \"%s\" (total codes: %u)\n", 
	       self->name, self->num_codes, cs->name, cs->num_codes);

    /* start iteration from 1 because 0 means "Unrecognized Code" */
    for (i = 1; i < cs->num_codes; i++) {
	code_name = cs->code_names[i];

	code = (struct ooCode*)cs->codes->get(cs->codes, code_name);
	if (!code) continue;

	/* add Numeric_Denotmap -> local id mappings */
	if (cs->use_numeric_codes) {
	    code_value = strtoul(code_name, NULL, 16);
	    if (code_value > UCS2_MAX) continue;
	    
	    denot_value = 0;
	    for (j = 0; j < code->num_denots; j++) {
		denot_name = code->denot_names[j];
		denot = self->codes->get(self->codes, denot_name);
		if (!denot) continue;
		denot_value = denot->id;
	    }

	    if (DEBUG_CS_LEVEL_4)
		printf(" == REF: \"%s\" %s (%u) denotes \"%s\" (local id: %u)\n", 
		       cs->name, code_name, code_value, denot_name, denot_value);

	    cs->numeric_denotmap[code_value] = denot_value;
	    continue;
	}

	for (j = 0; j < code->num_denots; j++) {
	    denot_name = code->denot_names[j];
	    denot = self->codes->get(self->codes, denot_name);
	    if (!denot) { 
		if (DEBUG_CS_LEVEL_2) {
		    printf(" -- no such denot: \"%s\" :(\n", 
		       denot_name);
		}
		continue;
	    }
	    code->denots[j] = denot;
	}

	/* any implied codes? */
	if (code->num_implied_codes) {
	    for (j = 0; j < code->num_implied_codes; j++) {
		implied_name = code->implied_code_names[j];
		implied = cs->codes->get(cs->codes, implied_name);
		if (!implied) continue;
		code->implied_codes[j] = implied;
		/*printf("IMPLIED by %s: %s %p\n", code->name, implied_name, implied);*/
	    }
	}

	if (DEBUG_CS_LEVEL_4) {
	    printf(" ++ CODE READY:\n");
	    code->str(code);
	}
    }

    return oo_OK;
}


/**
 * add a CodeSystem name to the list of known providers 
 */
static int
ooCodeSystem_add_provider(struct ooCodeSystem *self, 
			  const char *cs_uri)
{
    char **names;
    char *name;
    int i, ret;
    size_t nameset_size;

    if (DEBUG_CS_LEVEL_1)
	printf("  ** Adding a provider name \"%s\"\n", cs_uri);

    nameset_size = (self->num_providers + 1) * sizeof(char*);
    names = realloc(self->provider_names, nameset_size);
    if (!names) return oo_NOMEM;

    name = malloc(strlen(cs_uri) + 1);
    if (!name) return oo_NOMEM;

    strcpy(name, cs_uri);
    names[self->num_providers] = name;

    self->provider_names = names;
    self->num_providers++;

    return oo_OK;
}

static int
ooCodeSystem_add_inheritor(struct ooCodeSystem *self, 
			   const char *cs_uri)
{
    char **names;
    char *name;
    int i, ret;
    size_t nameset_size;

    if (DEBUG_CS_LEVEL_1)
	printf("  ** Adding an inheritor name \"%s\"\n", cs_uri);

    nameset_size = (self->num_inheritors + 1) * sizeof(char*);
    names = realloc(self->inheritor_names, nameset_size);
    if (!names) return oo_FAIL;

    name = malloc(strlen(cs_uri) + 1);
    if (!name) return oo_NOMEM;

    strcpy(name, cs_uri);
    names[self->num_inheritors] = name;

    self->inheritor_names = names;
    self->num_inheritors++;

    return oo_OK;
}

static int
ooCodeSystem_read_providers(struct ooCodeSystem *self, 
			    xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    xmlAttr *cur_attr = NULL;
    xmlChar *myattr;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	cur_attr = cur_node->properties;
	while (cur_attr) {
           if ((!xmlStrcmp(cur_attr->name, (const xmlChar *)"name"))) {
	       myattr = xmlGetProp(cur_node,  (const xmlChar *)"name");
	       if (DEBUG_CS_LEVEL_3) printf("  == Unit Type Name: %s\n", myattr);
	       /* register a subordinate unit codesystem */
	       ret = ooCodeSystem_add_provider(self, (const char*)myattr);
	       xmlFree(myattr);
	   }

	   cur_attr = cur_attr->next;
        }
    }

    return oo_OK;
}


static int
ooCodeSystem_read_inheritors(struct ooCodeSystem *self, xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    xmlAttr *cur_attr = NULL;
    xmlChar *myattr;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	cur_attr = cur_node->properties;
	while (cur_attr) {
           if ((!xmlStrcmp(cur_attr->name, (const xmlChar *)"name"))) {
	       myattr = xmlGetProp(cur_node,  (const xmlChar *)"name");
	       if (DEBUG_CS_LEVEL_3) printf("  == Unit Type Name: %s\n", myattr);
	       ret = ooCodeSystem_add_inheritor(self, (const char*)myattr);
	       xmlFree(myattr);
	   }
	   cur_attr = cur_attr->next;
        }
    }

    return oo_OK;
}







static int
ooCodeSystem_read_codestruct(struct ooCodeSystem *self, xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char *value, *code_name, *seq;

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"type");
    if (value) {
	if (strcmp(value, "operational") == oo_OK)
	    self->type = CS_OPERATIONAL;

	if (strcmp(value, "positional") == oo_OK)
	    self->type = CS_POSITIONAL;
	xmlFree(value);
    }

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"root");
    if (value) {
	seq = malloc(strlen(value) + 1);
	if (!seq) {
	    xmlFree(value);
	    return oo_NOMEM;
	}
	strcpy(seq, value);
	self->root_elem_name = seq;
	xmlFree(value);
    }

    /* read the children */
    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"providers"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"logic_oper");
	    if (value) {
		if (!strcmp(value, "OR")) self->providers_logic_oper = LOGIC_OR;
		xmlFree(value);
	    }
	    ooCodeSystem_read_providers(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"inheritors"))) {
	    ooCodeSystem_read_inheritors(self, cur_node->children);
	    continue;
	}

    }
    return oo_OK;
}

static int
ooCodeSystem_read_initcache(struct ooCodeSystem *self, xmlNode *input_node)
{
    char *value;
    char *provider_name;
    size_t matrix_depth = DEFAULT_MATRIX_DEPTH;
    size_t max_unrec_chars = DEFAULT_MAX_UNREC_CHARS;
    bool trust_separators = false;
    int ret;

    if (DEBUG_CS_LEVEL_2)
	printf("  ** Reading the Linear Cache settings...\n");

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"enable");
    if (!value) return oo_FAIL; 
    if (atoi(value) != 1) {
	xmlFree(value);
	return oo_FAIL;
    }
    xmlFree(value);

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"matrix_depth");
    if (value) {
	matrix_depth = atoi(value);
	xmlFree(value);
    }

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"max_unrec_chars");
    if (value) {
	max_unrec_chars = atoi(value);
	xmlFree(value);
    }

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"trust_separators");
    if (value) {
	trust_separators = (bool)atoi(value);
	xmlFree(value);
    }

    provider_name = (char*)xmlGetProp(input_node,  (const xmlChar *)"unittype");
    if (!provider_name) {
	xmlFree(provider_name);
	return oo_FAIL;
    }

    if (DEBUG_CS_LEVEL_3)
	printf("  ** Adding Linear Cache...\n");

    ret = ooLinearCache_new(&self->cache);
    if (ret != oo_OK) {
	xmlFree(provider_name);
	return oo_NOMEM;
    }
    self->cache->provider_name = malloc(strlen(provider_name) + 1);
    if (!self->cache->provider_name) {
	self->cache->del(self->cache);
	xmlFree(provider_name);
	return oo_NOMEM;
    }
    strcpy(self->cache->provider_name, provider_name);
    xmlFree(provider_name);

    self->cache->trust_separators = trust_separators;
    self->cache->matrix_depth = matrix_depth;
    self->cache->max_unrec_chars = max_unrec_chars;
    self->cache->cs = self;

    return oo_OK;
}


static int
ooCodeSystem_build_cache(struct ooCodeSystem *self)
{
    struct ooCodeSystem *provider;
    int ret;
    if (!self->cache) return oo_FAIL;

    provider = self->mindmap->get_codesystem(self->mindmap,
					   (const char*)self->cache->provider_name);
    if (!provider) return oo_FAIL;

    self->cache->provider = provider;

    ret = self->cache->build_matrix(self->cache);
    ret = self->cache->populate_matrix(self->cache);

    return oo_OK;
}



/* reading XML description of a new code */
static int
ooCodeSystem_add_new_code(struct ooCodeSystem *self, 
			  xmlNode *cur_node)
{
    struct ooCode *code, *denot_code;
    char *value;
    size_t nameset_size;
    char **names;
    int ret;

    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
    if (!value) {
	fprintf(stderr, "  -- No name given for the code %d"
		" in the element \"%s\"...\n",
		self->num_codes, cur_node->name);
	return oo_FAIL;
    }

    /* check for doublets */
    code = (struct ooCode*)self->codes->get(self->codes, value);
    if (code) {
	fprintf(stderr, "  -- Ignoring the doublet of \"%s\"...\n", value);
	xmlFree(value);
	return oo_FAIL;
    }

    /* create a new code instance */
    ret = ooCode_new(&code);
    if (ret != oo_OK) {
	xmlFree(value);
	return ret;
    }
    code->id = self->num_codes;
    code->cs = self;
    code->name = malloc(strlen(value) + 1);
    if (!code->name) {
	xmlFree(value);
	return oo_NOMEM;
    }

    strcpy(code->name, value);
    xmlFree(value);

    /* code type? */
    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"type");
    if (value) {
	if (!strcmp(value,"Separator")) 
	    code->type = CODE_SEPARATOR;
	if (!strcmp(value,"Grouping Marker")) 
	    code->type = CODE_GROUP_MARKER;
	if (!strcmp(value,"Alternation Marker")) 
	    code->type = CODE_ALTERN_MARKER;
	if (!strcmp(value,"Static Non-Terminal"))
	    code->type = CODE_STATIC_NON_TERMINAL;
	if (!strcmp(value,"Dynamic Non-Terminal"))
	    code->type = CODE_STATIC_NON_TERMINAL;
	if (!strcmp(value,"Prefix Non-Terminal"))
	    code->type = CODE_PREFIX_NON_TERMINAL;
	if (!strcmp(value,"Contact Term Separator"))
	    code->type = CODE_CONTACT_TERM_SEPARATOR;
	if (DEBUG_CS_LEVEL_4) 
	    printf("Code %s has type %d\n", code->name, code->type);
	xmlFree(value);
    }

    /* allows peer grouping? 
       (true by default) */
    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"grouping");
    if (value) {
	if (!atoi(value)) 
	    code->allows_grouping = false;
	xmlFree(value);
    }

    /* add a single denot name */
    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"denot");
    if (value) {
	code->denot_names = malloc(sizeof(char*));
	if (!code->denot_names) {
	    xmlFree(value);
	    return oo_NOMEM;
	}
	code->denots = malloc(sizeof(struct ooCode*));
	if (!code->denots) {
	    free(code->denot_names);
	    xmlFree(value);
	    return oo_NOMEM;
	}

	code->denot_names[0] = malloc(strlen(value) + 1);
	if (!code->denot_names[0]) {
	    free(code->denot_names);
	    free(code->denots);
	    xmlFree(value);
	    return oo_NOMEM;
	}

	strcpy(code->denot_names[0], value);
	code->denots[0] = NULL;
	code->num_denots = 1;
	xmlFree(value);
    }

    /* read XML-description of the code */
    ret = code->read(code, cur_node->children);

    /* register code's name */
    names = realloc(self->code_names, 
		    sizeof(char*) * (self->num_codes + 1));
    if (!names) {
	code->del(code);
	return oo_NOMEM;
    }

    names[self->num_codes] = code->name;
    self->code_names = names;
    self->num_codes++;

    /* add code to cache */
    if (code->is_cached && self->cache) {
	self->cache->set(self->cache, code);
    }

    /* remember this code's address by its name */
    ret = self->codes->set(self->codes,
			   code->name, code);

    /* root element */
    if (self->root_elem_name) {
	if (!strcmp(code->name, self->root_elem_name)) {
	    self->root_elem_id = code->id;
	}
    }


    /* NB */
    fprintf(stderr, "Reading XML entry: %d...\r", self->num_codes);

    /*  update index */
    self->code_index[code->id] = code;

    if (DEBUG_CS_LEVEL_4) {
	printf("\n  REGISTER NEW CODE %d:\n", code->id);
	code->str(code);
    }

    return oo_OK;
}


static int
ooCodeSystem_read_codeset_include(struct ooCodeSystem *self, 
				  xmlNode *input_node,
				  bool need_parsing)
{
    const char *path, *value;
    char *filename;

    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    size_t chunk_size = 0, path_size = 0, filename_size;
    int errcode = 0;
    int ret;

    /* is canonical name different? */
    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"filename");
    if (!value) return oo_FAIL;

    filename_size = strlen(value);

    /* extract the path to directory */
    for (path = self->filename; *path; path++) {
	if ('\\' == *path || '/' == *path) {
	    path_size += chunk_size + 1;
	    chunk_size = 0;
	    continue;
	}
	chunk_size++;
    }

    filename = malloc(path_size + filename_size + 1);
    if (!filename) {
	xmlFree(value);
	return oo_NOMEM;
    }

    strncpy(filename, self->filename, path_size);
    strcpy(filename + path_size, value);
    filename[path_size + filename_size] ='\0';
    xmlFree(value);

    if (DEBUG_CS_LEVEL_1)
	printf("   ... Trying to read the codeset include file \"%s\"\n\n", 
	       filename);

    doc = xmlParseFile(filename);
    if (!doc) {
	fprintf(stderr,"  -- Document \"%s\" not parsed successfully :( \n",
	    filename);
	errcode = -1;
	goto error;
    }

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"  -- Empty document: \"%s\"\n", filename);
	xmlFreeDoc(doc);
	errcode = -2;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "codeset")) {
	fprintf(stderr,"  -- Document \"%s\" is of the wrong type,"
		" the root node is not \"codeset\"",
		filename);
	errcode = -3;
	goto error;
    }

    fprintf(stderr,"  ... include: \"%s\"\n", filename);

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"code"))) {
	    if (!need_parsing) { 
		self->code_index_capacity++;
		continue;
	    }
	    /* read the new code and regiser it in the index */
	    ret = ooCodeSystem_add_new_code(self, cur_node);
	}
    }

    errcode = oo_OK;

 error: 
    xmlFreeDoc(doc);
    xmlCleanupParser();

    free(filename);
    return errcode;
}


static int
ooCodeSystem_read_codeset(struct ooCodeSystem *self, xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char *value, *name;
    size_t code_value, denot_id;
    int ret;

    if (DEBUG_CS_LEVEL_3) 
	printf("  Reading Codes in Codeset %s...\n", self->name);

    /* calculate the expected total number of codes in CS */
    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) {
	    ret = ooCodeSystem_read_codeset_include(self, cur_node, 0);
	    continue;
	}

	self->code_index_capacity++;
    }

    if (!self->code_index_capacity) return oo_FAIL;

    fprintf(stderr,"  == Total number of codes to read: %d\n\n",
	self->code_index_capacity);
    
    self->code_index = malloc(sizeof(struct ooCode*) * (self->code_index_capacity + 1));
    if (!self->code_index) return oo_NOMEM;

    /* 0 slot has a special meaning of an unrecognized value */
    self->code_index[0] = NULL;
    self->num_codes = 1;

    /* start reading the actual code descriptions */
    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	/* index capacity exceeded for some reason? */
	if (self->num_codes > self->code_index_capacity) return oo_FAIL;

	/* read the include file */
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) {
	    ret = ooCodeSystem_read_codeset_include(self, cur_node, 1);
	    continue;
	}

	/* read the new code and register it in the index */
	ret = ooCodeSystem_add_new_code(self, cur_node);
    }


    if (DEBUG_CS_LEVEL_3) 
	printf("\n-- Read %zu codes from Codeset %s...\n",
	       self->num_codes, self->name);

    return oo_OK;
}



static int
ooCodeSystem_read_constraint_type_values(struct ooCodeSystem *self, 
					 xmlNode *input_node,
					 struct ooConstraintType *ct)
{
    char **values, *v, *value = NULL;
    xmlNode *cur_node, *val_node;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;
	if (xmlStrcmp(cur_node->name, (const xmlChar *)"cvalues")) continue;

	for (val_node = cur_node->children; val_node; val_node = val_node->next) {
	    if (val_node->type != XML_ELEMENT_NODE) continue;
	    if (xmlStrcmp(val_node->name, (const xmlChar *)"cvalue")) continue;

	    value = (char*)xmlGetProp(val_node,  (const xmlChar *)"name");
	    if (!value) continue;

	    values = realloc(ct->values, (sizeof(char*) * (ct->num_values + 1)));
	    if (!values) {
		xmlFree(value);
		return oo_NOMEM;
	    }

	    v = malloc(strlen(value) + 1);
	    if (!v) {
		free(values);
		xmlFree(value);
		return oo_NOMEM;
	    }

	    strcpy(v, value);

	    values[ct->num_values] = v;
	    ct->values = values;
	    ct->num_values++;
	    xmlFree(value);
	}
    }


    return oo_OK;
}


static int
ooCodeSystem_read_constraint_types(struct ooCodeSystem *self, 
			      xmlNode *input_node)
{
    char *value = NULL;
    xmlNode *cur_node;
    struct ooConstraintType **constraints, *ct;
    int ret, errcode;

    if (DEBUG_CS_LEVEL_2)
	printf("\n  ** Reading the CONSTRAINTS...\n");

    /* reading the context constraints */
    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;
	
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"constraint"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");

	    constraints = realloc(self->constraints, 
				  (sizeof(struct ooConstraintType*) *
				  (self->num_constraints + 1)));
	    if (!constraints) {
		xmlFree(value);
		errcode = oo_NOMEM;
		goto error;
	    }

	    ct = malloc(sizeof(struct ooConstraintType));
	    if (!ct) {
		free(constraints);
		xmlFree(value);
		errcode = oo_NOMEM;
		goto error;
	    }

	    ct->name = malloc(strlen(value) + 1);
	    if (!ct->name) {
		free(ct);
		free(constraints);
		xmlFree(value);
		errcode = oo_NOMEM;
		goto error;
	    }
	    strcpy(ct->name, value);
	    xmlFree(value);

	    ct->values = NULL;
	    ct->num_values = 0;

	    ret = ooCodeSystem_read_constraint_type_values(self, cur_node->children, ct);

	    constraints[self->num_constraints] = ct;
	    self->constraints = constraints;
	    self->num_constraints++;
	}

    }

    errcode = oo_OK;

error:
    return errcode;
}


static int
ooCodeSystem_read_XML(struct ooCodeSystem *self, xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    xmlAttr *cur_attr = NULL;
    char *value = NULL;
    size_t i;
    int ret;

    if (DEBUG_CS_LEVEL_3)
	printf("  ** Reading XML-representation of the Coding System...\n");

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"name");
    if (!value) return oo_FAIL;

    self->name = malloc(strlen(value) + 1);
    if (!self->name) return oo_NOMEM;
    strcpy(self->name, value);
    xmlFree(value);

    if (DEBUG_CS_LEVEL_3) printf("  == CS name: %s\n", self->name);

    /* is atomic? */
    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"codetype");
    if (value) {
	if (!strcmp(value, "UTF-8")) {
	    self->is_atomic = true;
	    self->atomic_codesystem_type = ATOMIC_UTF8;
	}
	if (!strcmp(value, "UTF-16")) {
	    self->is_atomic = true;
	    self->atomic_codesystem_type = ATOMIC_UTF16;
	}
	if (!strcmp(value, "SingleByte")) {
	    self->is_atomic = true;
	    self->atomic_codesystem_type = ATOMIC_SINGLEBYTE;
	}
	xmlFree(value);
    }

    /* allows polysemy? */
    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"polysemy");
    if (value) {
	if (!strcmp(value, "true")) {
	    self->allows_polysemy = true;
	}
	xmlFree(value);
    }

    /* visual separators aka spaces? */
    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"visual_separators");
    if (value) {
	if (!strcmp(value, "0")) {
	    self->use_visual_separators = false;
	}
	xmlFree(value);
    }

    /* activate numeric_denotmap table (UCS-2) */
    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"use_numcodes");
    if (value) {
	self->numeric_denotmap = malloc(sizeof(NUMERIC_CODE_TYPE) * (UCS2_MAX + 1));
	if (!self->numeric_denotmap) {
	    xmlFree(value);
	    return oo_NOMEM;
	}
	for (i = 0; i < (UCS2_MAX + 1); i++)
	    self->numeric_denotmap[i] = 0;
	self->use_numeric_codes = true;
	xmlFree(value);
    }


    /* read the children */
    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"codestruct"))) {
	    ret = ooCodeSystem_read_codestruct(self, cur_node);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"codeset"))) {
	    ret = ooCodeSystem_read_codeset(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"initcache"))) {
	    ret = ooCodeSystem_read_initcache(self, cur_node);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"constraints"))) {
	    ret = ooCodeSystem_read_constraint_types(self, cur_node->children);
	    continue;
	}



    }

    /* now that all local ids are assigned
     * use these for setting up back references */
    /*ret = self->resolve_refs(self); */

    return oo_OK;
}


/*  find solutions */
static
int ooCodeSystem_lookup(struct ooCodeSystem *self, 
			struct ooConcept *conc)
{
    int ret;
    if (DEBUG_CS_LEVEL_1)
	printf("CodeSystem: looking up \"%s\"...\n", conc->name);


    return oo_OK;
}


/*  ooCodeSystem constructor */
int ooCodeSystem_new(struct ooCodeSystem **cs)
{
    size_t i;
    int ret;
    struct ooCodeSystem *self = malloc(sizeof(struct ooCodeSystem));
    if (!self) return oo_NOMEM;
    self->read = 0;

    self->type = CS_DENOTATIONAL;
    self->name = NULL;
    self->id = 0;

    self->is_atomic = false;
    self->atomic_codesystem_type = ATOMIC_UTF8;

    self->allows_polysemy = false;

    self->operids = ooCodeSystem_operids;

    self->root_elem_name = NULL;
    self->root_elem_id = 0;

    self->use_visual_separators = true;

    self->provider_names = NULL;
    self->providers = NULL;
    self->providers_logic_oper = LOGIC_AND;
    self->num_providers = 0;

    self->inheritor_names = NULL;
    self->inheritors = NULL;
    self->num_inheritors = 0;

    self->is_coordinated = false;

    ret = ooDict_new(&self->codes);
    if (ret != oo_OK) {
	free(self);
	return ret;
    }

    self->code_names = malloc(sizeof(char*));
    if (!self->code_names) {
	free(self->codes);
	free(self);
	return oo_NOMEM;
    }
    self->code_names[0] = NULL;

    self->code_index = NULL;
    self->code_index_capacity = 0;
    self->num_codes = 0;

    self->use_numeric_codes = false;
    self->numeric_denotmap = NULL;

    self->denotmap = NULL;
    
    self->cache = NULL;

    self->constraints = NULL;
    self->num_constraints = 0;

    /* bind your methods */
    self->del = ooCodeSystem_del;
    self->read = ooCodeSystem_read_XML;
    self->lookup = ooCodeSystem_lookup;
    self->coordinate_codesets = ooCodeSystem_coordinate_codesets;
    self->build_cache = ooCodeSystem_build_cache;
    self->resolve_refs = ooCodeSystem_resolve_refs;

    *cs = self;
    return oo_OK;
}


