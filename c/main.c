
#include <stdio.h>
#include <stdlib.h>

#define DGAF_JSON_CODE 1
#include "dgaf_json.h"


int main()
{
	printf("Consume my json\n");
	
	struct dgaf_json_state *it=dgaf_json_setup();
	dgaf_json_load_file(it,"../test/json/good/y_object_long_strings.json");
	dgaf_json_parse(it);

	printf("\n\n%s\n",it->data);

	return(0);
}
