
#include <stdio.h>
#include <stdlib.h>

#define DGAF_JSON_CODE 1
#include "dgaf_json.h"


int main(int argc, char *argv[])
{
	if(argc>=2)
	{
		struct dgaf_json_state *it=dgaf_json_setup();
		dgaf_json_load_file(it,argv[1]);
		dgaf_json_parse(it);
		dgaf_json_print(it,it->parse_first,0);
		printf("\n\n%s\n",it->data);
	}
	return(0);
}
