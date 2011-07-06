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
 *   ---------
 *   oocache.h
 *   OOmnik Linear Cache implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "ooconcunit.h"
#include "ooconstraint.h"
#include "oocode.h"
#include "oocache.h"
#include "oocodesystem.h"
#include "oosegmentizer.h"
#include "ooagenda.h"

/* destructor */
static int 
ooLinearCache_del(struct ooLinearCache *self)
{
    size_t i, j, u, *num_codes;
    const char *seq;
    struct ooLinearCacheCell *cell;
    struct ooLinearCacheTail *tail;
    struct ooCodeMatch *code_match, *next_code_match;
    struct ooCode **codes;

    /* remove all scaffoldings */
    for (i = 0; i < self->num_codes; i++) {
	seq = self->codeseqs[i];
	num_codes = self->code_list_sizes->get(self->code_list_sizes, 
					       (const char*)seq);
	if (num_codes)
	    free(num_codes);
	codes = self->codes->get(self->codes, 
				 (const char*)seq);
	if (codes) free(codes);
    }

    if (self->codes) {
	self->codes->del(self->codes);
	free(self->codes);
    }

    if (self->code_list_sizes) {
	self->code_list_sizes->del(self->code_list_sizes);
	free(self->code_list_sizes);
    }

    /* remove every cell */
    for (i = 0; i < self->num_cells; i++) {
	cell = self->matrix[i];
	if (!cell) continue;

	/* remove all possibble tails */
	for (j = 0; j < cell->num_tails; j++) {
	    tail = cell->tails[j];
	    if (!tail) continue;
	    if (tail->units)
		free(tail->units);

	    code_match = tail->code_match;
	    while (code_match) {
		next_code_match = code_match->next;
		free(code_match);
		code_match = next_code_match;
	    }
	    free(tail);
	}
	free(cell->tails);
	free(cell);
    }

    free(self->row_sizes);
    free(self->matrix);

    free(self->codeseqs);

    if (self->provider_name)
	free(self->provider_name);

    free(self);

    return 0;
}


static const char* 
ooLinearCache_str(struct ooLinearCache *self)
{
    size_t i, j, c;
    struct ooLinearCacheCell *cell;
    struct ooLinearCacheTail *cache_tail;

    if (DEBUG_CACHE_LEVEL_1)
	printf("  == CACHE for %s. Total cells: %u\n",
	       self->cs->name, self->num_cells);

    if (!self->matrix) return self->repr;

    for (i = 0; i < self->num_cells; i++) {
	cell = self->matrix[i];
	if (!cell)  continue; 

	if (DEBUG_CACHE_LEVEL_3) 
	    printf("CELL: %u  num tails: %u\n", i, cell->num_tails);

	for (j = 0; j < cell->num_tails; j++) {
	    cache_tail = cell->tails[j];
	    printf("  >>");
	    for (c = 0; c < cache_tail->num_units; c++) {
		printf(" %zu ", cache_tail->units[c]);
	    }
	    printf("\n");
	}

    }
    
    return self->repr;
}


/* compare the equality of two tails */
static
int ooLinearCache_tail_match(struct ooLinearCache *self,
		       struct ooLinearCacheTail  *tail,
		       size_t *newtail,
		       size_t newtail_len)
{   size_t i;

    /* different tails */
    if (tail->num_units != newtail_len) return oo_FAIL;

    for (i = 0; i < newtail_len; i++) 
	if (tail->units[i] != newtail[i])
	    return oo_FAIL;

    return oo_OK;
}

/* linear search of the matching tail */
static
struct ooLinearCacheTail* ooLinearCache_find_tail(struct ooLinearCache *self,
			       struct ooLinearCacheCell *cell,
			       size_t *newtail,
			       size_t newtail_len)
{   size_t i;
    int ret;
    struct ooLinearCacheTail *tail = NULL;

    for (i = 0; i < cell->num_tails; i++) {
	ret = ooLinearCache_tail_match(self, cell->tails[i], 
				    newtail, newtail_len);
	if (ret == oo_OK) {
	    tail = cell->tails[i];
	    break;
	}
    }
    return tail;
}


/**
 * calculate the position in the matrix */
static
int ooLinearCache_calc_pos(struct ooLinearCache *self,
		     struct ooSegmentizer *segm,
		     size_t *result_matrix_pos,
		     size_t *tail_start,
		     size_t *code_tail_len,
		     size_t *coverage)
{
    size_t i, curr_id, matrix_pos = 0;
    size_t cur_depth = 0;
    size_t tail_len = 0;
    struct ooConcUnit *cu;
    struct ooAgenda *agenda = segm->agenda;

    for (i = 0; i < agenda->last_idx_pos; i++) {
	cu = agenda->index[i];
	if (!cu || !cu->concid) continue;

	/* matrix offset */
	curr_id = (size_t)cu->concid;

	if (cur_depth < self->matrix_depth) {

	    /* using a multiplier */
	    matrix_pos += self->row_sizes[cur_depth] * curr_id;
	    cur_depth++;
	    (*coverage) += cu->coverage;
	    (*tail_start) = cur_depth;

	    /*printf("apply multiplier %zu for: %zu Result: %zu\n", 
	      self->row_sizes[cur_depth], curr_id, matrix_pos);*/
	    continue;
	}
	tail_len++;
	(*coverage) += cu->coverage;
    }

    /* any tail left? */
    if (DEBUG_CACHE_LEVEL_3) 
	printf("  Matrix position: %zu Tail length: %zu Tail start: %zu\n", 
	       matrix_pos, tail_len, *tail_start);

    if (matrix_pos > self->num_cells) return oo_FAIL;

    *result_matrix_pos = matrix_pos;
    *code_tail_len = tail_len;
    return oo_OK;
}






/* put a single code into the matrix */
static int 
ooLinearCache_insert_code(struct ooLinearCache *self,
		    const unsigned char *seq,
		    struct ooSegmentizer *segm)
{   size_t i, j, pos = 0, cu_count, *num_newcodes;
    struct ooCode *code, **newcodes;
    struct ooCodeMatch *code_match;
    struct ooLinearCacheCell *cell;
    struct ooLinearCacheTail **tails;
    struct ooLinearCacheTail *tail = NULL;
    struct ooConcUnit *cu;
    struct ooAgenda *agenda = segm->agenda;
    size_t *tail_units = NULL;
    size_t tail_len = 0, tail_start = 0, coverage = 0;
    bool register_newtail = false;
    int ret;

    if (DEBUG_CACHE_LEVEL_3)
	printf("  ** Inserting Sequence \"%s\" to the Cache Matrix...\n", 
	       seq);

    /* get the codes that correspond to this atomic sequence */
    newcodes = self->codes->get(self->codes, (const char*)seq);
    if (!newcodes) return oo_FAIL;

    num_newcodes = self->code_list_sizes->get(self->code_list_sizes, 
					      (const char*)seq);
    if (!num_newcodes) return oo_FAIL;

    ret = ooLinearCache_calc_pos(self, segm, &pos, &tail_start, &tail_len, &coverage);
    if (ret != oo_OK) return oo_FAIL;

    if (tail_len) {
	tail_units = malloc(sizeof(size_t) * tail_len);
	if (!tail_units) return oo_NOMEM;

	j = 0;
	cu_count = 0;
	for (i = 0; i < agenda->last_idx_pos; i++) {
	    cu = agenda->index[i];
	    if (!cu || cu->concid == 0) continue;
	    if (cu_count < tail_start) {
		cu_count++;
		continue;
	    }
	    /*printf("UNIT %zu) tail component: %s %zu\n", 
	      i, cu->code->name, cu->concid);*/

	    tail_units[j++] = (size_t)cu->concid;
	    if (j == tail_len) break;
	}
    }

    if (DEBUG_CACHE_LEVEL_4)
	printf("  ++ Position of sequence \"%s\" in Cache matrix: %u Tail length: %u\n", 
	       seq, pos, tail_len);

    cell = self->matrix[pos];

    /* initialize the cell */
    if (!cell) {
	cell = malloc(sizeof(struct ooLinearCacheCell));
	if (!cell) {
	    free(tail_units);
	    return oo_NOMEM;
	}
	cell->num_tails = 0;
	cell->max_tail_len = 0;
	cell->tails = NULL;
    }

    /* add code as one of the cell tails  */
    /* get the matching tail */
    if (tail_units)
	tail = ooLinearCache_find_tail(self, cell, tail_units, tail_len);
   

    if (!tail) {
	tail = malloc(sizeof(struct ooLinearCacheTail));
	if (!tail) {
	    free(tail_units);
	    return oo_NOMEM;
	}
	tail->units = tail_units;
	tail->num_units = tail_len;
	tail->coverage = coverage;
	tail->code_match = NULL;
	register_newtail = true;
    }
    else {
	free(tail_units);
    }

    for (i = 0; i < *num_newcodes; i++) {
	code = newcodes[i];

	code_match = malloc(sizeof(struct ooCodeMatch));
	if (!code_match) return oo_NOMEM;

	code_match->code = code;
	code_match->context = NULL;

	/* find the exact match of this sequence with specific context */
	for (j = 0; j < code->cache->num_seqs; j++) {
	    if (!strcmp((const char*)seq, (const char*)code->cache->seqs[j])) {
		if (code->cache->contexts[j]) {
		    /*printf("add context for %s %p\n", seq, code->cache->contexts[j]);*/
		    code_match->context = code->cache->contexts[j];
		}
		break;
	    }
	}

	code_match->next = tail->code_match;
	tail->code_match = code_match;
    }

    /* add a new tail */
    if (register_newtail) {
	tails = realloc(cell->tails, sizeof(struct ooLinearCacheTail*) *
			(cell->num_tails + 1));
	if (!tails) return oo_NOMEM;
	tails[cell->num_tails] = tail;
	cell->tails = tails;
	cell->num_tails++;
    }

    if (tail_len > cell->max_tail_len) 
	cell->max_tail_len = tail_len;

    self->matrix[pos] = cell;

    return oo_OK;
}

static
int ooLinearCache_populate_matrix(struct ooLinearCache *self)
{   
    struct ooSegmentizer *segm;
    struct ooDecoder *dec;
    size_t i;
    const unsigned char *seq;
    int ret;

    fprintf(stderr,"  ++ Populating the Linear Cache (provider: %s)...\n", 
	    self->provider->name);

    /* cache segmentizer */
    ret = ooSegmentizer_new(&segm);
    if (ret != oo_OK) return ret;

    segm->agenda->codesystem = self->provider;
    segm->decoders = malloc(sizeof(struct ooDecoder*));
    if (!segm->decoders) return oo_NOMEM;

    /* subordinate decoder */
    ret = ooDecoder_new(&dec);
    if (ret != oo_OK) {
	segm->del(segm);
	return ret;
    }

    ret = dec->set_codesystem(dec, self->provider);
    if (ret != oo_OK) return ret;

    dec->used_by_cache = true;
    segm->decoders[0] = dec;
    segm->num_decoders = 1;

    /* TODO: read the atomic encoding from XML-file */
    segm->pref_atomic_decoder = ATOMIC_UTF8;

    for (i = 0; i < self->num_codes; i++) {
	seq = (const unsigned char*)self->codeseqs[i];

	if (DEBUG_CACHE_LEVEL_3)
	    printf("\n   ...Adding CODESEQ \"%s\" to Cache...\n", seq);

	/* prepare the segmentizer to split this sequence */
	segm->reset(segm);
	segm->input = seq;
	segm->input_len = (size_t)strlen((const char*)seq);

	ret = segm->segmentize(segm);
	if (ret != oo_OK) continue;

	if (!segm->agenda->last_idx_pos) {
	    if (DEBUG_CACHE_LEVEL_4)
		printf("   -- Nothing was recognized by Segmentizer :((\n");
	    continue;
	}


	if (DEBUG_CACHE_LEVEL_4)
	    printf("Total units found by Segmentizer: %u\n",
		   segm->agenda->last_idx_pos - 1);

	ret = ooLinearCache_insert_code(self, seq, segm);
	fprintf(stderr, "   .. codes added: %d...\r", i);
    }

    segm->del(segm);

    return oo_OK;
}

/* set a new cache item, remember its char sequence */
static
int ooLinearCache_set(struct ooLinearCache *self, 
		      struct ooCode *code)
{
    size_t i, *num_codes;
    const unsigned char *seq, **seqs;
    struct ooCode **codes;
    int ret;

    if (DEBUG_CACHE_LEVEL_3) {
	printf("\n adding %s (%p) to cache...\n", code->name, (void*)code);
	code->str(code);
    }

    for (i = 0; i < code->cache->num_seqs; i++) {
	seq = code->cache->seqs[i];

	if (DEBUG_CACHE_LEVEL_3)
	    printf(" -- code's seq: %s\n", seq);

	/* how many codes do we have for this sequence? */
	num_codes = self->code_list_sizes->get(self->code_list_sizes, 
					       (const char*)seq);
	if (!num_codes) {
	    num_codes = malloc(sizeof(size_t));
	    *num_codes = 1;
	    codes = malloc(sizeof(struct ooCode*));
	    codes[0] = code;
	}
	else {
	    (*num_codes)++;
	    codes = self->codes->get(self->codes, (const char*)seq);
	    codes = realloc(codes, (sizeof(struct ooCode*) *
                             (*num_codes)));
	    codes[(*num_codes) - 1] = code;
	}

	if (DEBUG_CACHE_LEVEL_3)
	    printf(" ++ curr num of codes for \"%s\": %u\n", seq, *num_codes);

	ret = self->codes->set(self->codes, (const char*)seq, codes);

	ret = self->code_list_sizes->set(self->code_list_sizes, 
					 (const char*)seq, 
					 num_codes);

	/* remember the key if it's new */
	if (*num_codes > 1) continue;

	seqs = realloc(self->codeseqs, (sizeof(unsigned char*) *
                             (self->num_codes  + 1)));
	if (!seqs) return oo_NOMEM;

	seqs[self->num_codes] = seq;
	self->codeseqs = seqs;
	self->num_codes++;
    }

    return oo_OK;
}


static
int ooLinearCache_update_agenda(struct ooLinearCache *self, 
			  struct ooLinearCacheTail *tail,
			  size_t linear_pos,
			  size_t coverage,
			  size_t term_count,
			  size_t num_terms,
			  struct ooAgenda *agenda)
{
    struct ooConcUnit *cu;
    struct ooCodeMatch *cm;
 
    cm = tail->code_match;

    while (cm) {
	cu = agenda->alloc_unit(agenda);
	if (!cu) return oo_NOMEM;

	cu->code = cm->code;
	cu->context = cm->context;

	cu->concid = cu->code->id;
	cu->linear_pos = linear_pos;
	cu->coverage = coverage;
	cu->start_term_pos = term_count;
	cu->num_terminals = num_terms;

	if (DEBUG_CACHE_LEVEL_3) {
	    printf("  ++ cache match:\n");
	    cu->str(cu, 1);
	}

	cu->next = agenda->index[linear_pos];
	agenda->index[linear_pos] = cu;
	agenda->last_idx_pos = linear_pos + 1;

	cm = cm->next;
    }

    return oo_OK;
}

static
int ooLinearCache_match(struct ooLinearCache *self, 
		  size_t curr_pos,
		  size_t term_count,
		  size_t *tail_buf,
		  struct ooSegmentizer *segm,
		  struct ooAgenda *agenda)
{   int ret, success = oo_FAIL;
    size_t i, pos = 0, cur_depth = 0;
    size_t coverage = 0, num_terms = 0;
    size_t tail_len = 0, max_tail_len = 0;
    struct ooConcUnit *cu = NULL;
    struct ooLinearCacheCell *cell;
    struct ooLinearCacheTail *tail;

    for (i = curr_pos; i < segm->agenda->last_idx_pos; i++) {
	cu = segm->agenda->index[i];
	if (!cu) continue;
	num_terms++;

	/* unrecognized unit is met */
	if (cu->concid == 0) {
	    if (i == 0) break;
	}
	coverage += cu->coverage;

	/* calculate the position in the flat matrix */
	if (cur_depth < self->matrix_depth) {
	    /* using a multiplier */
	    pos += self->row_sizes[cur_depth] * (size_t)cu->concid;
	    cur_depth++;
	}
	else {
	    if (tail_len > max_tail_len) break;
	    tail_buf[tail_len] = cu->concid;
	    tail_len++;
	}

	/*printf("UNIT: %zu  CONCID: %zu  DEPTH: %zu  MATRIX_POS: %zu\n", 
	  num_terms, cu->concid, cur_depth, pos);*/



	/* this should never happen.. but just in case.. */
	if (pos > self->matrix_size) break;

	cell = self->matrix[pos];
	if (!cell)  continue;

	max_tail_len = cell->max_tail_len;

	tail = ooLinearCache_find_tail(self, cell, tail_buf, tail_len);
	if (!tail) continue;
	success = oo_OK;

	/* register this tail */
	ret = ooLinearCache_update_agenda(self, tail,
				    curr_pos, coverage, 
				    term_count, num_terms, agenda);
    }
    return success;
}

/** 
 * Lookup linear sequence of concids in cache.
 *  Save results directly to agenda.
 */
static
int ooLinearCache_lookup(struct ooLinearCache *self,
		   struct ooSegmentizer *segm,
		   ooATOM *input,
		   size_t input_len,
		   size_t task_id,
		   struct ooAgenda *agenda)
{   size_t i, tail_buf[INPUT_BUF_SIZE], term_count = 0;
    struct ooConcUnit *cu = NULL, *src_cu = NULL;
    int ret;

    if (DEBUG_CACHE_LEVEL_1) printf("  ** Cache Matrix lookup .....\n");

    /* let the subordinate segmentizer produce terminal units */
    segm->input = input;
    segm->input_len = input_len;
    segm->task_id = task_id;

    ret = segm->segmentize(segm);
    if (ret != oo_OK) return ret;

    agenda->linear_structure = true;

    for (i = 0; i < segm->agenda->last_idx_pos; i++) {
	src_cu = segm->agenda->index[i];
	if (!src_cu) continue;

	term_count++;

	if (!src_cu->concid) continue;

	if (DEBUG_CACHE_LEVEL_3)
	    printf("  ++ cache lookup at %d\n", i);

	/* register a single unit as an unrecognized unit
         * RATIONALE: to handle spelling errors, abbreviations 
         *            unknown sequences etc. 
         */
	cu = agenda->alloc_unit(agenda);
	if (!cu) return oo_NOMEM;
	cu->linear_pos = src_cu->linear_pos;
	cu->coverage = src_cu->coverage;
	cu->terminals = src_cu;
	agenda->index[i] = cu;

	/* now try to match sequences starting from this position
         * with the valid linear sequences 
         */
	ret = ooLinearCache_match(self, i, term_count - 1, tail_buf, segm, agenda);
    }


    if (DEBUG_CACHE_LEVEL_2) {
	printf("\n   == Total terminal symbols read by ooLinearCache: %lu\n", 
	       (unsigned long)term_count);
	agenda->str(agenda);
    }

    return oo_OK;
}

/*  build Cache matrix  */
static int ooLinearCache_build_matrix(struct ooLinearCache *self)
{
    size_t i, num_cells = 0;

    if (!self->provider) return oo_FAIL;
    if (self->matrix_depth == 0) return oo_FAIL;

    if (DEBUG_CACHE_LEVEL_1)
	printf("Matrix depth: %lu\n", (unsigned long)self->matrix_depth);

    self->row_sizes = malloc(sizeof(size_t) * self->matrix_depth);
    if (!self->row_sizes) return oo_NOMEM;

    num_cells = self->provider->num_codes;

    /* last multiplier equals to 1 */
    self->row_sizes[self->matrix_depth - 1] = 1;

    for (i = self->matrix_depth - 1; i > 0; i--) {
	self->row_sizes[i-1] = num_cells;
	/*if (DEBUG_CACHE_LEVEL_3)
	  printf(" pos: %u: value: %u\n", i - 1, self->row_sizes[i - 1]);*/
	num_cells = num_cells * self->provider->num_codes;
    }

    self->num_cells = num_cells;
    self->matrix_size = sizeof(struct ooLinearCacheCell*) * self->num_cells;

    if (DEBUG_CACHE_LEVEL_4)
	for (i = 0; i < self->matrix_depth; i++)
	    printf("%zu: multiplier: %zu\n", i, self->row_sizes[i]);

    if (DEBUG_CACHE_LEVEL_3)
	printf("  Total cells: %zu Matrix size in bytes: %zu\n", 
	       self->num_cells, self->matrix_size);

    /* check the maximum cache size 
     * that is dynamically set by the main controller
     */
    if (self->matrix_size > MAX_MEMCACHE_SIZE) {
	if (DEBUG_CACHE_LEVEL_3)
	    printf("  -- Memcache limit reached :(\n");
	free(self->row_sizes);
	free(self);
	return oo_FAIL;
    }

    self->matrix = malloc(self->matrix_size);
    if (!self->matrix) return oo_NOMEM;

    for (i = 0; i < self->num_cells; i++)
	self->matrix[i] = NULL;

    if (DEBUG_CACHE_LEVEL_1)
	printf("  !! Matrix build successful!\n");

    return oo_OK;
}

/*  ooLinearCache constructor 
 */
int ooLinearCache_new(struct ooLinearCache **cache)
{   int ret;
    struct ooLinearCache *self = malloc(sizeof(struct ooLinearCache));
    if (!self) return oo_NOMEM;

    self->cs = NULL;
    self->provider_name = NULL;
    self->provider = NULL;

    self->repr = NULL;

    self->matrix = NULL;
    self->matrix_depth = DEFAULT_MATRIX_DEPTH;
    self->max_unrec_chars = DEFAULT_MAX_UNREC_CHARS;
    self->trust_separators = false;
    self->num_cells = 0;

    /* temporary storage of codes */
    ret = ooDict_new(&self->codes);
    if (ret != oo_OK) {
	free(self);
	return oo_NOMEM;
    }

    ret = ooDict_new(&self->code_list_sizes);
    if (ret != oo_OK) {
	self->codes->del(self->codes);
	free(self);
	return oo_NOMEM;
    }

    self->codeseqs = NULL;
    self->num_codes = 0;

    /* public methods */
    self->del = ooLinearCache_del;
    self->str = ooLinearCache_str;
    self->set = ooLinearCache_set;
    self->populate_matrix = ooLinearCache_populate_matrix;
    self->build_matrix = ooLinearCache_build_matrix;
    self->lookup = ooLinearCache_lookup;

    *cache = self;
    return oo_OK;
}
