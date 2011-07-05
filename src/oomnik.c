/**
 *   Copyright (c) 2011 by Dima Dmitriev <http://www.oomnik.ru>
 *   All rights reserved.
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   oomnik.c
 *   OOmnik Controller implementation 
 */

/* 
 * standard headers
 */
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <string.h>

/*
 * third-party libraries 
 */
#include <libxml/parser.h>

/* 
 * local headers 
 */
#include "ooconfig.h"
#include "oomnik.h"
#include "ooconcunit.h"
#include "oosegmentizer.h"
#include "ooagenda.h"

/*
 * prototypes 
 */
static int
OOmnik_read_data(struct OOmnik *self, const char *config);


/*  destructor */
static int
OOmnik_del(OOmnik *self)
{
    int i;

    /* free up the subordinate resources */
    if (self->mindmap) 
	self->mindmap->del(self->mindmap);

    if (self->db_filename)
	free(self->db_filename);

    if (self->default_codesystem_name)
	free(self->default_codesystem_name);

    /* delete include path strings */
    for (i = 0; i < self->num_includes; i++)
	if (self->includes[i])
	    free(self->includes[i]);
    if (self->includes)
	free(self->includes);

    if (self->includes_path)
	free(self->includes_path);

    /* free up yourself */
    free(self);

    return oo_OK;
}

/**
 *  object string representation 
 */
static int
OOmnik_str(OOmnik *self)
{
    /* TODO */
    return oo_OK;
}


/**
 * read included XML files 
 */
static int
OOmnik_add_includes(struct OOmnik *self,
		    xmlNode *input_node)
{
    xmlNodePtr cur_node;
    char *value, *filename;
    char **filenames;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"filename");
	    if (!value) continue;
	    filename = malloc(strlen(self->includes_path) +
			      strlen(value) + 1);
	    if (!filename) {
		xmlFree(value);
		return oo_NOMEM;
	    }
	    strcpy(filename, self->includes_path);
	    strcat(filename, value);

	    filenames = realloc(self->includes, sizeof(char *) * 
				(self->num_includes + 1));
	    if (!filenames) {
		free(filename);
		xmlFree(value);
		return oo_NOMEM;
	    }
	    filenames[self->num_includes] = filename;
	    self->includes = filenames;
	    self->num_includes++;
	    xmlFree(value);
	}
    }

    return oo_OK;
}



/* read configuration XML file */
static int
OOmnik_read_config(struct OOmnik *self, 
		   const char *filename)
{
    int errcode, path_size = 0, chunk_size = 0, db_filename_size = 0, ret;
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value;
    const char *path;

    if (DEBUG_LEVEL_1)
	fprintf(stderr, " -- OOmnik: reading config file \"%s\"...\n", filename);

    doc = xmlParseFile(filename);
    if (!doc) {
	fprintf(stderr, "Couldn't read \"%s\" :( \n", filename);
	errcode = -1;
	goto error;
    }

    /* extract the path to directory */
    for (path = filename; *path; path++) {
	if ('\\' == *path || '/' == *path) {
	    path_size += chunk_size + 1;
	    chunk_size = 0;
	    continue;
	}
	chunk_size++;
    }

    self->includes_path = malloc(path_size + 1);
    if (!self->includes_path) {
	errcode = oo_NOMEM;
	goto error;
    }

    strncpy(self->includes_path, filename, path_size);
    self->includes_path[path_size] = '\0';

    /* MindMap DB filename */
    db_filename_size = strlen(self->includes_path) +\
	strlen("default.mm") + 1;
    self->db_filename = malloc(db_filename_size);
    if (!self->db_filename) {
	errcode = oo_NOMEM;
	goto error;
    }

    strcpy(self->db_filename, self->includes_path);
    strcpy(self->db_filename + path_size, "default.mm");

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	errcode = -2;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "oomniconfig")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"oomniconfig\"");
	errcode = -3;
	goto error;
    }

    /* read the children of "oomniconfig" */

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"db"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"filename");
	    if (value) {
		if (self->db_filename) {
		    free(self->db_filename);
		    self->db_filename = NULL;
		}
		self->db_filename = malloc(strlen(value) + 1);
		if (!self->db_filename) {
		    errcode = oo_NOMEM;
		    xmlFree(value);
		    goto error;
		}
		strcpy(self->db_filename, value);
		xmlFree(value);
	    }
	}
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"output"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"format");
	    if (value) {
		if (!strcmp(value, "JSON"))
		    self->default_format = FORMAT_JSON;
		else if (!strcmp(value, "XML"))
		    self->default_format = FORMAT_XML;
		xmlFree(value);
	    }
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"codesystem"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (value) {
		self->default_codesystem_name = malloc(strlen(value) + 1);
		if (!self->default_codesystem_name) {
		    errcode = oo_NOMEM;
		    xmlFree(value);
		    goto error;
		}
		strcpy(self->default_codesystem_name, value);
		xmlFree(value);
	    }
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"includes"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"path");
	    if (value) {
		if (self->includes_path) {
		    free(self->includes_path);
		    self->includes_path = NULL;
		}
		self->includes_path = malloc(strlen(value) + 1);
		if (!self->includes_path) {
		    errcode = oo_NOMEM;
		    xmlFree(value);
		    goto error;
		}
		strcpy(self->includes_path, value);
		xmlFree(value);
	    }
	    ret = OOmnik_add_includes(self, cur_node->children);
	}

    }

    fprintf(stderr, "\n Initializing OOmnik...\n"
	    "    * working directory: %s\n", self->includes_path);
    fprintf(stderr, "    * MindMap DB file name: %s\n", self->db_filename);

    errcode = oo_OK;

error:
    if (errcode != oo_OK)
	fprintf(stderr, "\n Error reading configuration file \"%s\"\n", filename);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return errcode;
}




/*  open the MindMap DB */
static int 
OOmnik_open_mindmap(OOmnik *self, const char *dbname)
{
    if (self->mindmap->open(self->mindmap, dbname) != oo_OK) {
	fprintf(stderr, 
		"Failed to activate the MindMap database: \"%s\" :(\n",
		dbname);
	return FAIL;
    }
    return oo_OK;
}

/*  hot restart */
static int 
OOmnik_reload(struct OOmnik *self)
{
    int i, ret;

    fprintf(stderr, " Reloading OOmnik...\n");

    /* free up the subordinate resources */
    if (self->mindmap) 
	self->mindmap->del(self->mindmap);

    if (self->db_filename)
	free(self->db_filename);

    if (self->default_codesystem_name)
	free(self->default_codesystem_name);

    /* delete include path strings */
    for (i = 0; i < self->num_includes; i++)
	if (self->includes[i])
	    free(self->includes[i]);
    if (self->includes)
	free(self->includes);

    if (self->includes_path)
	free(self->includes_path);

    /* recreate the Mind Map */
    ret = ooMindMap_new(&self->mindmap);
    if (ret != oo_OK) return ret;

    self->db_filename = NULL;
    self->default_codesystem_name = NULL;
    self->default_codesystem = NULL;
    self->default_format = FORMAT_XML;

    self->includes_path = NULL;
    self->includes = NULL;
    self->num_includes = 0;

    OOmnik_read_data(self, self->conf_name);

    return oo_OK;
}

static int
OOmnik_interact(struct OOmnik *self)
{
    char buf[INPUT_BUF_SIZE];
    size_t task_id = 1;
    output_type format = FORMAT_XML;
    int ret;
    const char *result = NULL;

    fprintf(stderr, " OOmnik :)  At your service!\n");
    fprintf(stderr, ">>> ");

    while (fgets(buf, sizeof(buf), stdin)) {

	if (!strcmp(buf, "r\n")) {
	    ret = OOmnik_reload(self);
	    if (ret != oo_OK) {
		fprintf(stderr, " -- Reload failed... :(( Exiting now...\n");
		return oo_FAIL;
	    }
	    fprintf(stderr, "\n  OOmnik reloaded! :)\n");
	    fprintf(stderr, ">>> ");
	    continue;
	}

	result = OOmnik_process(self, buf, 0);

	if (!result) {
	    fprintf(stderr, "  OOmnik :( \"I\'m sorry, I don't see what you mean?\"\n");
	    fprintf(stderr, ">>> ");
	    continue;
	}

	printf("%s\n", result);
	OOmnik_free_result(result);

	fprintf(stderr, ">>> ");
    }

    if (feof(stdin)) {
	fprintf(stderr, "\nBye!\n");
    }

    return oo_OK;
}



static int
OOmnik_read_data(struct OOmnik *self, 
		 const char *conf_name)
{
    struct ooMindMap *mm;
    struct ooCodeSystem *cs = NULL;
    char *include_filename;
    int i, ret;
 
    ret = OOmnik_read_config(self, conf_name);
    if (ret != oo_OK) goto error;

    self->conf_name = conf_name;

    fprintf(stderr, "    * default CS: %s\n",
	   self->default_codesystem_name);

    if (self->open_mindmap(self, self->db_filename) != oo_OK) {
	fprintf(stderr, "  -- Failed to activate the MindMap database: \"%s\" :(\n",
		self->db_filename);
	/*goto error;*/
    }

    mm = self->mindmap;

    fprintf(stderr, "  OOmnik: importing the Concepts...\n");

    /* read the XML datasets */
    for (i = 0; i < self->num_includes; i++) {
	include_filename = self->includes[i];
	fprintf(stderr, "  ++ importing %s...\n", include_filename);
	ret = mm->import_file(mm, include_filename);
    }

    fprintf(stderr, "  OOmnik: resolving name references...\n");
    ret = mm->resolve_refs(mm);

    /*mm->str(mm);*/

    if (ret != oo_OK) {
	    fprintf(stderr, "  -- Couldn't resolve the references "
                             " in CodeSystems... :((\n");
	goto error;
    }

    if (DEBUG_LEVEL_1)
	printf("\n   Coordination of CodeSystems complete!\n");

    ret = mm->build_cache(mm);

    if (DEBUG_LEVEL_1)
	printf("\n   OOmnik Cache is ready!\n");

    cs = mm->get_codesystem(mm, self->default_codesystem_name);
    if (!cs) {
	fprintf(stderr, "  -- OOmnik: failed to assign the CodeSystem \"%s\"\n",
		self->default_codesystem_name);
	goto error;
    }

    self->default_codesystem = cs;

    return oo_OK;

 error:
    return oo_FAIL;
}


EXPORT extern int
OOmnik_free_result(void *outbuf)
{
    char *buf = (char*)outbuf;
    if (buf)
	free(buf);

    return oo_OK;
}


/* for external systems */
EXPORT extern const char*
OOmnik_process(void *oomnik, 
	       const char *input,
	       int format)
{
    struct OOmnik *self = (struct OOmnik*)oomnik;
    struct ooDecoder *dec;
    char *output_buf = NULL;
    int ret;

    output_buf = malloc(OUTPUT_BUF_SIZE);
    if (!output_buf) return NULL;
   
    /* init new Decoder */
    ret = ooDecoder_new(&dec);
    if (ret != oo_OK) return NULL;

    ret = dec->set_codesystem(dec, self->default_codesystem);

    if (ret != oo_OK) {
	printf("  Sorry, the default codesystem \"%s\" is not available :(\n",
	     self->default_codesystem_name);
	return NULL;
    }

    dec->is_root = true;
    dec->oomnik = self;
    dec->format = (output_type)format;

    ret = dec->process(dec, input);


    /*if (!dec->agenda->accu->output_buf) {
	free(output_buf);
	ret = dec->del(dec);
	return NULL;
	}*/


    dec->agenda->accu->present_solution(dec->agenda->accu,
					output_buf, OUTPUT_BUF_SIZE);

    /*sprintf(output_buf, "%s", dec->agenda->accu->solution);*/

    ret = dec->del(dec);

    return output_buf;
}


EXPORT extern void* 
OOmnik_create(const char *conf_name)
{
    struct OOmnik *oomnik;
    int ret;

    ret = OOmnik_new(&oomnik);

    /* if init failed  we return immediately
     *  and don't call a cleanup destructor */
    if (ret != oo_OK) return NULL; 

    ret = OOmnik_read_data(oomnik, conf_name);
    if (ret != oo_OK) goto error;

    fprintf(stderr,"\n   OOmnik is up and running!\n");

    return (void*)oomnik;

error:
    oomnik->del(oomnik);
    return NULL;
}


/*  OOmnik Initializer */
extern int 
OOmnik_init(struct OOmnik *self)
{
    int ret;

    self->conf_name = NULL;
    self->mindmap = NULL;

    ret = ooMindMap_new(&self->mindmap);
    if (ret != oo_OK) {
	free(self);
	return ret;
    }

    self->db_filename = NULL;
    self->default_codesystem_name = NULL;
    self->default_codesystem = NULL;

    self->default_format = FORMAT_XML;

    self->includes_path = NULL;
    self->includes = NULL;
    self->num_includes = 0;
    
    /* bind your methods */
    self->str = OOmnik_str;
    self->del = OOmnik_del;

    self->open_mindmap = OOmnik_open_mindmap;
    self->reload = OOmnik_reload;
    self->interact = OOmnik_interact;
    self->process = OOmnik_process;

    return oo_OK;
}


extern int 
OOmnik_new(struct OOmnik **oom)
{
    int ret;
    struct OOmnik *self = malloc(sizeof(struct OOmnik));
    if (!self) return oo_NOMEM;

    OOmnik_init(self);

    *oom = self;
    return oo_OK;
}


