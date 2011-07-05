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
 *   ------
 *   main.c
 *   interaction shell 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

/* win options not needed under Linux */
#if defined(_WIN32) || defined(WIN32)
#define NO_BUILD_DLL
#endif


#include "ooconfig.h"
#include "oomnik.h"

static const char *options_string = "c:h?";

static struct option main_options[] =
{
    {"config", 1, NULL, 'c'},
    {"help", 0, NULL, 'h'},
    { NULL, 0, NULL, 0 }
};

void display_usage(void);

void display_usage(void)
{
    fprintf(stderr, "\nUsage: oomnik --config=path_to_your_oomniconf_xml\n\n");
}

/******************* MAIN ***************************/

int main(int argc, char *argv[])
{
    struct OOmnik *oom;
    const char *config = "oomniconf.xml";
    int long_option;
    int opt;
    
    while((opt = getopt_long(argc, argv, 
			     options_string, main_options, &long_option)) >= 0) {
	switch(opt)
	{
	case 'c':
	    if (optarg) {
		config = optarg;
		printf("%s\n", optarg);
	    }
	    break;
	case 'h':
	case '?':
	    display_usage();
	    break;
	case 0:  /* long option without a short arg */
	    if(!strcmp("config", main_options[long_option].name)) {
		config = optarg;
	    }
	    break;
	default:
	    break;
	}
    }

    oom = (struct OOmnik*)OOmnik_create(config);
    if (!oom) {
	display_usage();
	exit(-2);
    }
    oom->interact(oom);
    oom->del(oom);

    exit (0);

}
