
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DJON_C 1
#include "djon.h"


int main(int argc, char *argv[])
{
//printf("START %d\n",argc);

	FILE *fp=0;
	int checkopts=1;
	int write_djon=0;
	char *fname1=0;
	char *fname2=0;
	int i;
	char *cp;
	for( i=1 ; i<argc ; i++ )
	{
		cp=argv[i];
		if( checkopts && ( cp[0]=='-' && cp[1]=='-' ) ) // option
		{
			if( 0==strcmp(cp,"--") )
			{
				checkopts=0;
			}
			else
			if( 0==strcmp(cp,"--djon") )
			{
				write_djon=1;
			}
			else
			if( 0==strcmp(cp,"--json") )
			{
				write_djon=0;
			}
			else
			if( 0==strcmp(cp,"--help") )
			{
				printf("\n\
djon input.filename output.filename\n\
\n\
	If no output.filename then write to stdout\n\
	If no input.filename then read from stdin\n\
\n\
Possible options are:\n\
\n\
	--djon : output djon format\n\
	--json : output json format\n\
	--     : stop parsing options\n\
\n\
");
				return 0;
			}
			else
			{
				fprintf(stderr,"Unknown option %s\n",cp);
				return 20;
			}
		}
		else // filename
		{
			if(!fname1) { fname1=cp; }
			else
			if(!fname2) { fname2=cp; }
			else
			{
				fprintf(stderr,"Unknown option %s\n",cp);
				return 20;
			}
		}
	}
	

	djon_state *it=djon_setup();
	if(fname1)
	{
		djon_load_file(it,fname1); // filename
	}
	else
	{
		djon_load_file(it,0); // stdin
	}
	if( it->error_string ){ goto error; }

	
	djon_parse(it);
	
	djon_value *v=djon_get(it,it->parse_first);
	if(v && v->nxt)
	{
		v=djon_get(it,v->nxt);
		if(v) { it->parse_idx=v->str-it->data; }
		djon_set_error(it,"multiple root values");
	}
	else
	if(!v)
	{
		djon_set_error(it,"no data");
	}

	if(fname2)
	{
		fp=fopen(fname2,"wb");
		if(!fp)
		{
			djon_set_error(it,"output file error");
			goto error;
		}
		it->fp=fp;
	}
	else
	{
		it->fp=stdout;
	}
	i=it->parse_first;
	while( i )
	{
		if(write_djon)
		{
			djon_write_djon(it,i,0);
		}
		else
		{
			djon_write_json(it,i,0);
		}
		v=djon_get(it,i);
		i=v?v->nxt:0;
	}

	if( it->error_string ){ goto error; }

	if(fp)
	{
		fclose(fp);
	}
	djon_clean(it);

	return 0;
error:
	if( it->error_string )
	{
		fprintf(stderr,"%s\n",it->error_string);
		fprintf(stderr,"line %d char %d byte %d\n",it->error_line,it->error_char,it->error_idx);
	}
	if(fp)
	{
		fclose(fp);
	}
	if(it)
	{
		djon_clean(it);
	}
	return 20;
}
