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
 *   ooconcept.h
 *   OOmnik Concept implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <db.h>

#include "ooconfig.h"
#include "oomindmap.h"
#include "ooconcept.h"
#include "oodomain.h"
#include "ooutils.h"


extern const char *oo_oper_type_names[] = 
{  OO_STR(OO_NONE), 
   OO_STR(OO_IS_SUBCLASS), 
   OO_STR(OO_AGGREGATES), 
   OO_STR(OO_ATTR), 
   OO_STR(OO_ARG), 
   OO_STR(OO_RUNS), 
   OO_STR(OO_NEXT),
   OO_STR(OO_NUM_OPERS),
   NULL 
};

/*  Concept Destructor */
static int 
ooConcept_del(ooConcept *self)
{
    int ret;
    size_t i;

    /* free up your subordinate resources */

    free(self->name);
    free(self->id);
    
    if (self->bytecode != NULL) free(self->bytecode);

    for (i = 0; i < self->_num_attrs; i++) {
	free(self->_attrs[i]);
    }
    free(self->_attrs);


    free(self);

    return oo_OK;
}

/*  string representation method */
static const char* 
ooConcept_str(ooConcept *self)
{
    size_t i;
    printf("Concept id: %s name: %s\n", 
           self->id, self->name);

    for (i = 0; i < self->_num_attrs; i++) {
	if (!self->_attrs[i])
	    continue;
	printf("%d\n", i);
	printf("  -- attr >>> [%lu]\n",
	       (unsigned long)self->_attrs[i]->concid );
    }

    return "Concept presentation";
}


/* calculate the number of valid attributes */
static
size_t ooConcept_calc_valid_attrs(ooConcept *self)
{   size_t i;
    size_t num_valid_attrs = 0;

    for (i = 0; i < self->_num_attrs; i++) {
	if (!self->_attrs[i])
	    continue;
	num_valid_attrs++;
    }
    if (DEBUG_LEVEL_3) {
	printf("Valid attrs: %d\n", num_valid_attrs);
    }
    return num_valid_attrs; 
}

/*  calculate the size of packed attr */
static 
size_t ooConcept_calc_attrsize(void)
{
    static ooAttr attr;
    static size_t s;

    s = sizeof(attr.operid);
    s += sizeof(attr.concid);
    s += sizeof(attr.relevance);

    if (DEBUG_LEVEL_3) {
	printf("Estimated attr size: %d\n", s);
    }
    return s;
}


/*  pack a single attribute into the bytecode */
static
int ooConcept_pack_attr(ooAttr *attr, char *buf)
{
    static size_t operid_size = sizeof(attr->operid);
    static size_t concid_size = sizeof(attr->concid);
    static size_t relevance_size = sizeof(attr->relevance);

    memcpy(buf, &attr->operid, operid_size);
    buf += operid_size;

    memcpy(buf, &attr->concid, concid_size);
    buf += concid_size;

    memcpy(buf, &attr->relevance, relevance_size);

    return oo_OK;
}


/*  pack all the attributes into the bytecode */
static
int ooConcept_pack_attrs(ooConcept *self, 
			      char *buf, 
		    unsigned short  num_attrs,
                            size_t  attr_size)
{   int i, ret;

    /* saving the actual number of valid attrs */
    memcpy(buf, &num_attrs, sizeof(unsigned short));
    buf +=  sizeof(unsigned short);

    for (i = 0; i < self->_num_attrs; i++) {
	if (!self->_attrs[i])
	    continue;
	ret = ooConcept_pack_attr(self->_attrs[i], buf);
	if (ret != oo_OK)
	    continue;
	buf += attr_size;
    }
    return oo_OK;
}

static
int ooConcept_pack_header(ooConcept *self, char *buf)
{ 
    static size_t uchar_size = sizeof(unsigned char);
    unsigned char conc_type, name_size;
    conc_type =  (unsigned char)self->type;
    memcpy(buf, &conc_type, uchar_size);

    name_size =  (unsigned char)self->name_size;
    buf += uchar_size;

    memcpy(buf, &name_size, uchar_size);
    buf += uchar_size;

    if (self->name_size)
	memcpy(buf, self->name, self->name_size);

    return oo_OK;
}


/*  serialize yourself as a DB record */
static 
int ooConcept_pack(struct ooConcept *self, pack_type pt)
{
    char *buf;
    size_t buf_size, header_size, num_attrs, attr_size;
    int ret;

    /* packing the Concept:
     *   1. concept type:            1 byte
     *   2. raw name string size:    1 byte
     *   3. raw name string bytes:   N bytes
     *   4. number of attrs:         2 bytes
     *   5. attrs:                   (N * attr_size) bytes
     */
    attr_size = ooConcept_calc_attrsize();
    num_attrs = ooConcept_calc_valid_attrs(self);

    header_size = sizeof(unsigned char) * 2 + self->name_size;
    buf_size =  header_size +
                sizeof(unsigned short) +
                (num_attrs * attr_size);

    buf = malloc(buf_size);
    if (!buf)	return oo_FAIL;

    ret = ooConcept_pack_header(self, buf);
    if (ret != oo_OK) return oo_FAIL;

    ret = ooConcept_pack_attrs(self, buf + header_size, 
		  (unsigned short)num_attrs, attr_size);
    if (ret != oo_OK) return oo_FAIL;

    /* TODO: pack the search indices */

    self->bytecode = buf;
    self->bytecode_size = buf_size;

    if (DEBUG_LEVEL_3) {
	printf("Concept %s   bytecode_size: %d\n",
          self->id, self->bytecode_size);
    }
    return oo_OK;
}




/*
 * Extract a single attribute 
 * and append it to the list self->_attrs.
 */
static
int ooConcept_unpack_attr(ooConcept *self, 
                         const char *buf, 
                             size_t  item_pos)
{
    oo_oper_type operid;
    mindmap_size_t concid;
    grade_t relevance = 0;
    bool  is_affirmed = true;
    char *ptr;
    ooAttr *newattr;

    ptr = (char*)buf;
    memcpy(&operid, ptr, sizeof(operid));
    ptr += sizeof(operid);

    memcpy(&concid, ptr,  sizeof(concid));
    ptr += sizeof(concid);

    memcpy(&relevance, ptr,  sizeof(relevance));

    /* allocating and initializing a new attribute */
    newattr = malloc(sizeof(*newattr));
    if (newattr == NULL) return oo_FAIL;

    newattr->operid = operid;
    newattr->concid = concid;
    newattr->is_affirmed = is_affirmed;
    newattr->baseclass = NULL;
    newattr->relevance = relevance;

    self->_attrs[item_pos] = newattr;

    return oo_OK;
}


/**
 *  Extracting the attributes from the packed bytecode 
 *  that was retrieved from the MindMap DB.
 */
static
int ooConcept_unpack_attrs(ooConcept *self, const char *buf)
{
    static ooAttr attr;
    size_t i, j, attr_array_size, num_attrs = 0;

    static size_t operid_size = sizeof(attr.operid);
    static size_t concid_size = sizeof(attr.concid);
    static size_t relevance_size = sizeof(attr.relevance);

    static size_t attr_ptr_size = sizeof(ooAttr*);
    static size_t packed_attr_size;

    packed_attr_size = operid_size + concid_size + relevance_size;

    memcpy(&num_attrs, buf, sizeof(unsigned short));

    /* create an array of attr pointers */
    attr_array_size = attr_ptr_size * num_attrs;
    self->_attrs = malloc(attr_array_size);
    if (!self->_attrs) return oo_NOMEM;


    /* initial position for reading the attributes */
    j = sizeof(unsigned short);

    if (DEBUG_LEVEL_3) {
	printf("Unpacking the attributes, total: %d\n", num_attrs);
    }
    for (i = 0; i < num_attrs; i++) {
	ooConcept_unpack_attr(self, &buf[j], i);
	j += packed_attr_size;
    }

    self->_num_attrs = num_attrs;

    return oo_OK;
}



/*  unpack the bytecode */
static
int ooConcept_unpack(ooConcept *self)
{
    unsigned char c;
    size_t i = 0;
    int ret;

    /*** unpack the header ***/
    /* concept type */
    c = (unsigned char)self->bytecode[i++];
    self->type = (conc_type)c;

    /* encoded name (NAME) */
    c = (unsigned char)self->bytecode[i++];
    self->name_size = (size_t)c;
    self->name = malloc(self->name_size + 1);
    if (!self->name)
	return oo_FAIL;

    memcpy(self->name, &self->bytecode[i], self->name_size);
    self->name[self->name_size] = '\0';
    i += self->name_size;

    if (DEBUG_LEVEL_3) {
	printf("Unpacking concept %s bytecode size: %d\n",
                 self->id, self->bytecode_size);
    }
    /*** unpack the attributes ***/
    ret = ooConcept_unpack_attrs(self, &self->bytecode[i]);
    if (ret != oo_OK)
	return oo_FAIL;

    self->bytecode = NULL;
    self->bytecode_size = 0;
    return oo_OK;
}

/* save concept as a MindMap DB record */
static
int ooConcept_put(struct ooConcept *self, DB *dbp)
{
    ooConcept *child_conc;
    DBT key, data;
    size_t i;
    int ret;
    static mindmap_size_t idsize = sizeof(mindmap_size_t);

    /* does this concept need saving? */
    if (!self->_is_modified)
	return oo_FAIL;

    if (DEBUG_LEVEL_1) {
	printf("Packing the concept \"%s\" [%s]...\n",
            self->name, self->id);
    }
    /* serialize yourself */
    ret = self->pack(self, PACK_COMPACT);
    if (ret != oo_OK)
	return oo_FAIL;

    /* initialize the DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* prepare the key: the Concept id */
    key.data = malloc(idsize);
    if (!key.data)
	return oo_FAIL;

    memcpy(key.data, &(self->numid), idsize);
    key.size  =  (u_int32_t)idsize;

    data.data  = self->bytecode;
    data.size  =  (u_int32_t)self->bytecode_size;

    ret = dbp->put(dbp, NULL, &key, &data, 0);
    if (ret != oo_OK)
	return oo_FAIL;

    /* now that the concept is saved 
     * switch the modification flag  */
    self->_is_modified = false;

    /* every attr should take care of saving its children */
    for (i = 0; i < self->_num_attrs; i++) {
	/* pass the deleted attribute */
	if (!self->_attrs[i])
	    continue;

	child_conc = self->_attrs[i]->baseclass;
	child_conc->put(child_conc, dbp);
    }

    /* free the resources */
    free(self->bytecode);
    self->bytecode_size = 0;

    return oo_OK;
}

/*  get an attribute */
static struct ooAttr*
ooConcept_getattr(struct ooConcept *self,
		  mindmap_size_t  concid)
{   size_t i;
    for (i = 0; i < self->_num_attrs; i++)  {
	if (self->_attrs[i] == NULL)
	    continue;
	if (self->_attrs[i]->concid == concid)
	    return self->_attrs[i];
    }
    return NULL;
}


/*  set attribute's value */
static
int ooConcept_setattr(struct ooConcept *self, 
		        mindmap_size_t  concid,
		      struct ooConcept *value )
{   ooAttr *attr;
    attr = ooConcept_getattr(self, concid);
    if (!attr) return oo_FAIL;

    /* TODO: check the subclass */

    attr->value = value;
    return oo_OK;
}


/*  allocate, initialize and assign a new attribute */
static int
ooConcept_initattr(struct ooConcept *self, 
		   oo_oper_type  operid,
		   struct ooConcept *attrconc, 
		   bool  is_affirmed,
		   unsigned short  relevance)
{   ooAttr *newattr;
    ooAttr **ptr;
    static size_t attr_size = sizeof(ooAttr);
    static size_t attr_ptr_size = sizeof(ooAttr*);
    size_t attr_array_size;

    newattr = malloc(attr_size);
    if (!newattr) return oo_FAIL;

    newattr->operid = operid;
    newattr->concid = attrconc->numid;
    newattr->is_affirmed = is_affirmed;
    newattr->baseclass = attrconc;
    newattr->relevance = relevance;

    /* TODO: find an empty slot in _attrs 
     * after deleted item */

    attr_array_size = attr_ptr_size * (self->_num_attrs + 1);
    ptr = (ooAttr**)realloc(self->_attrs, attr_array_size);
    if (!ptr) return oo_FAIL;

    self->_attrs = ptr;
    self->_attrs[self->_num_attrs] = newattr;
    self->_num_attrs++;
    self->_is_modified = true;
    return oo_OK;
}


/*  add a new attribute */
static
int ooConcept_newattr(struct ooConcept *self, 
                             oo_oper_type  operid,
		      struct ooConcept *attrconc, 
                                  bool  is_affirmed,
                        unsigned short  relevance)
{   int ret;
    if (DEBUG_LEVEL_3) {
	printf("Concept %s is adding a new attr \"%s\"\n",
	       self->id, attrconc->id);
    }
    /* check if this attribute is already set
     * to avoid loops  */
    if (self->getattr(self, attrconc->numid))
	return oo_FAIL;

    if (self->_num_attrs + 1 > ATTR_MAX)
	return oo_FAIL;

    ret = ooConcept_initattr(self, operid, attrconc,
                             is_affirmed, relevance);
    if (ret != oo_OK) return oo_FAIL;

    /* TODO: notify our parent's search engine */
    /*ret = ooConcept_notify(self, operid, attrconc); */

    /* set the inverted relation */
    return attrconc->newattr(attrconc, ~operid, self, 
                             is_affirmed, relevance);
}


/* removing an attribute */
static
int ooConcept_delattr(struct ooConcept *self, mindmap_size_t concid)
{
    size_t i;
    for (i = 0; i < self->_num_attrs; i++) {
	if (self->_attrs[i] == NULL)
	    continue;
	if (self->_attrs[i]->concid == concid) {
	    free(self->_attrs[i]);
	    self->_attrs[i] = NULL;
	    return oo_OK;
	}
    }
    return oo_FAIL;
}

/**
 * Find all our child concepts 
 * leading to the destination concept.
 * Guides are sorted by the length of paths.
 */
static
ooGuide* ooRefList_get_guides(struct ooRefList *self, 
                                mindmap_size_t  concid)
{   ooRefNode *ptr;
    /* trying to fetch a RefNode 
     * with a given Concept id */
    ptr = self->first;
    while (ptr != NULL) {
	if (ptr->concid == concid)
	    return ptr->guides;
	ptr = ptr->next;
    }
    return(NULL);
}


/** Ask the search engine about an attribute.
 *  Returns a list of guides to the destination concept.
 */
static
ooGuide* ooConcept_lookup(struct ooConcept *self,
			    mindmap_size_t  dest_concid)
{
    ooGuide *guides;

    /* guides = ooRefList_get_guides(self->_guides, dest_concid); */

    return NULL;
}



/*************************************
 *****     RefList methods     ******/

static
int ooRefNode_new(ooRefNode **refnode, 
	     mindmap_size_t  dest_concid,
	     mindmap_size_t  child_concid,
	     mindmap_size_t  path_length)
{
    ooRefNode *ptr;
    ooGuide *g;
    ptr = malloc(sizeof(ooRefNode));
    if (!ptr) return oo_FAIL;

    g = malloc(sizeof(ooGuide));
    if (g == NULL) return oo_FAIL;

    ptr->concid = dest_concid;
    g->concid = child_concid;
    g->len = path_length;
    ptr->guides = g;
    ptr->num_guides = 1;
    *refnode = ptr;
    return oo_OK;
}

/* append another direction to the destination concept */
static
int ooRefNode_add_guide(ooRefNode *refnode,
		   mindmap_size_t  child_concid,
                   mindmap_size_t  path_length)
{   ooGuide *ptr;
    ptr = realloc(refnode->guides,
	     sizeof(*ptr) * (refnode->num_guides + 1));
    if (ptr == NULL) return oo_FAIL;

    /*ptr[refnode->num_guides]->concid = child_concid;
    ptr[refnode->num_guides]->len = path_length;
    refnode->guides = ptr; */

    refnode->num_guides++;
    return oo_OK;
}


static
int ooRefNode_drop_guide(ooRefNode *refnode,
		    mindmap_size_t  child_concid)
{
    return oo_OK;
}

/**
 * adding a concept reference
 */
static
int ooRefList_add_ref(ooRefList *self,
	     mindmap_size_t dest_concid,
	     mindmap_size_t child_concid,
	     mindmap_size_t path_length)
{   ooRefNode *ptr;
    int ret;

    ptr = self->first;
    while (ptr != NULL) {
	if (ptr->concid != dest_concid) {
	    ptr = ptr->next;
	    continue;
	};
	/* gotcha! */
	ret = ooRefNode_add_guide(ptr, child_concid, path_length);
	if (ret != oo_OK) return oo_FAIL;
	return oo_OK;
    }

    /* no match was found,
     * adding a new RefNode to the end of the RefList */
    ret = ooRefNode_new(&ptr, dest_concid, child_concid, path_length);
    if (ret != oo_OK) return oo_FAIL;
    return oo_OK;
}


/**
 * removing a concept reference
 */
static
int ooRefList_drop_guide(struct ooRefList *self,
                     mindmap_size_t  concid,
                     mindmap_size_t  child_concid)
{   ooRefNode *ptr;
    int ret;

    /* trying to fetch a RefNode 
     * with a given Concept id */
    ptr = self->first;
    while (ptr) {
	if (ptr->concid != concid) {
	    ptr = ptr->next;
	    continue;
	}
	/* gotcha! */
	ret = ooRefNode_drop_guide(ptr, child_concid);
	if (ret != oo_OK) return oo_FAIL;
	return oo_OK;
    }
    return oo_FAIL;
}


static int
ooConcept_read_XML(struct ooConcept *self, 
		   xmlNode *input_node,
		   struct ooMindMap *mm,
		   struct ooDomain *domain)
{
    xmlNode *cur_node = NULL;
    xmlNode *deriv_node = NULL;
    struct ooConcept *c;
    int ret;

    /* required */
    ret = oo_copy_xmlattr(input_node, "name", &self->name, &self->name_size);
    if (ret != oo_OK) return ret;

    ret = oo_copy_xmlattr(input_node, "classid", &self->id, &self->id_size);
    if (ret != oo_OK) return ret;

    /* optional */
    /*ret = oo_copy_xmlattr(input_node, "annot", &self->annot, &self->annot_size);*/

    /*printf("...Reading concept \"%s\"...\n", self->name);*/

    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

        /* derivatives */
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"derivs"))) {
	    for (deriv_node = cur_node->children; 
		 deriv_node; 
		 deriv_node = deriv_node->next) {
		if (deriv_node->type != XML_ELEMENT_NODE) continue;

		if ((!xmlStrcmp(deriv_node->name, (const xmlChar *)"concept"))) {
		    ret = ooConcept_new(&c);
		    if (ret != oo_OK) return ret;

		    ret = c->read(c, deriv_node, mm, domain);
		    if (ret != oo_OK) {
			c->del(c);
			return ret;
		    }
		}

	    }
	}
    }

    ret = domain->add_concept(domain, self);
    if (ret != oo_OK) return ret;

    ret = mm->add_concept(mm, self);
    if (ret != oo_OK) return ret;


    return oo_OK;
}

/*  Concept Initializer */
extern int
ooConcept_init(struct ooConcept *self)
{
    self->numid = 0;

    self->id = NULL;
    self->id_size = 0;

    self->name = NULL;
    self->name_size = 0;

    self->domain = NULL;

    self->complexity = 0;

    self->annot = NULL;
    self->annot_size = NULL;

    self->bytecode = NULL;
    self->bytecode_size = 0;

    self->_is_modified = false;
    self->_num_attrs = 0;
    self->_attrs = NULL;

    /* initialize the plain search RefList
    self->_reflist = malloc(sizeof(ooRefList));
    if (!self->_reflist)
	return oo_FAIL;
    */

    /* initialize the BTree search index */
    /*
    NEW(ooBTree, self->_bt_index);
    if (ret != oo_OK)
	return oo_FAIL;
    */


    /* public methods */
    self->del = ooConcept_del;
    self->str = ooConcept_str;
    self->init = ooConcept_init;
    self->read = ooConcept_read_XML;

    self->pack = ooConcept_pack;
    self->unpack = ooConcept_unpack;

    self->put = ooConcept_put;

    self->newattr = ooConcept_newattr;
    self->delattr = ooConcept_delattr;
    self->setattr = ooConcept_setattr;
    self->getattr = ooConcept_getattr;

    self->lookup = ooConcept_lookup;

    return oo_OK;
}


/*  Concept Constructor */
extern int
ooConcept_new(struct ooConcept **conc)
{
    int ret;

    struct ooConcept *self = malloc(sizeof(struct ooConcept));
    if (!self) return oo_NOMEM;

    ret = ooConcept_init(self);

    *conc = self;
    return oo_OK;
}

