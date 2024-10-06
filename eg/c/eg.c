
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DJON_C 1
#include "djon.h"


// find index of = seperating a path from a value
// return -1 if no = is found
int str_find_end_of_path(char *str)
{
	int ret=-1;
	int idx=0;
	int mode=0;
	for( char *cp=str ; *cp ; cp++ )
	{
		switch(mode)
		{
			case '\'':
				if(*cp=='\'')
				{
					mode=0; // end string
				}
				else
				if( (cp[0]=='\\') && (cp[1]=='\'') )
				{
					cp++; // skip next char
				}
			break;
			case '"':
				if(*cp=='"')
				{
					mode=0; // end string
				}
				else
				if( (cp[0]=='\\') && (cp[1]=='"') )
				{
					cp++; // skip next char
				}
			break;
			default:
				if(*cp=='=' ) { ret=idx; break; }
				else
				if(*cp=='\'') { mode='\''; }
				else
				if(*cp=='"') { mode='"'; }
			break;
		}
		idx++;
	}
	return ret;
}

int main(int argc, char *argv[])
{
	int error_code=20;
	
	// sanity incase of insane future
	if(sizeof **argv != 1) { printf("invalid universe, char must be 1 byte!\n"); return 20; }

	int checkopts;
	FILE *fp=0;

	int idx;
	char *fname="eg.djon";
	char *cp;

	checkopts=1;
	for( int i=1 ; i<argc ; i++ )
	{
		char *cp=argv[i];
		if( checkopts && ( cp[0]=='-' && cp[1]=='-' ) ) // option
		{
			if( 0==strcmp(cp,"--") )
			{
				checkopts=0;
			}
			else
			if( 0==strncmp(cp,"--file=",7) )
			{
				fname=cp+7;
			}
			else
			if( 0==strcmp(cp,"--help") )
			{
				printf("\n\
eg is djon C example code\n\
\n\
	--file     : filename of djon file to load and save\n\
\n\
	path.path[path]\n\
		print the value of this path\n\
\n\
	path.path[path]=value\n\
		set the value of this path\n\
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
	}
	

	printf("Loading %s\n",fname);
	djon_state *ds=djon_setup();
	djon_load_file(ds,fname);
	if( ds->error_string ){ goto error; }

	
	djon_parse(ds);
	if( ds->error_string ){ goto error; }
	

	if(ds->parse_value) // good read
	{

// now we can set/print from command line
		checkopts=1;
		for( int i=1 ; i<argc ; i++ )
		{
			cp=argv[i];
			if( checkopts && ( cp[0]=='-' && cp[1]=='-' ) ) // option
			{
				if( 0==strcmp(cp,"--") )
				{
					checkopts=0;
					cp=0;
				}
				else
				if( 0==strncmp(cp,"--",2) ) // ignore args that start with --
				{
					cp=0;
				}
			}
			if(cp) // a set or get depending on presence of an = in the string
			{
				int vi;
				const char *lastkey;
				char *path=cp;
				char *value="";
				char buff[256];
				int eop=str_find_end_of_path(cp);
				if(eop>=0) // this is a set
				{
					cp[eop]=0;
					value=cp+eop+1;
					printf("setting %s\n",path);
					int pi=djon_value_by_path(ds,ds->parse_value,path,&lastkey); // get last parent
					if( ds->error_string ){ goto error; }

					vi=djon_value_newkey(ds,pi,0,lastkey);
					if( ds->error_string ){ goto error; }
					djon_value_set(ds,vi,DJON_STRING,0,0,value);
					if( ds->error_string ){ goto error; }
				}
				else // this is a get and print
				{
					printf("getting %s\n",path);
					vi=djon_value_by_path(ds,ds->parse_value,path,0);
					if( ds->error_string ){ goto error; }
				}
				if(!vi)
				{
					printf("\tno value found at path\n");
				}
				else
				{
					djon_value_copy_str(ds,vi,buff,sizeof buff);
					value=buff; // build value in buff
					printf("\t=\t\"%s\"\n",value);
				}
			}
		}

		printf("Saving %s\n",fname);
		fp=fopen(fname,"wb");
		if(!fp)
		{
			djon_set_error(ds,"output file error");
			goto error;
		}
		ds->fp=fp;


		djon_write_djon(ds,ds->parse_value);
		if( ds->error_string ){ goto error; }
	}
	
	error_code=0; // not an error
error:
	if(fp)
	{
		fclose(fp); fp=0;
	}
	if(ds)
	{
		if( ds->error_string )
		{
			fprintf(stderr,"%s\n",ds->error_string);
			fprintf(stderr,"line %d char %d byte %d\n",ds->error_line,ds->error_char,ds->error_idx);
		}
		djon_clean(ds); ds=0;
	}
	return error_code;
}
