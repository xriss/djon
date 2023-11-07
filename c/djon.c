
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DJON_CODE 1
#include "djon.h"


int main(int argc, char *argv[])
{
//printf("START %d\n",argc);
	struct djon_state *it=djon_setup();
	if(argc>=2)
	{
		djon_load_file(it,argv[1]); // filename
	}
	else
	{
		djon_load_file(it,0); // stdin
	}
	djon_parse(it);

	it->fp=stdout;
	djon_print(it,it->parse_first,0);

	djon_clean(it);

	return(0);
}
