
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DJON_C 1
#include "djon.h"


int main(int argc, char *argv[])
{
//printf("START %d\n",argc);
	djon_state *it=djon_setup();
	if(argc>=2)
	{
		djon_load_file(it,argv[1]); // filename
	}
	else
	{
		djon_load_file(it,0); // stdin
	}
	if( it->error_string ){ goto error; }
	djon_parse(it);
	if( it->error_string ){ goto error; }
	
	djon_value *v=djon_get(it,it->parse_first);
	if(v && v->nxt)
	{
		it->parse_idx=v->str-it->data; // fake idx
		djon_set_error(it,"multiple root values");
		goto error;
	}
	it->fp=stdout;
	djon_print(it,it->parse_first,0);

	djon_clean(it);

	return 0;
error:
	if( it->error_string )
	{
		fprintf(stderr,"%s\n",it->error_string);
		fprintf(stderr,"line %d char %d (%d)\n",it->error_line,it->error_char,it->error_idx);
	}
	return 20;
}
