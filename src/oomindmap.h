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
 *   oomindmap.h
 *   OOmnik MindMap 
 */

#ifndef OOMINDMAP_H
#define OOMINDMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <db.h>



struct ooCodeSystem;
struct ooConcept;
struct ooDomain;
struct ooDict;

/*  the Mind Map Controller */
typedef struct ooMindMap 
{
    unsigned long id;
    unsigned long total;
    unsigned long curr_concid;

    /* root concept with id = 0 */
    struct ooConcept *root;

    /* a batch of retrieved concepts */
    struct ooConcept *result;

    /* activated Coding Systems */
    struct ooCodeSystem **codesystems;
    int num_codesystems;

    struct ooConcept **concept_index;
    size_t concept_index_size;       /* current capacity */
    size_t num_concepts; 

    struct ooDomain *root_domain;
    struct ooDomain **domains;
    size_t num_domains;

    struct ooTopic **topics;
    size_t num_topics;
    struct ooTopicIngredient **topic_index;
    
    /***********  public methods ***********/
    int (*del)(struct ooMindMap *self);
    const char* (*str)(struct ooMindMap *self);

    /*  open the MindMap DB */
    int (*open)(struct ooMindMap *self, const char *dbname);

    /*  close the MindMap DB */
    int (*close)(struct ooMindMap *self);

    /* lookup a concept by name */
    struct ooConcept* (*lookup)(struct ooMindMap *self, const char *conc_name);

    /* retrieve a concept by its numeric id */
    struct ooConcept* (*get)(struct ooMindMap *self, mindmap_size_t id);


    /* retrieve a CodeSystem by its URI */
    struct ooCodeSystem* (*get_codesystem)(struct ooMindMap*, 
                                       const char *cs_uri);

    /* transform names to real references */
    int (*resolve_refs)(struct ooMindMap *self);

    /* build cache for each of the CodeSystems */
    int (*build_cache)(struct ooMindMap *self);

    /* generate a new Concept id */
 
    mindmap_size_t (*newid)(struct ooMindMap *self);

    /* save new concept in MindMap DB */
    int (*put)(struct ooMindMap*, struct ooConcept*);

    /* save new concept in memory */
    int (*add_concept)(struct ooMindMap*, struct ooConcept*);

    /* remove a concept by id */
    int (*drop)(struct ooMindMap *self, mindmap_size_t);

    /* list existing concept ids in batches */
    int (*keys)(struct ooMindMap  *self,
		  mindmap_size_t **ids,
		  mindmap_size_t   batch_size,
		  mindmap_size_t   batch_start);

    /* load concepts from file */
    int (*import_file)(struct ooMindMap *self, 
		       const char *filename,
		       struct ooDomain *parent_domain);

    /* save the complete MindMap DB into a file */
    int (*export_file)(struct ooMindMap *self, const char *filename);


    /***********  private attributes ***********/

    /* long-term memory:
       persistent  Concept Storage implemented as 
       Berkeley DB backend */
    DB *_storage;

    /* last concept identifier */
    mindmap_size_t _currid;

    wchar_t *repr;

    struct ooDict *_name_index;

} ooMindMap;

extern int ooMindMap_new(struct ooMindMap**); 


#endif /* OOMINDMAP_H */
