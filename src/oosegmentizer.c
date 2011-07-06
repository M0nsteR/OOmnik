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
 *   ---------------
 *   oosegmentizer.c
 *   OOmnik Segmentizer implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "oocodesystem.h"
#include "oosegmentizer.h"
#include "oocode.h"
#include "ooconcunit.h"
#include "ooagenda.h"

/**
 *  Destructor 
 */
static int
ooSegmentizer_del(struct ooSegmentizer *self)
{
    int i;
    struct ooDecoder *dec;

    if (self->decoders) {
	for (i = 0; i < self->num_decoders; i++) {
	    dec = self->decoders[i];
	    if (!dec) continue;
	    dec->del(dec);
	}
	free(self->decoders);
    }
    self->agenda->del(self->agenda);
    free(self);

    return oo_OK;
}

/* visualization of Segmentizer's hypotheses space */
static int
ooSegmentizer_str(struct ooSegmentizer *self)
{
    /* TODO */
    return oo_OK;
}

/* reset initial values */
static int
ooSegmentizer_reset(struct ooSegmentizer *self)
{
    size_t i;
    self->input = NULL;
    self->input_len = 0;

    self->num_parsed_atoms = 0;
    self->num_terminals = 0;

    self->next_solution_id = 0;

    for (i = 0; i < self->num_decoders; i++)
	self->decoders[i]->segm->reset(self->decoders[i]->segm);

    return oo_OK;
}


/* UTF-8 agent */
static int
ooSegmentizer_UTF8_parser(struct ooSegmentizer *self)
{
    ooATOM *input = self->input;
    size_t num_bytes = self->input_len;
    struct ooAgenda *agenda = self->agenda;
    mindmap_size_t num_value;
    struct ooConcUnit *cu;
    size_t linear_pos = 0;

    if (DEBUG_SEGM_LEVEL_2)
	printf("\n    ** UTF-8 parser activated!"
               "  Num of bytes to read: %lu\n", 
	       (unsigned long)num_bytes);

    /* reset the symbol counter */
    self->num_terminals = 0;
	
    while (*input) {
	/* single byte ASCII-code */
	if((unsigned char)*input < 128) {
	    if (DEBUG_SEGM_LEVEL_4) printf("    == ASCII code: %u\n", 
               (unsigned char)*input);
	    num_value = (unsigned char)*input;
            input++;
	    num_bytes--;

	    cu = agenda->alloc_unit(agenda);
	    if (cu == NULL) goto error;

            cu->concid = (mindmap_size_t)num_value;

	    cu->linear_pos = linear_pos;
	    cu->coverage = 1;
	    agenda->index[linear_pos] = cu;
	    linear_pos++;
	    self->num_terminals++;
	    continue;
	}
	
	/* 2-byte indicator */
	if((*input & 0xE0) == 0xC0)
	{
	    if (num_bytes < 2) {
		if (DEBUG_SEGM_LEVEL_4) 
		    printf("    -- No payload byte left :(\n");
		goto error;
	    }

	    if((input[1] & 0xC0) != 0x80) {
		if (DEBUG_SEGM_LEVEL_4) 
		    printf("    -- Invalid UTF-8 payload byte: %2.2x\n", 
			   input[1]);
		goto error;
	    }
	    
	    num_value = ((input[0] & 0x1F) << 6) | 
                         (input[1] & 0x3F);

	    if (DEBUG_SEGM_LEVEL_4)
		printf("    == UTF-8 2-byte code: %u\n", num_value);
	    num_bytes -= 2;
	    input += 2;
	    cu = agenda->alloc_unit(agenda);
	    cu->concid = (mindmap_size_t)num_value;
	    /*cu->name = "UTF-8 Code";*/

	    cu->linear_pos = linear_pos;
	    cu->coverage = 2;
	    agenda->index[linear_pos] = cu;

	    linear_pos += 2;
	    self->num_terminals++;
	    continue;
	}

	/* 3-byte indicator */
	if((*input & 0xF0) == 0xE0)
	{
	    if (num_bytes < 3) {
		if (DEBUG_SEGM_LEVEL_3) 
		    printf("    -- Not enough payload bytes left :(\n");
		goto error;
	    }
	    if((input[1] & 0xC0) != 0x80) {
		if (DEBUG_SEGM_LEVEL_3) 
		    printf("    -- Invalid UTF-8 payload byte: %2.2x\n", 
			   input[1]);
		goto error;
	    }
	    if((input[2] & 0xC0) != 0x80) {
		if (DEBUG_SEGM_LEVEL_3) 
		    printf("   -- Invalid UTF-8 payload byte: %2.2x\n", 
			   input[2]);
		goto error;
	    }

	    num_value = ((input[0] & 0x0F) << 12)  | 
                        ((input[1] & 0x3F) << 6) | 
                         (input[2] & 0x3F);
	    if (DEBUG_SEGM_LEVEL_3) 
		printf("    == UTF-8 3-byte code: %zu\n", 
		       num_value);
	    num_bytes -= 3;
	    input += 3;
	    cu = agenda->alloc_unit(agenda);
	    cu->linear_pos = linear_pos;
	    cu->coverage = 3;
	    cu->concid = (mindmap_size_t)num_value;
	    /* cu->name = "UTF-8 Code";*/
	    agenda->index[linear_pos] = cu;

	    linear_pos += 3;
	    self->num_terminals++;
	    continue;
	}

	if (DEBUG_SEGM_LEVEL_3) 
	    printf("    -- Invalid UTF-8 code: %2.2x\n", *input);
	goto error;
    }

    self->num_parsed_atoms = linear_pos;
    agenda->last_idx_pos = linear_pos;
    return oo_OK;

 error:
    self->num_parsed_atoms = linear_pos;
    return FAIL;
}

/* singlebyte encoding parser */
static int
ooSegmentizer_singlebyte_parser(ooSegmentizer *self)
{
    struct ooConcUnit *cu;
    ooATOM *input = self->input;
    struct ooAgenda *agenda = self->agenda;
    mindmap_size_t num_value;
    size_t linear_pos = 0;

    if (DEBUG_SEGM_LEVEL_2)
	printf("    ** SingleByte parser activated!\n");

    while (*input) {
	if (DEBUG_SEGM_LEVEL_3)
	    printf("    ++ SingleByte: %u\n", (unsigned char)*input);
	num_value = (unsigned char)*input;

	cu = agenda->alloc_unit(agenda);
	if (!cu) return oo_NOMEM;

	cu->concid = (mindmap_size_t)num_value;
	cu->linear_pos = linear_pos;
	cu->coverage = 1;
	agenda->index[linear_pos] = cu;
	linear_pos++;
	input++;
    }
    self->num_parsed_atoms = linear_pos;
    agenda->last_idx_pos = linear_pos;

    return oo_OK;
}


/**
 * double byte encoding parser (UCS-2/UTF-16)
 * Little Endian (x86) */
static int
ooSegmentizer_UTF16_LE_parser(ooSegmentizer *self)
{
    ooATOM *input = self->input;
    size_t num_bytes = self->input_len;
    mindmap_size_t num_value;
    /* TODO */
    return FAIL;
}

/* double byte encoding parser (UCS-2/UTF-16)
 * Big Endian (sparc) */
static int
ooSegmentizer_UTF16_BE_parser(ooSegmentizer *self)
{
    ooATOM *input = self->input;
    size_t num_bytes = self->input_len;
    mindmap_size_t num_value;
    /* TODO */
    return FAIL;
}



/* managing atomic decoders */
static int
ooSegmentizer_choose_atomic_decoder(struct ooSegmentizer *self,
				    atomic_codesystem_t atomic_cs_type)
{
    if (DEBUG_SEGM_LEVEL_3)
	printf("    ... Atomic Segmentation of type %d in progress...\n",
	       self->parent_decoder->codesystem->atomic_codesystem_type);

    switch (atomic_cs_type) {
    case ATOMIC_UTF8:
	ooSegmentizer_UTF8_parser(self);
	break;
    case ATOMIC_UTF16:
	ooSegmentizer_UTF16_LE_parser(self);
	break;
    case ATOMIC_SINGLEBYTE:
	ooSegmentizer_singlebyte_parser(self);
	break;
    default:
	return FAIL;
    }
    
    if (DEBUG_SEGM_LEVEL_3)
	printf("    ++ Atomic units found: %lu   Parsed atoms: %lu\n", 
	       (unsigned long)self->agenda->storage_space_used, 
	       (unsigned long)self->num_parsed_atoms);
    
    return oo_OK;
}



/* add units from a single decoder */
static int
ooSegmentizer_add_units(struct ooSegmentizer *self, 
			struct ooDecoder     *dec)
{
    int last_pos, i;
    struct ooAgenda *agenda = self->agenda, *input_agenda;
    struct ooConcUnit *cu, *tail_cu;
    struct ooCodeSystem *parent_cs;
    struct ooComplex *complex, *last_complex;
    int weight = 0;

    input_agenda = dec->agenda;
    if (dec->is_atomic) input_agenda = dec->segm->agenda;
    
    if (DEBUG_SEGM_LEVEL_2) {
	/* if (dec->codesystem->type == CS_OPERATIONAL) {*/
	printf("    ...Adding units from \"%s\"...\n", dec->codesystem->name);
	printf("      Num of new units: %lu last index pos: %lu  curr_idx_pos: %lu\n",
	       (unsigned long)input_agenda->storage_space_used,
	       (unsigned long)input_agenda->last_idx_pos,
	       (unsigned long)agenda->last_idx_pos);
    }

    /* keep track of alternative solutions */
    if (dec->segm->next_solution_id) {
	self->next_solution_id = dec->segm->next_solution_id;
	self->num_solutions = dec->segm->num_solutions;
    }

    if (dec->codesystem->type == CS_OPERATIONAL) {
	/* TODO: proper solution */

	for (i = 0; i < INPUT_BUF_SIZE; i++) {
	    complex = input_agenda->linear_index[i];
	    if (!complex) continue;

	    /* NB: complexes from different CS are lost here! */
	    agenda->linear_index[i] = complex;
	    last_pos = complex->linear_end;
	}

	if (last_pos > agenda->last_idx_pos) 
	    agenda->last_idx_pos = last_pos;

	return oo_OK;
    }

    /* default: DENOTATIONAL CodeSystem */

    /* transfer all good units from subordinate decoder 
     * to the aggregated agenda */
    for (i = 0; i < input_agenda->last_idx_pos; i++) {
	cu = input_agenda->index[i];
	if (!cu || !cu->concid) continue;


	if (input_agenda->tail_index[i])
	    agenda->tail_index[i]->next = agenda->index[i];

	agenda->index[i] = cu;
    }

    if (input_agenda->last_idx_pos > agenda->last_idx_pos) 
	agenda->last_idx_pos = input_agenda->last_idx_pos;


    return oo_OK;
}



/*  find linear segmentation solutions */
static int
ooSegmentizer_call_subordinate_decoder(struct ooSegmentizer *self,
				       struct ooDecoder *dec)
{
    int ret;

    dec->input = self->input;
    dec->input_len = self->input_len;
    dec->task_id = self->task_id;

    if (DEBUG_SEGM_LEVEL_3)
	printf("\n  ** Activating subdecoder \"%s\" (addr: %p, input batch id: %u)\n",
	       dec->codesystem->name, (void*)dec, self->task_id);

    /* pass a type of the preferable atomic decoder */
    if (self->pref_atomic_decoder)
	dec->segm->pref_atomic_decoder = self->pref_atomic_decoder;

    /* main decoding job */
    ret = dec->decode(dec);
    self->num_parsed_atoms = dec->num_parsed_atoms;
    self->num_terminals = dec->num_terminals;

    if (DEBUG_SEGM_LEVEL_3)
	printf("\n  ** Subdecoder \"%s\" done: %d\n",
	       dec->codesystem->name, ret);

    return ret;
}


/*  find linear segmentation solutions */
static int
ooSegmentizer_segmentize(struct ooSegmentizer *self)
{
    atomic_codesystem_t atomic_cs_type;
    int ret;
    size_t i;
    struct ooDecoder *dec;

    if (DEBUG_SEGM_LEVEL_1)
	printf("    ... Segmentizing atomic input: \"%s\"...\n", 
	       self->input);

    /* TODO: check the context... clear up the agenda */
    self->agenda->reset(self->agenda);

    /* atomic segmentation: byte(s) -> integer */
    if (self->is_atomic) {
	atomic_cs_type = self->parent_decoder->codesystem->atomic_codesystem_type;
	return ooSegmentizer_choose_atomic_decoder(self, atomic_cs_type);
    }

    if (!self->num_decoders) {
	if (DEBUG_SEGM_LEVEL_3)
	    printf("  No subordinate decoders found for the \"%s\" Segmentizer :(\n",   
		   self->parent_decoder->codesystem->name);
	return FAIL;
    }

    /* calling our subordinate decoders */
    if (DEBUG_SEGM_LEVEL_3)
	if (self->parent_decoder)
	    printf("    ** CS decoder \"%s\"  num_subordinates: %d logic: %d"
		   " next_sol: %d num_solutions: %d\n", 
		   self->parent_decoder->codesystem->name,
		   self->num_decoders, self->decoders_logic_oper,
		   self->next_solution_id, self->num_solutions);


   /* alternative decoders */
    if (self->decoders_logic_oper == LOGIC_OR) {
	if (self->next_solution_id >= self->num_solutions) return oo_FAIL;
	dec = self->decoders[self->next_solution_id++];

	ret = ooSegmentizer_call_subordinate_decoder(self, dec);
	if (ret != oo_OK) return ret; 
	return ooSegmentizer_add_units(self, dec);
    }

    /* cooperative decoders */
    for (i = 0; i < self->num_decoders; i++) {
	dec = self->decoders[i];
	ret = ooSegmentizer_call_subordinate_decoder(self, dec);
	if (ret != oo_OK) continue;
	ret = ooSegmentizer_add_units(self, dec);

	/* CHECK */
	/*self->agenda->str(self->agenda);*/
    }

    if (DEBUG_SEGM_LEVEL_3)
	printf("\n FINAL: ******* Segmentizer has merged all interpretations.\n");

    /*self->agenda->str(self->agenda);*/

    return oo_OK;
}


/**
 *  ooSegmentizer Initializer 
 */
extern int
ooSegmentizer_new(ooSegmentizer **segm)
{
    int ret;

    struct ooSegmentizer *self = malloc(sizeof(ooSegmentizer));
    if (!self) return oo_NOMEM;

    ret = ooAgenda_new(&self->agenda);
    if (ret != oo_OK) return ret;

    self->agenda->linear_structure = true;
    self->parent_decoder = NULL;

    self->decoders = NULL;
    self->num_decoders = 0;
    self->decoders_logic_oper = LOGIC_AND;

    self->is_atomic = false;

    self->input = NULL;
    self->input_len = 0;
    self->task_id = 0;
    self->num_parsed_atoms = 0;
    self->num_terminals = 0;

    self->next_solution_id = 0;
    self->num_solutions = 0;

    /* no preferable decoder is selected */
    self->pref_atomic_decoder = NULL;

    /* bind your methods */
    self->str = ooSegmentizer_str;
    self->del = ooSegmentizer_del;
    self->reset = ooSegmentizer_reset;

    self->segmentize = ooSegmentizer_segmentize;

    *segm = self;
    return 0;
}


