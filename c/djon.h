
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef DJON_H
#define DJON_H

typedef enum djon_enum
{
	DJON_NULL     = 0x0001,
	DJON_NUMBER   = 0x0002,
	DJON_BOOL     = 0x0003,
	DJON_STRING   = 0x0004,
	DJON_ARRAY    = 0x0005,
	DJON_OBJECT   = 0x0006,
	DJON_COMMENT  = 0x0007,
	
	DJON_ESCAPED  = 0x0100, // this string contains \n \t \" etc
	DJON_KEY      = 0x0200, // this string is a key
	
	DJON_TYPEMASK = 0x00ff, // base types are in lower byte
	DJON_FLAGMASK = 0xff00, // additional flags are in upper byte

	DJON_MASK     = 0xFFFF,
} djon_enum ;

typedef struct djon_value
{
	djon_enum typ; // the type of data contained in the string

	int       nxt; // idx to next value if this is an object key/value or array
	char    * str; // start of string ( points into the input json string )
	int       len; // number of characters

	double    num; // number or bool value
	int       key; // linked list of keys for object
	int       val; // linked list of values for object or array
	int       com; // linked list of comments for this value

} djon_value ;

typedef struct djon_state
{
	// input data string
	char *data;
	int   data_len;

	// output values
	djon_value * values;
	int          values_len; // used buffer size
	int          values_siz; // allocated buffer size

	// current parse state
	int   parse_idx;
	int   parse_first; // first output value
	char *parse_stack; // starting stack so we can play stack overflow chicken
	int   parse_com;   // a list of comments, cache before we hand it off to a value and a final file comment if non zero.
	int   parse_com_last; // end of comment chain so we can easily add another one

	char *error_string; // if this is not 0 we have an error
	int   error_idx;    // char in buffer
	int   error_char;   // char in line
	int   error_line;   // line in file
	
	FILE *fp; // where to write output

	char buf[256]; // small buffer used for generating text output
	
} djon_state ;

extern djon_state * djon_setup();
extern djon_value * djon_get(djon_state *it,int idx);
extern int djon_load_file(djon_state *it,char *fname);
extern int djon_parse(djon_state *it);

int djon_alloc(djon_state *it);
int djon_free(djon_state *it,int idx);
int djon_parse_value(djon_state *it);
int djon_check_stack(djon_state *it);

#define DJON_IS_WHITESPACE(c) ( ((c)==' ') || ((c)=='\t') || ((c)=='\n') || ((c)=='\r') || ((c)=='\v') || ((c)=='\f') )
#define DJON_IS_STRUCTURE(c)  ( ((c)=='{') || ((c)=='}') || ((c)=='[') || ((c)==']') || ((c)==':') || ((c)=='=') || ((c)==',') || ((c)=='/') )
#define DJON_IS_TERMINATOR(c) ( (((c)<=32)) || (((c)>=128)) || DJON_IS_STRUCTURE(c) )
// this will work when char is signed or unsigned , note that '/' is the start of /* or // comments

#endif


#ifdef DJON_C

// replace /n /t /uXXXX etc with utf8 chars, return new length of string
// new string should always be shorter due to encoding inefficiencies
int djon_unescape( char *cs , int len )
{
	char *cp=cs;     // input
	char *co=cs;     // output
	char *ce=cs+len; // end of input
	char c;
	int i;
	int t; // 16bit utf char
	while( cp<ce && co<ce ) // check all the chars and do not overrun
	{
		c=*cp;
		if(c=='\\')
		{
			cp++; // skip backslash
			c=*cp; // get char after
			cp++; // which we always consume
			switch(c)
			{
				case 'b' : *co++='\b'; break; // special chars
				case 'f' : *co++='\f'; break;
				case 'n' : *co++='\n'; break;
				case 'r' : *co++='\r'; break;
				case 't' : *co++='\t'; break;
				default  : *co++=c;    break; // everything else
				case 'u' :
					t=0;
					// note that we may read less than 4 chars here if they are not what we expect
					for(i=0;i<4;i++) // grab *upto* four hex chars next
					{
						c=*cp;
						if( c>='0' && c<='9' )
						{
							t=t*16+(c-'0');
							cp++;
						}
						else
						if( c>='a' && c<='f' )
						{
							t=t*16+(c-'a'+10);
							cp++;
						}
						else
						if( c>='A' && c<='F' )
						{
							t=t*16+(c-'A'+10);
							cp++;
						}
					}
					if(t<=0x007f)
					{
						*co++=t; // 7bit ascii
					}
					else
					if(t<=0x07ff)
					{
						*co++=0xc0|((t>>6) &0x1f); // top 5 bits
						*co++=0x80|( t     &0x3f); // last 6 bits
					}
					else // 0xffff
					{
						*co++=0xe0|((t>>12)&0x0f); // top 4 bits
						*co++=0x80|((t>>6) &0x3f); // next 6 bits
						*co++=0x80|( t     &0x3f); // last 6 bits
					}
				break;
			}
		}
		else // copy as is
		{
			*co++=c;
			cp++; // consume
		}
	}
	return co-cs; // number of chars written
}

// this will remove the DJON_ESCAPED bit from a string and fix the estring
// it is safe to call multiple times
int djon_unescape_string( djon_value * v )
{
	// must exist
	if(!v) { return 0; }

	// must be string
	if((v->typ&DJON_TYPEMASK)!=DJON_STRING) { return 0; }

	// must be escaped
	if(!(v->typ&DJON_ESCAPED)) { return 0; }

	v->len=djon_unescape(v->str,v->len); // perform unescape

	v->typ&=~DJON_ESCAPED; // remove flag

	return 1; // we changed the string
}

// can this string be a rawkey
int djon_is_rawkey( char *cp , int len )
{
	if(len<=0) { return 0; } // may not empty
	char *ce=cp+len;
	while( cp<ce ) // check all the chars
	{
		char c=*cp++;
		if( DJON_IS_TERMINATOR(c) ) { return 0; }
	} 
	return 1; // all chars OK
}

// write into buf, return length of string, maximum 32 including null
int djon_double_to_str( double num , char * buf )
{
// maximum precision of digits ( dependent on doubles precision )
#define DJON_DIGIT_PRECISION 15
// amount of zeros to include before/after decimal point before we switch to e numbers
#define DJON_DIGIT_ZEROS 8
// these two numbers +8 is the maximum we write to buf, so be careful
#define DJON_DIGIT_LEN (8+DJON_DIGIT_ZEROS+DJON_DIGIT_PRECISION)
// The possible extra non number chars are - . e-123 /0

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

	int i;
	int d;

	int digits=DJON_DIGIT_PRECISION;
	if( (e<0) && (e>=-DJON_DIGIT_ZEROS) ) // special case when close to 0 dont add an e until we get really small
	{
		digits=DJON_DIGIT_PRECISION+1-e;
		d=1.0;
		e=0; // we want a 0.0000001234 style number when it is near 0
	}
	
	double t=pow(10.0,(double)e); // divide by this to get current decimal
	num=num+pow(10.0,(double)e-digits); // add a tiny roundup for the digit after the last one we plan to print
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
		d=(int)((num)/t); //auto floor converting to int
		num-=((double)d)*t;
		t=t/10.0; // next digit
		if(d==0) { z++; } else { z=0; } // count zeros at end
		*cp++='0'+d;
	}

	if( (e>0) && (e<=DJON_DIGIT_ZEROS) ) // we want to keep all these zeros and add some more
	{
		for(i=0;i<e;i++) { *cp++='0'; }
		e=0; // we want a 1234000000 style number when it is near 0
	}
	else
	if(z>0) // undo trailing zeros
	{
		cp=cp-z; // remove zeros
		if(e>=0) // adjust e number
		{
			e=e+z; // new e
			if(t<0.1) { e=0; } // we only removed zeros after the . so e should be 0
		}
	}

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

// locales are a cursed so we need our own basic functions ?
// strtod replacement ?
double djon_str_to_double(char *cps,char **endptr)
{
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
		cp++;
		for( c=*cp ; (c>='0') && (c<='9') ; c=*++cp )// and 0 or more integer parts
		{
			m=m*0.1;
			d+=((c-'0')*m);
			gotdata++;
		}
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

	// final check, number must be terminated by something to be valid
	c=*cp;
	if( ! DJON_IS_TERMINATOR(c) ){goto error;}

	if(endptr){*endptr=cp;} // we used this many chars
	return d; // and parsed this number

error:
	if(endptr){*endptr=cps;} // 0 chars used
	return nan;
}

double djon_str_to_hex(char *cps,char **endptr)
{
	const double nan = 0.0/0.0;

	int gotdata=0;

	char *cp=cps;
	char c;
	
	double sign=1.0;
	if(c=='-') { sign=-1.0; cp++; } // maybe negative
	else
	if(c=='+') { cp++; } // allow

	if(!( (cp[0]=='0') && (cp[1]=='x') || (cp[1]=='X') )){goto error;}
	cp+=2; // skip 0x

	double d=0.0;
	for( c=*cp ; c ; c=*++cp )
	{
		
		if( (c>='0') && (c<='9') )
		{
			d=(d*16.0)+(double)(c-'0');
			gotdata++;
		}
		else
		if( (c>='a') && (c<='f') )
		{
			d=(d*16.0)+(double)(c-'a'+10);
			gotdata++;
		}
		else
		if( (c>='A') && (c<='F') )
		{
			d=(d*16.0)+(double)(c-'A'+10);
			gotdata++;
		}
		else
		{
			break;
		}
	}
	if(gotdata==0){goto error;} // require some numbers

	// final check, number must be terminated by something to be valid
	if( ! DJON_IS_TERMINATOR(c) ){goto error;}

	d*=sign; // apply sign

	if(endptr){*endptr=cp;} // we used this many chars
	return d; // and parsed this number

error:
	if(endptr){*endptr=cps;} // 0 chars used
	return nan;
}

double djon_str_to_number(char *cp,char **endptr)
{
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
}

// set an error string and calculate the line/char that we are currently on
void djon_set_error(djon_state *it, char *error)
{
	if( it->error_string ) { return; } // keep first error
	
	it->error_string=error;
	it->error_idx=0;
	it->error_char=0;
	it->error_line=0;
	
	if(!it->data){ return; } // must have some parsing data to locate

	it->error_idx=it->parse_idx;
	
	int x=0;
	int y=0;
	char * cp;
	char * cp_start = it->data;
	char * cp_end   = it->data+it->data_len;
	char * cp_error = it->data+it->parse_idx;
	
	y++; // 1st line
	for( cp = it->data ; cp<=cp_end ; cp++ )
	{
		x++;
		if( cp >= cp_error ) // found it
		{
			it->error_char=x;
			it->error_line=y;
			return;
		}
		if(*cp=='\n') // next line
		{
			x=0;
			y++;
		}
	}
	// error is at end of file
	it->error_char=x;
	it->error_line=y;
	return;
}

// allocate a new parsing state
djon_state * djon_setup()
{
	djon_state *it;
	
	it=(djon_state *)malloc( sizeof(djon_state) );
	if(!it) { return 0; }
	
	it->data=0;
	it->data_len=0;
	it->parse_stack=0;
	it->parse_idx=0;
	it->parse_com=0;
	it->parse_com_last=0;
	
	it->error_string=0;
	it->error_idx=0;
	it->error_char=0;
	it->error_line=0;

	it->values_len=1; // first value is used as a null so start at 1
	it->values_siz=16384;
	it->values=(djon_value *)malloc( it->values_siz * sizeof(djon_value) );
	if(!it->values) { free(it); return 0; }

	return it;
}
// free a new parsing state
void djon_clean(djon_state *it)
{
	if(!it) { return; }
	if(it->data) { free(it->data); }
	if(it->values) { free(it->values); }
}

// get a value by index
djon_value * djon_get(djon_state *it,int idx)
{
	if( idx <= 0 )              { return 0; }
	if( idx >= it->values_siz ) { return 0; }
	return it->values + idx ;
}

// unallocate unused values at the end of a parse
void djon_shrink(djon_state *it)
{
	djon_value * v;
	v=(djon_value *)realloc( (void*)it->values , (it->values_len) * sizeof(djon_value) );
	if(!v) { return; } //  fail but we can ignore
	it->values_siz=it->values_len;
}

// allocate a new value and return its index, 0 on error
int djon_alloc(djon_state *it)
{	
	djon_value * v;
	if( it->values_len+1 >= it->values_siz ) // check for space
	{
		v=(djon_value *)realloc( (void*)it->values , (it->values_siz+16384) * sizeof(djon_value) );
		if(!v) { djon_set_error(it,"out of memory"); return 0; }
		it->values_siz=it->values_siz+16384;
		it->values=v; // might change pointer
	}
	v=djon_get(it,it->values_len);
	v->nxt=0;
	v->str=0;
	v->len=0;
	v->typ=DJON_NULL;
	v->num=0.0;
	v->key=0;
	v->val=0;
	v->com=0;

	return it->values_len++;
}

// apply current cached comment chain to this value
void djon_apply_comments(djon_state *it, int idx)
{	
	djon_value * v;
	if( it->parse_com ) // we have a comment to save
	{
		v=djon_get(it,idx);
		while(v && v->com) // need to append to end of comment list
		{
			v=djon_get(it,v->com);
		}
		if(v)
		{
			v->com=it->parse_com;
			it->parse_com=0;
			it->parse_com_last=0;
		}
	}
}

// we can only free the top most allocated idx, return 0 if not freed
int djon_free(djon_state *it,int idx)
{
	if( idx == (it->values_len-1) )
	{
		it->values_len--;
		return idx;
	}
	return 0;
}
// write this char (16bit maybe) as a string escape sequence
void djon_write_string_escape(djon_state *it,int c)
{
	switch(c)
	{
		case '\\' : fputs("\\",  it->fp); break;
		case '\b' : fputs("\\b", it->fp); break;
		case '\f' : fputs("\\f", it->fp); break;
		case '\n' : fputs("\\n", it->fp); break;
		case '\r' : fputs("\\r", it->fp); break;
		case '\t' : fputs("\\t", it->fp); break;
		case '\'' : fputs("\\'", it->fp); break;
		case '"'  : fputs("\\\"",it->fp); break;
		case '`'  : fputs("\\`", it->fp); break;
		default:
			fprintf(it->fp,"\\u%04x",c&0xffff); // 16bit hex only
		break;
	}
}

int djon_write_indent(djon_state *it,int indent)
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
// write json to the given file handle
void djon_write_json(djon_state *it,int idx,int indent)
{
	djon_value *nxt=djon_get(it,idx);
	djon_value *key;
	djon_value *val;
	int key_idx;
	int val_idx;
	int len;
	char *cp;
	char *ce;
	char c;
	if(nxt)
	{
		char *coma=nxt->nxt?" ,":"";
		if((nxt->typ&DJON_TYPEMASK)==DJON_ARRAY)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"[\n");
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(val)
			{
				djon_write_json(it,val_idx,indent+1);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"]%s\n",coma);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_OBJECT)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"{\n");
			key_idx=nxt->key; key=djon_get(it,key_idx);
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(key&&val)
			{
				indent=djon_write_indent(it,indent+1)-1;
				fputs("\"",it->fp);
				for( cp=key->str,ce=key->str+key->len ; cp<ce ; cp++ )
				{
					c=*cp;
					if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
					{
						djon_write_string_escape(it,c);
					}
					else
					{
						putc(c,it->fp);
					}
				}
				fputs("\" : ",it->fp);
				djon_write_json(it,val_idx,-(indent+1));
				key_idx=key->nxt; key=djon_get(it,key_idx);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"}%s\n",coma);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_STRING)
		{
			indent=djon_write_indent(it,indent);
			fputs("\"",it->fp);
			for( cp=nxt->str,ce=nxt->str+nxt->len ; cp<ce ; cp++ )
			{
				c=*cp;
				if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
				{
					djon_write_string_escape(it,c);
				}
				else
				{
					putc(c,it->fp);
				}
			}
			fputs("\"",it->fp);
			fputs(coma,it->fp);
			fputs("\n",it->fp);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_NUMBER)
		{
			indent=djon_write_indent(it,indent);
			len=djon_double_to_str(nxt->num,it->buf);
			fwrite(it->buf, 1, len, it->fp);
			fprintf(it->fp,"%s\n",coma);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_BOOL)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s%s\n",nxt->num?"true":"false",coma);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_NULL)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s%s\n","null",coma);
		}
		else
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s%s\n","undefined",coma);
		}
	}
}

// write djon to the given file handle
void djon_write_djon(djon_state *it,int idx,int indent)
{
	djon_value *nxt=djon_get(it,idx);
	djon_value *key;
	djon_value *val;
	djon_value *com;
	int key_idx;
	int val_idx;
	int com_idx;
	int len;
	char *cp;
	char *ce;
	char c;
	if(nxt)
	{
		if( ((nxt->typ&DJON_TYPEMASK)!=DJON_COMMENT) && (nxt->com) )
		{
			djon_write_djon(it,nxt->com,indent);
		}
		
		if((nxt->typ&DJON_TYPEMASK)==DJON_COMMENT)
		{
			len=0;
			for( com_idx=idx ; com=djon_get(it,com_idx) ; com_idx=com->com )
			{
				len+=com->len;
			}
			if(len>0) // ignore a completely empty comment chain
			{
				for( com_idx=idx ; com=djon_get(it,com_idx) ; com_idx=com->com )
				{
					indent=djon_write_indent(it,indent);
					fprintf(it->fp,"// ");
					fwrite(com->str, 1, com->len, it->fp);
					fprintf(it->fp,"\n");
				}
			}
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_ARRAY)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"[\n");
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(val)
			{
				if(val->com)
				{
					djon_write_djon(it,val->com,indent+1);
				}
				djon_write_djon(it,val_idx,indent+1);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"]\n");
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_OBJECT)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"{\n");
			key_idx=nxt->key; key=djon_get(it,key_idx);
			val_idx=nxt->val; val=djon_get(it,val_idx);
			while(key&&val)
			{
				if(key->com)
				{
					djon_write_djon(it,key->com,indent+1);
				}
				if(val->com)
				{
					djon_write_djon(it,val->com,indent+1);
				}
				indent=djon_write_indent(it,indent+1)-1;
				if( djon_is_rawkey(key->str,key->len) )
				{
					fprintf(it->fp,"%.*s = ",key->len,key->str);
				}
				else
				{
					fputs("\"",it->fp);
					for( cp=key->str,ce=key->str+key->len ; cp<ce ; cp++ )
					{
						c=*cp;
						if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
						{
							djon_write_string_escape(it,c);
						}
						else
						{
							putc(c,it->fp);
						}
					}
					fputs("\" = ",it->fp);
				}
				djon_write_djon(it,val_idx,-(indent+1));
				key_idx=key->nxt; key=djon_get(it,key_idx);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"}\n");
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_STRING)
		{
			indent=djon_write_indent(it,indent);
			fputs("\"",it->fp);
			for( cp=nxt->str,ce=nxt->str+nxt->len ; cp<ce ; cp++ )
			{
				c=*cp;
				if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
				{
					djon_write_string_escape(it,c);
				}
				else
				{
					putc(c,it->fp);
				}
			}
			fputs("\"\n",it->fp);
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_NUMBER)
		{
			indent=djon_write_indent(it,indent);
			len=djon_double_to_str(nxt->num,it->buf);
			fwrite(it->buf, 1, len, it->fp);
			fprintf(it->fp,"\n");
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_BOOL)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s\n",nxt->num?"TRUE":"FALSE");
		}
		else
		if((nxt->typ&DJON_TYPEMASK)==DJON_NULL)
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s\n","NULL");
		}
		else
		{
			indent=djon_write_indent(it,indent);
			fprintf(it->fp,"%s\n","UNDEFINED");
		}
	}
}

// load a new file or possibly from stdin , pipes allowed
int djon_load_file(djon_state *it,char *fname)
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
		fp = fopen( fname , "rb" ); if(!fp) { djon_set_error(it,"file error"); goto error; }
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
			temp = realloc(data, used+chunk); if(!temp) { djon_set_error(it,"out of memory"); goto error; }
			size = used+chunk;
			data=temp;			
		}
		used += fread( data+used , 1 , chunk, fp );		
		if(ferror(fp)) { djon_set_error(it,"file error"); goto error; }
    }


	size = used+1; // this may size up or down
	temp = realloc(data, size); if(!temp) { djon_set_error(it,"out of memory"); goto error; }
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
int djon_peek_white(djon_state *it)
{
	char c1=it->data[ it->parse_idx ];
	char c2=it->data[ it->parse_idx+1 ];
	if( DJON_IS_WHITESPACE(c1) )
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
int djon_peek_punct(djon_state *it,char *punct)
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
int djon_peek_string(djon_state *it,char *s)
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
		if( DJON_IS_TERMINATOR(d) )
		{
			return 1;
		}
	}
	return 0;
}

// allocate a new comment in the current comment chain
int djon_alloc_comment(djon_state *it)
{
	// allocate new comment and place it in chain
	int com_idx=djon_alloc(it);
	djon_value * com=djon_get(it,com_idx);
	if(!com){return 0;}

	if( it->parse_com_last ) // append
	{
		djon_value * v=djon_get(it,it->parse_com_last);
		v->com=com_idx;
		it->parse_com_last=com_idx;
	}
	else // start a new one
	{
		it->parse_com=com_idx;
		it->parse_com_last=com_idx;
	}
	com->typ=DJON_COMMENT;
	com->str=it->data+it->parse_idx;
	com->len=0;
	
	return com_idx;
}

void djon_trim_comment(djon_state *it,int idx)
{
	char *cp;
	char c;
	djon_value *com=djon_get(it,idx);
	if(com)
	{
		for( c=*com->str ; ( com->len>0 ) && ( DJON_IS_WHITESPACE(c) ) ; c=*com->str )
		{
			com->str++;
			com->len--;
		}
		for( c=com->str[com->len-1] ; ( com->len>0 ) && ( DJON_IS_WHITESPACE(c) ) ; c=com->str[com->len-1] )
		{
			com->len--;
		}
	}
}
// skip whitespace and comments
void djon_skip_white(djon_state *it)
{
	int com_idx;
	djon_value * com;
	
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
			
			// allocate new comment and place it in chain
			com_idx=djon_alloc_comment(it);
			
			while( it->parse_idx < it->data_len )
			{
				c1=it->data[ it->parse_idx ];
				if( (c1=='\n') )
				{
					djon_trim_comment(it,com_idx);
					it->parse_idx++;
					return;
				}
				else
				{
					it->parse_idx++;
					if(com_idx)
					{
						com=djon_get(it,com_idx);
						if(com)
						{
							com->len++;
						}
					}
				}
			}
			return; // file ending counts as a \n so this is OK
		}
		else
		if( (c1=='/') && (c2=='*'))
		{
			it->parse_idx+=2;
			
			// allocate new comment and place it in chain
			com_idx=djon_alloc_comment(it);
			
			while( it->parse_idx < it->data_len )
			{
				c1=it->data[ it->parse_idx ];
				c2=it->data[ it->parse_idx+1 ];
				if( (c1=='*') || (c2=='/') )
				{
					djon_trim_comment(it,com_idx);
					it->parse_idx+=2;
					return;
				}
				else
				{
					if(c1=='\n') // chain a new comment at new line
					{
						djon_trim_comment(it,com_idx);
						com_idx=djon_alloc_comment(it);
					}
					else
					{
						it->parse_idx++;
						if(com_idx)
						{
							com=djon_get(it,com_idx);
							if(com)
							{
								com->len++;
							}
						}
					}
				}
			}
			djon_set_error(it,"missing */");
		}
		else
		{
			return;
		}
	}
}

// skip these punct chars only, counting thme
int djon_skip_punct(djon_state *it,char *punct)
{
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
	return ret;
}
// skip these punct chars or white space , counting how many punct chars we found.
int djon_skip_white_punct(djon_state *it,char *punct)
{
	int ret=0;
	int progress=-1;
	
	while( ( progress != it->parse_idx ) && ( it->parse_idx < it->data_len ) )
	{
		progress = it->parse_idx;
		
		djon_skip_white(it);
		ret+=djon_skip_punct(it,punct);
		djon_skip_white(it);

		// repeat until progress stops changing
	}

	return ret;
}

// allocate 0 length string value pointing at the next char 
int djon_parse_next(djon_state *it)
{
	if( it->parse_idx >= it->data_len ) { return 0; } // EOF

	djon_value *nxt;
	int idx=0;

	idx=djon_alloc(it);
	nxt=djon_get(it,idx);
	if(nxt==0) { return 0; }
	
	nxt->typ=DJON_STRING;
	nxt->str=it->data+it->parse_idx;
	nxt->len=0;
	
	djon_apply_comments(it,idx); // apply any comments
	
	return idx;
}

int djon_parse_string(djon_state *it,char * term)
{
	int str_idx=djon_parse_next(it);
	djon_value *str=djon_get(it,str_idx);
	if(!str) { return 0; }
	
	char c;
	char *cp;
	char *tp;
	int term_len=1; // assume 1 char
	
	if( *term=='`' ) // need to find special terminator
	{
		term=str->str;
		cp=term+1; // skip first char which is a `
		c=*cp; // next char
		if( (c=='\'') || (c=='"') || (c=='`') )// not a single opener
		{
			while(1)
			{
				c=*cp++;
				if( (c=='\'') || (c=='"') ) { term_len++; } // allowable
				else
				if( (c=='`') ) { term_len++; break; } // final
				else
				{
					term_len=1; // single ` only
					break;
				}
			}
		}
		str->typ=DJON_STRING; // no escapes allowed
	}
	else
	{
		str->typ=DJON_ESCAPED|DJON_STRING;
	}

	if( *term != '\n' ) // need to skip opening quote if not \n terminated
	{
		it->parse_idx+=term_len;
		str->str+=term_len;
		str->len=0;
	}

	while( it->parse_idx < it->data_len ) // while data
	{
		cp=it->data+it->parse_idx;
		c=*cp; // get next char
		
		if( str->typ=DJON_ESCAPED|DJON_STRING ) // we need to check for back slashes
		{
			if(c=='\\') // skip next char whatever it is
			{
				str->len+=2; // grow string
				it->parse_idx+=2; // advance
				continue;
			}
		}

		for( tp=term ; tp<term+term_len ; tp++,cp++ ) // check for term
		{
			if(*tp!=*cp) { break; } // not found
			if(tp==term+term_len-1) // found full terminator
			{
				it->parse_idx+=term_len; // advance
				djon_unescape_string(str); // fix string and remove escape bit
				return str_idx; // done
			}
		}

		str->len++; // grow string
		it->parse_idx++; // advance
	}
	if(*term=='\n')
	{
		djon_unescape_string(str); // fix string and remove escape bit
		return str_idx; // accept end of file as a \n
	}

	if(*term=='\'')
	{
		djon_set_error(it,"missing '");
	}
	else
	if(*term=='"')
	{
		djon_set_error(it,"missing \"");
	}
	else
	{
		djon_set_error(it,"missing string terminator");
	}
	return 0;
}

int djon_parse_number(djon_state *it)
{
	int num_idx=djon_parse_next(it);
	djon_value *num=djon_get(it,num_idx);
	if(!num){return 0;}

	char *cps=num->str;
	char *cpe;

//	double d=strtod(cps,&cpe);
	double d=djon_str_to_number(cps,&cpe);
	int len=cpe-cps;
	if( len > 0 ) // valid number
	{
		it->parse_idx+=len;

		num->typ=DJON_NUMBER;
		num->num=d;
		
		return num_idx;
	}
	djon_set_error(it,"invalid number");
	return 0;
}

int djon_parse_key(djon_state *it)
{
	djon_skip_white_punct(it,",");

	int key_idx=djon_parse_next(it);
	djon_value *key=djon_get(it,key_idx);
	if(!key){return 0;} // out of data

	char term=0;
	char c;
	char *cp;

	c=it->data[ it->parse_idx ];
	if( c=='\'' || c=='"' ) // open quote
	{
		term=c;
		key->str++; // skip opening quote
		it->parse_idx++; // advance
		key->typ=DJON_KEY|DJON_ESCAPED|DJON_STRING; // this is a key with escapes inside 
	}
	else
	{
		key->typ=DJON_KEY|DJON_STRING; // this is a key with no escape
	}

	while( it->parse_idx < it->data_len ) // while data
	{
		if(term==0) // naked string will not contain escapes
		{
			if( djon_peek_white(it) ) { return key_idx; } // stop at whitespace
			if( djon_peek_punct(it,"=:") ) { return key_idx; } // stop at punct or closing quote
			c=it->data[ it->parse_idx ];
			if( DJON_IS_TERMINATOR(c) ) // a naked key may not contain any terminator
			{ djon_set_error(it,"invalid naked key"); goto error; } 
		}
		else
		if( it->data[ it->parse_idx ]=='\\' ) // skip escaped char
		{
			key->len++; // grow string
			it->parse_idx++; // advance
		}
		else
		if( it->data[ it->parse_idx ]==term )
		{
			it->parse_idx++; // skip closing quote
			djon_unescape_string(key); // fix string and remove escape bit
			return key_idx; // end
		}

		key->len++; // grow string
		it->parse_idx++; // advance
	}
	if(term=='\'')
	{
		djon_set_error(it,"missing '");
	}
	else
	if(term=='"')
	{
		djon_set_error(it,"missing \"");
	}
	else
	{
		djon_set_error(it,"missing key terminator");
	}

error:
	return 0;
}

int djon_parse_object(djon_state *it)
{
	int obj_idx=djon_parse_next(it);
	djon_value *obj=djon_get(it,obj_idx);
	if(!obj){return 0;}

	djon_value *key;
	djon_value *val;

	it->parse_idx++; // skip opener
	obj->typ=DJON_OBJECT;

	int key_idx;
	int val_idx;

	while(1)
	{
		djon_skip_white_punct(it,",");
		if( it->data[it->parse_idx]=='}' ) // found closer
		{
			djon_apply_comments(it,val_idx?key_idx:obj_idx); // apply any final comments to the last key or the object
			it->parse_idx++;
			return obj_idx;
		}

		key_idx=djon_parse_key(it);
		if(!key_idx) { djon_set_error(it,"missing }"); return 0; }
		if( djon_skip_white_punct(it,"=:") != 1 ) { djon_set_error(it,"missing : or ="); return 0; } // required
		djon_apply_comments(it,key_idx); // apply any middle comments to the key
		val_idx=djon_parse_value(it); if(!val_idx){ djon_set_error(it,"missing value"); return 0; }
		djon_skip_white_punct(it,","); // optional , seperators

		if( obj->key==0) // first
		{
			obj->key=key_idx;
			obj->val=val_idx;
			key=djon_get(it,key_idx);
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			key->nxt=key_idx;
			val->nxt=val_idx;
			key=djon_get(it,key_idx);
			val=djon_get(it,val_idx);
		}
	}
	
	return 0;
}

int djon_parse_array(djon_state *it)
{
	int arr_idx=djon_parse_next(it);
	djon_value *arr=djon_get(it,arr_idx);
	if(!arr){return 0;}
	
	djon_value *val=0;

	it->parse_idx++; // skip opener
	arr->typ=DJON_ARRAY;

	int val_idx=0;

	while(1)
	{
		djon_skip_white_punct(it,",");
		if( it->data[it->parse_idx]==']' )  // found closer
		{
			djon_apply_comments(it,val_idx?val_idx:arr_idx); // apply any final comments to the last value or the array
			it->parse_idx++;
			return arr_idx;
		}

		val_idx=djon_parse_value(it);
		if(!val_idx) { djon_set_error(it,"missing ]"); return 0; } // no value, probably missed a ]
		djon_skip_white_punct(it,","); // optional , separators

		if( arr->val==0) // first
		{
			arr->val=val_idx;
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			val->nxt=val_idx;
			val=djon_get(it,val_idx);
		}
	}
	
	return arr_idx;
}

int djon_parse_value(djon_state *it)
{
	int idx;
	djon_value *nxt;
	
	if(it->error_string){ return 0; }
	if(!djon_check_stack(it)){ return 0; }

	djon_skip_white(it);

// check for special strings lowercase only
	if( djon_peek_string(it,"true" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->typ=DJON_BOOL;
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
		nxt->typ=DJON_BOOL;
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
		nxt->typ=DJON_NULL;
		it->parse_idx+=4;
		return idx;
	}

	char c=it->data[it->parse_idx]; // peek next char

	switch( c )
	{
		case '{' :
			return djon_parse_object(it);
		break;
		case '[' :
			return djon_parse_array(it);
		break;
		case '\'' :
			return djon_parse_string(it,"'");
		break;
		case '"' :
			return djon_parse_string(it,"\"");
		break;
		case '`' :
			return djon_parse_string(it,"`");
		break;
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
			return djon_parse_number(it);
		break;
	}

	return djon_parse_string(it,"\n");
}


// allocate a new value and return its index, 0 on error
int djon_check_stack(djon_state *it)
{
	int stack=0xdeadbeef;
	if(it->parse_stack) // check stack flag
	{
		int stacksize = it->parse_stack - ((char*)&stack); // oh yeah stack grows backwards
		if ( stacksize > (256*1024) ) // 256k stack burper, give up if we use too much
		{
			djon_set_error(it,"stack overflow");
			return 0;
		}
	}
	return 1;
}
int djon_parse(djon_state *it)
{
	int stack=0xdeadbeef;
	it->parse_stack=(char*)&stack;
	it->parse_idx=0;
	it->parse_first=0;

	it->error_string=0;
	it->error_char=0;
	it->error_line=0;

	int idx;
	djon_value *nxt;
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
		
		// , are ignored  between top level values as if this was an array
		djon_skip_white_punct(it,",");
	}
	djon_shrink(it);
	return it->parse_first;
error:
	it->parse_stack=0;
	return 0;
}

#endif

