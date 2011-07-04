/**
 *   Copyright (c) 2011 by Dmitri Dmitriev 
 *   <http://www.oomnik.ru>
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received
 *   as part of this distribution.
 *
 *   oomnik.h
 *   OOmnik Controller Header 
 */
#ifndef OOMNIK_H
#define OOMNIK_H

#include "ooconfig.h"
#include "oomindmap.h"
#include "ooconcept.h"


/* forward declarations */
struct ooDecoder;


/**
 * OOmnik Controller:
 * main controller of decoding process
 * producing the conceptual graph
 * as a result of interpretation of input atoms (= bytes)
 */
typedef struct OOmnik {

    /***********  public attributes **********/
    struct ooMindMap *mindmap;

    const char *conf_name;
    char **includes;
    size_t num_includes;

    char *db_filename;

    char *default_codesystem_name;
    struct ooCodeSystem *default_codesystem;

    char *includes_path;

    output_type default_format;

    /* public methods */
    int   (*del)(struct OOmnik *self);
    int   (*str)(struct OOmnik *self);
    
    /* linking the decoder with specific Mind Map */
    int (*open_mindmap)(struct OOmnik *self, const char *dbname);


    /*  start the interactive shell */
    int (*interact)(struct OOmnik *self);

    /*  re-read all knowledge sources */
    int (*reload)(struct OOmnik *self);

    /*  process string from memory */
    int (*process)(struct OOmnik *self, 
		   struct ooDecoder *dec,
		   const char *input,
		   output_type format);

    /***********  private attributes ************/

    /* cached string representation */
    char *_str;

} OOmnik;


/* for external systems like .NET */
#ifdef __cplusplus
extern "C" {
#endif

EXPORT extern void* OOmnik_create(const char *conf_name);
EXPORT extern const char* OOmnik_process(void *oomnik, 
					 const char *buf,
					 int format);
EXPORT extern int OOmnik_free_result(void *buf);

extern int OOmnik_new(struct OOmnik **self);

#ifdef __cplusplus
}
#endif


#endif /* OOMNIK_H */
