
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DGAF_JSON_CODE 1
#include "dgaf_json.h"


int main(int argc, char *argv[])
{
//printf("START %d\n",argc);
	struct dgaf_json_state *it=dgaf_json_setup();
	if(argc>=2)
	{
		dgaf_json_load_file(it,argv[1]); // filename
	}
	else
	{
		dgaf_json_load_file(it,0); // stdin
	}
	dgaf_json_parse(it);

	it->fp=stdout;
	dgaf_json_print(it,it->parse_first,0);

	dgaf_json_clean(it);

	return(0);
}
