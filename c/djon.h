
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef DJON_H
#define DJON_H

typedef enum djon_enum
{
	NONE,
	NUMBER,
	BOOL,
	STRING,
	ARRAY,
	OBJECT,
} djon_enum ;

typedef struct djon_value
{
	int       nxt; // idx to next value if this is an object key/value or array
	char    * str; // start of string ( offset into the input json string )
	int       len; // number of characters

	djon_enum typ; // the type of data contained in the string
	double    num; // number or bool value
	int       nam; // linked list of names for object
	int       val; // linked list of values for object or array

} djon_value ;

typedef struct djon_state
{
	// input data string
	char *data;
	int   data_len;

	// output values
	struct djon_value * values;
	int values_len;
	int values_siz;

	// current parse state
	int parse_idx;
	int parse_first; // first output value
	char *parse_stack; // HAXTBH remember starting stack so we can play chicken

	char *error_string; // if this is not 0 we have an error
	int error_char;
	int error_line;
	
	FILE *fp; // where to print output

	char      buf[256];
	
} djon_state ;

extern struct djon_state * djon_setup();
extern struct djon_value * djon_get(struct djon_state *it,int idx);
extern int djon_load_file(struct djon_state *it,char *fname);
extern int djon_parse(struct djon_state *it);

int djon_alloc(struct djon_state *it);
int djon_free(struct djon_state *it,int idx);
int djon_parse_value(struct djon_state *it);
int djon_check_stack(struct djon_state *it);

#define DJON_IS_WHITE(c) ( ((c)==' ') || ((c)=='\t') || ((c)=='\n') || ((c)=='\r') || ((c)=='\v') || ((c)=='\f') )
#define DJON_IS_PUNCT(c) ( ( ((c)>='!') && ((c)<='/') ) || ( ((c)>=':') && ((c)<='@') ) || ( ((c)>='[') && ((c)<='`') ) || ( ((c)>='{') && ((c)<='~') ) )


#endif


#ifdef DJON_C

// locales are a cursed so we need our own basic functions ?
// strtod replacement ?
double djon_str_to_double(char *cps,char **endptr)
{
	const double inf = 1.0/0.0;
	const double nan = 0.0/0.0;
	
	int gotdata=0;

	char c;
	char *cp=cps;

	c=*cp;
	double sign=1.0;
	if(c=='-') { sign=-1.0; cp++; } // maybe negative
	else
	if(c=='+') { cp++; } // allow
	
	double d=0.0;
	for( c=*cp ; (c>='0') && (c<='9') ; c=*++cp )// 0 or more integer parts
	{
		d=(d*10.0)+(c-'0');
		gotdata++;
	}

	double m=1.0;
	if( *cp=='.' ) // a decimal point
	{
//		gotdata=0; // reset
		cp++;
		for( c=*cp ; (c>='0') && (c<='9') ; c=*++cp )// and 0 or more integer parts
		{
			m=m*0.1;
			d+=((c-'0')*m);
//			gotdata++;
		}
//		if(gotdata==0){goto error;} // require some numbers
	}

	if(gotdata==0){goto error;} // require some numbers before or after decimal point

	c=*cp;
	if( (c=='e') || (c=='E') )
	{
		cp++;
		c=*cp;

		gotdata=0; // reset

		double esign=1.0;
		if(c=='-') { esign=-1.0; cp++; } // maybe negative
		else
		if(c=='+') { cp++; } // allow

		double e=0.0;
		for( c=*cp ; (c>='0') && (c<='9') ; c=*++cp )// and 0 or more exponent parts
		{
			m=m*0.1;
			e=(e*10.0)+(c-'0');
			gotdata++;
		}
		d*=pow(10.0,e*esign); // apply exponent

		if(gotdata==0){goto error;} // require some numbers after the e
	}
	
	d*=sign; // apply sign

	if(endptr){*endptr=cp;} // we used this many chars
	return d; // and parsed this number

error:
	if(endptr){*endptr=cps;} // 0 chars used
	return nan;
}

// write into buf, return length of string, maximum 32 including null
int djon_double_to_str(double num,char buf[32] )
{
//printf("n %g\n",num);

	char *cp=buf;

	if( isnan(num) ) // A nan , so we write null
	{
		*cp++='n';
		*cp++='u';
		*cp++='l';
		*cp++='l';
		*cp=0; // null
		return cp-buf;
	}

	if( signbit(num) )
	{
		*cp++='-';
		num=-num; // remove sign
	}

	if(num==0.0) // zero
	{
		*cp++='0';
		*cp=0; // null
		return cp-buf;
	}

	if(isinf(num)) //inf
	{
		*cp++='9';
		*cp++='e';
		*cp++='9';
		*cp++='9';
		*cp++='9';
		*cp=0; // null
		return cp-buf;
	}
	
	int e=(int)floor(log10(num)); // find exponent
//printf("n %g\n",num);
//printf("e %d\n",e);

	int i;
	int d;

	int digits=15;
	if( (e<0) && (e>=-8) ) // special case when close to 0 dont add an e until we get really small
	{
		digits=15+1-e;
		d=1.0; // we want a 0.00000001 sort of number when it is near 0
		e=0;
	}
	
	double t=pow(10.0,(double)e); // divide by this to get current decimal
	double r=pow(10.0,(double)e-digits); // rounding error
	if(e>0)
	{
		e=e+1-digits; // the e we will be when we print all the digits
		if(e<0) { e=0; } // goes from +e to -e so force 0
	}
	if(e<0) // start with a . not a digit when the number goes tiny.
	{
		e=e+1;
		*cp++='.';
	}
	int z=0; // run of zeros we will want to undo
	for(i=0;i<digits;i++) // probably 15 digits
	{
		if((t>0.09)&&(t<0.11)) { *cp++='.'; z=1; } // decimal point, reset out count of zeros
		d=(int)((num+r)/t); //auto floor converting to int with a tiny roundup
		num-=((double)d)*t;
		t=t/10.0; // next digit
		if(d==0) { z++; } else { z=0; } // count zeros at end
		*cp++='0'+d;
	}

	if(z>0) // undo trailing zeros
	{
		cp=cp-z;
		if(e>=0)
		{
			e=e+z; // possible new e if the number was huge, might force this to 0 later
		}
	}
	if( (e>=0) && (t<0.1) ) { e=0; } // we only removed zeros after the . so e should be 0

	if(e!=0)
	{
		*cp++='e';
		if(e<0)
		{
			*cp++='-';
			e=-e;
		}
		for( i= (e>=100) ? 100 : (e>=10) ? 10 : 1 ; i>=1 ; i=i/10 )
		{
			d=e/i;
			e-=d*i;
			*cp++='0'+d;
		}
	}
	
	*cp=0; // null
	return cp-buf;
}

double djon_str_to_hex(char *cps,char **endptr)
{
	const double inf = 1.0/0.0;
	const double nan = 0.0/0.0;

	return 0;
}

double djon_str_to_number(char *cp,char **endptr)
{
	const double inf = 1.0/0.0;
	const double nan = 0.0/0.0;

	if	(
			( (cp[0]=='0') && ( (cp[1]=='x') || (cp[1]=='X') ) )
			||
			( ( (cp[0]=='+') || (cp[0]=='-') ) && (cp[1]=='0') && ( (cp[2]=='x') || (cp[2]=='X') ) )
		)
	{
		return djon_str_to_hex(cp,endptr);
	}
	else
	{
		return djon_str_to_double(cp,endptr);
	}
	return nan;
}

// allocate a new parsing state
struct djon_state * djon_setup()
{
	struct djon_state *it;
	
	it=(struct djon_state *)malloc( sizeof(struct djon_state) );
	if(!it) { return 0; }
	
	it->data=0;
	it->data_len=0;
	it->parse_stack=0;

	it->values_len=1; // first value is used as a null so start at 1
	it->values_siz=16384;
	it->values=(struct djon_value *)malloc( it->values_siz * sizeof(struct djon_value) );
	if(!it->values) { free(it); return 0; }

	return it;
}
// free a new parsing state
void djon_clean(struct djon_state *it)
{
	if(!it) { return; }
	if(it->data) { free(it->data); }
	if(it->values) { free(it->values); }
}

// get a value by index
struct djon_value * djon_get(struct djon_state *it,int idx)
{
	if( idx <= 0 )              { return 0; }
	if( idx >= it->values_siz ) { return 0; }
	return it->values + idx ;
}

// unallocate unused values at the end of a parse
void djon_shrink(struct djon_state *it)
{
	struct djon_value * v;
	v=(struct djon_value *)realloc( (void*)it->values , (it->values_len) * sizeof(struct djon_value) );
	if(!v) { return; } //  fail but we can ignore
	it->values_siz=it->values_len;
}

// allocate a new value and return its index, 0 on error
int djon_alloc(struct djon_state *it)
{	
	struct djon_value * v;
	if( it->values_len+1 >= it->values_siz ) // check for space
	{
		v=(struct djon_value *)realloc( (void*)it->values , (it->values_siz+16384) * sizeof(struct djon_value) );
		if(!v) { return 0; }
		it->values_siz=it->values_siz+16384;
		it->values=v; // might change pointer
	}
	v=djon_get(it,it->values_len);
	v->nxt=0;
	v->str=0;
	v->len=0;
	v->typ=NONE;
	v->num=0.0;
	v->nam=0;
	v->val=0;
		
	return it->values_len++;
}

// we can only free the top most allocated idx, return 0 if not freed
int djon_free(struct djon_state *it,int idx)
{
	if( idx == (it->values_len-1) )
	{
		it->values_len--;
		return idx;
	}
	return 0;
}

int djon_print_indent(struct djon_state *it,int indent)
{
	if(indent<0) { return -indent; } // skip first indent
	else
	{
		int i;
		for( i=0 ; i<indent ; i++)
		{
			putc(' ',it->fp);
		}
	}
	return indent;
}
// debug print a value
void djon_print(struct djon_state *it,int idx,int indent)
{
	struct djon_value *nxt=djon_get(it,idx);
	struct djon_value *nam;
	struct djon_value *val;
	int nam_idx;
	int val_idx;
	int len;
	if(nxt)
	{
		if(nxt->typ==ARRAY)
		{
//			if(indent<0)
//			{
//				indent=djon_print_indent(it,indent);
//				printf("\n");
//			}
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"[\n");
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(val)
			{
				djon_print(it,val_idx,indent+1);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"]\n");
		}
		else
		if(nxt->typ==OBJECT)
		{
//			if(indent<0)
//			{
//				indent=djon_print_indent(it,indent);
//				fprintf(it->fp,"\n");
//			}
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"{\n");
			nam_idx=nxt->nam; nam=djon_get(it,nam_idx);
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(nam&&val)
			{
				indent=djon_print_indent(it,indent+1)-1;
				fprintf(it->fp,"%.*s = ",nam->len,nam->str);
				djon_print(it,val_idx,-(indent+1));
				nam_idx=nam->nxt; nam=djon_get(it,nam_idx);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"}\n");
		}
		else
		if(nxt->typ==STRING)
		{
			indent=djon_print_indent(it,indent);
			fwrite(nxt->str, 1, nxt->len, it->fp);
			fwrite("\n", 1, 1, it->fp);
		}
		else
		if(nxt->typ==NUMBER)
		{
			indent=djon_print_indent(it,indent);
			len=djon_double_to_str(nxt->num,it->buf);
			fwrite(it->buf, 1, len, it->fp);
			fwrite("\n", 1, 1, it->fp);
//			fprintf(it->fp,"%g\n",nxt->num);
		}
		else
		if(nxt->typ==BOOL)
		{
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"%s\n",nxt->num?"TRUE":"FALSE");
		}
		else
		if(nxt->typ==NONE)
		{
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"%s\n","NULL");
		}
		else
		{
			indent=djon_print_indent(it,indent);
			fprintf(it->fp,"%s\n","UNDEFINED");
		}
	}
}

// load a new file or read from stdin
int djon_load_file(struct djon_state *it,char *fname)
{
	const int chunk=0x10000; // read this much at once

	FILE * fp;
    char * temp;
    char * data=0;

    int size=0;
    int used=0;

	it->data="";
	it->data_len=0;

	// first alloc
	size = used+chunk;
	data = malloc(size); if(!data) { goto error; }

	// open file or use stdin
	if(fname)
	{
		fp = fopen( fname , "rb" ); if(!fp) { goto error; }
	}
	else
	{
		fp=stdin;
	}

    while( !feof(fp) ) // read all file
    {
		// extend buffer
		while(used+chunk > size)
		{
			temp = realloc(data, used+chunk); if(!temp) { goto error; }
			size = used+chunk;
			data=temp;			
		}
		used += fread( data+used , 1 , chunk, fp );		
		if(ferror(fp)) { goto error; }
    }


	size = used+1; // this may size up or down
	temp = realloc(data, size); if(!temp) { goto error; }
    data = temp;
    data[used] = 0; // null term

	it->data=data;
	it->data_len=used;

    return 1;

error:
	if(fname)
	{
		if(fp) { fclose(fp); }
	}
	if(data) { free(data); }
    return 0;
}

// peek for whitespace or comments
int djon_peek_white(struct djon_state *it)
{
	char c1=it->data[ it->parse_idx ];
	char c2=it->data[ it->parse_idx+1 ];
	if( DJON_IS_WHITE(c1) )
	{
		return 1;
	}
	else
	if( (c1=='/') && (c2=='/'))
	{
		return 1;
	}
	else
	if( (c1=='/') && (c2=='*'))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// peek for punct
int djon_peek_punct(struct djon_state *it,char *punct)
{
	char c=it->data[ it->parse_idx ];
	char *cp;
	for( cp=punct ; *cp ; cp++ )
	{
		if( c==*cp ) // a punctuation char
		{
			return 1;
		}
	}
	return 0;
}

// peek for a lowercase string eg, true false null
int djon_peek_string(struct djon_state *it,char *s)
{
	char *sp;
	char *dp;
	char d;
	for( sp=s , dp=it->data+it->parse_idx ; *sp && *dp ; sp++ , dp++ )
	{
		d=*dp;
		if( d>='A' && d<='Z' ) { d=d+('a'-'A'); } // lowercase
		if( d != *sp ) { return 0; } // no match
	}
	if( *sp==0 ) // we got to end of test string
	{
		d=*dp; // and text must be followed by some kind of terminator
		if( DJON_IS_WHITE(d) || DJON_IS_PUNCT(d) || ( d==0 ) )
		{
			return 1;
		}
	}
	return 0;
}

// skip whitespace and comments
void djon_skip_white(struct djon_state *it)
{
	while( it->parse_idx < it->data_len )
	{
		char c1=it->data[ it->parse_idx ];
		char c2=it->data[ it->parse_idx+1 ];
		if( (c1==' ') || (c1=='\t') || (c1=='\n') || (c1=='\r') )
		{
			it->parse_idx++;
		}
		else
		if( (c1=='/') && (c2=='/'))
		{
			it->parse_idx+=2;
			while( it->parse_idx < it->data_len )
			{
				c1=it->data[ it->parse_idx ];
				if( (c1=='\n') )
				{
					it->parse_idx++;
					break;
				}
				else
				{
					it->parse_idx++;
				}
			}
		}
		else
		if( (c1=='/') && (c2=='*'))
		{
			it->parse_idx+=2;
			while( it->parse_idx < it->data_len )
			{
				c1=it->data[ it->parse_idx ];
				c2=it->data[ it->parse_idx+1 ];
				if( (c1=='*') || (c2=='/') )
				{
					it->parse_idx+=2;
					break;
				}
				else
				{
					it->parse_idx++;
				}
			}
		}
		else
		{
			return;
		}
	}
}

// skip a patch of these punct chars sandwiched by white space , counting how many punct chars we found.
int djon_skip_white_punct(struct djon_state *it,char *punct)
{
	djon_skip_white(it);

	char *cp;
	int ret=0;
	if( it->parse_idx < it->data_len )
	{
		char c=it->data[ it->parse_idx ];
		for( cp=punct ; *cp ; cp++ )
		{
			if( c==*cp ) // a punctuation char we wish to ignore
			{
				it->parse_idx++;
				ret++; // count punctuation
				break;
			}
		}
	}
	
	djon_skip_white(it);

	return ret;
}

// advance one char allocate a temporary string value and return it
int djon_parse_step(struct djon_state *it)
{
	if( it->parse_idx >= it->data_len ) { return 0; } // EOF

	struct djon_value *nxt;
	int idx=0;

	idx=djon_alloc(it);
	nxt=djon_get(it,idx);
	if(nxt==0) { return 0; }
	nxt->typ=STRING;
	nxt->str=it->data+it->parse_idx;
	nxt->len=1;
	it->parse_idx++;
	return idx;
}

int djon_parse_string(struct djon_state *it,int lst_idx,char *term)
{
	struct djon_value *lst=djon_get(it,lst_idx);
	if(lst==0) { return 0; }

	char c;
	char *cp;

	while( it->parse_idx < it->data_len ) // while data
	{
		lst->len++; // grow string
		c=it->data[ it->parse_idx++ ]; // get next char

		for( cp=term ; *cp ; cp++ )
		{
			if( c==*cp ) // a termination char
			{
				return lst_idx;
			}
		}

	}

	return lst_idx;
}

int djon_parse_rawstring(struct djon_state *it,int lst_idx)
{
	return djon_parse_string(it,lst_idx,"`");
}

int djon_parse_number(struct djon_state *it,int lst_idx)
{
	struct djon_value *lst=djon_get(it,lst_idx);

	char *cps=lst->str;
	char *cpe;

//	double d=strtod(cps,&cpe);
	double d=djon_str_to_number(cps,&cpe);
	int len=cpe-cps;
	if( len > 0 )
	{
		it->parse_idx+=len-lst->len;

		lst->typ=NUMBER;
		lst->num=d;
		
		return lst_idx;
	}
	return 0;
}

int djon_parse_name(struct djon_state *it)
{
	djon_skip_white(it);

	int lst_idx=djon_parse_step(it);
	struct djon_value *lst=djon_get(it,lst_idx);
	if(lst==0){return 0;} // out of data

	char c;
	char *cp;

	while( it->parse_idx < it->data_len ) // while data
	{
		if( djon_peek_white(it) ) { return lst_idx; } // stop at whitespace
		if( djon_peek_punct(it,"=:") ) { return lst_idx; } // stop at punct

		lst->len++; // grow string
		c=it->data[ it->parse_idx++ ]; // get next char
		if(c=='"') { return lst_idx; } // end on closing quote
	}

	return lst_idx;
}

int djon_parse_object(struct djon_state *it,int lst_idx)
{
	struct djon_value *lst=djon_get(it,lst_idx);
	struct djon_value *nam;
	struct djon_value *val;

	lst->typ=OBJECT;

	int nam_idx;
	int val_idx;

	while(1)
	{
		djon_skip_white(it);
		if( it->data[it->parse_idx]=='}' ) { it->parse_idx++;  return lst_idx; } // found closer

		nam_idx=djon_parse_name(it);
		if(!nam_idx) { return 0; } // no value
		if( djon_skip_white_punct(it,"=:") != 1 ) { return 0; } // required assignment
		val_idx=djon_parse_value(it); if(!val_idx){return 0;}
		djon_skip_white_punct(it,",;"); // optional , seperators

		if( lst->nam==0) // first
		{
			lst->nam=nam_idx;
			lst->val=val_idx;
			nam=djon_get(it,nam_idx);
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			nam->nxt=nam_idx;
			val->nxt=val_idx;
			nam=djon_get(it,nam_idx);
			val=djon_get(it,val_idx);
		}
	}
	
	return lst_idx;
}

int djon_parse_array(struct djon_state *it,int lst_idx)
{
	struct djon_value *lst=djon_get(it,lst_idx);
	struct djon_value *val;

	lst->typ=ARRAY;

	int val_idx;

	while(1)
	{
		djon_skip_white(it);
		if( it->data[it->parse_idx]==']' ) { it->parse_idx++; return lst_idx; } // found closer

		val_idx=djon_parse_value(it);
		if(!val_idx) { return 0; } // no value
		djon_skip_white_punct(it,",;"); // optional , separators

		if( lst->val==0) // first
		{
			lst->val=val_idx;
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			val->nxt=val_idx;
			val=djon_get(it,val_idx);
		}
	}
	
	return lst_idx;
}

int djon_parse_value(struct djon_state *it)
{
	int idx;
	struct djon_value *nxt;
	
	if(!djon_check_stack(it)){ return 0; }

	djon_skip_white(it);

// check for special strings lowercase only
	if( djon_peek_string(it,"true" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->typ=BOOL;
		nxt->num=1.0;
		it->parse_idx+=4;
		return idx;
	}
	else
	if( djon_peek_string(it,"false" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->typ=BOOL;
		nxt->num=0.0;
		it->parse_idx+=5;
		return idx;
	}
	else
	if( djon_peek_string(it,"null" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->typ=NONE;
		it->parse_idx+=4;
		return idx;
	}
// amd a quick check for closers before we allocate anything
	if( it->data[it->parse_idx]=='}' ) { it->parse_idx++; return 0; }
	else
	if( it->data[it->parse_idx]==']' ) { it->parse_idx++; return 0; }

	idx=djon_parse_step(it);
	nxt=djon_get(it,idx);
	if(nxt==0){return 0;}
	char c=*(nxt->str);

	switch( c )
	{
		case '{' :
			return djon_parse_object(it,idx);
		break;
		case '[' :
			return djon_parse_array(it,idx);
		break;
		case '\'' :
			return djon_parse_string(it,idx,"'");
		break;
		case '"' :
			return djon_parse_string(it,idx,"\"");
		break;
		case '`' :
			return djon_parse_rawstring(it,idx);
		case '0' :
		case '1' :
		case '2' :
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
		case '8' :
		case '9' :
		case '.' :
		case '+' :
		case '-' :
			return djon_parse_number(it,idx);
		break;
	}
	return djon_parse_string(it,idx,"\n");
}


// allocate a new value and return its index, 0 on error
int djon_check_stack(struct djon_state *it)
{
	int stack=0xdeadbeef;
	if(it->parse_stack) // check stack flag
	{
		int stacksize = it->parse_stack - ((char*)&stack); // oh yeah stack grows backwards
		if ( stacksize > (256*1024) ) // 256k stack burper, give up if we use too much
		{
			printf("Stack overflow %d\n",stacksize);
			return 0;
		}
	}
	return 1;
}
int djon_parse(struct djon_state *it)
{
	int stack=0xdeadbeef;
	it->parse_stack=(char*)&stack;
	it->parse_idx=0;
	it->parse_first=0;

	it->error_string=0;
	it->error_char=0;
	it->error_line=0;

	int idx;
	struct djon_value *nxt;
	while( idx=djon_parse_value(it) )
	{
		if(it->parse_first==0) // remember the first value
		{
			it->parse_first=idx;
		}
		else
		if(nxt) // multiple values are linked as an array
		{
			nxt->nxt=idx;
		}
		nxt=djon_get(it,idx);
		if(nxt==0){	goto error; }
	}
	djon_shrink(it);
	return it->parse_first;
error:
	it->parse_stack=0;
	return it->parse_first;
}

#endif

