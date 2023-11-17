
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef DJON_H
#define DJON_H

#ifdef __cplusplus
extern "C" {
#endif

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

	int       nxt; // idx to next value if this is part of a list
	int       prv; // idx to prev value if this is part of a list

	char    * str; // start of string ( points into djon_state.data )
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
	int   compact; // compact output flag
	void (*write)(struct djon_state *it, char *cp, int len); // ptr to output function

	char buf[256]; // small buffer used for generating text output

} djon_state ;


extern djon_state * djon_setup();
extern void         djon_clean(       djon_state *it);
extern int          djon_load_file(   djon_state *it, char *fname);
extern int          djon_parse(       djon_state *it);
extern void         djon_set_error(   djon_state *it, char *error);
extern void         djon_write(       djon_state *it, char *ptr, int len );
extern void         djon_write_json(  djon_state *it, int idx);
extern void         djon_write_djon(  djon_state *it, int idx);
extern int          djon_alloc(       djon_state *it);
extern int          djon_free(        djon_state *it, int idx);
extern int          djon_idx(         djon_state *it, djon_value *val);
extern djon_value * djon_get(         djon_state *it, int idx);
extern int          djon_parse_value( djon_state *it);
extern int          djon_check_stack( djon_state *it);
extern void         djon_sort_object( djon_state *it, int idx );



#define DJON_IS_WHITESPACE(c) ( ((c)==' ') || ((c)=='\t') || ((c)=='\n') || ((c)=='\r') || ((c)=='\v') || ((c)=='\f') )
#define DJON_IS_STRUCTURE(c)  ( ((c)=='{') || ((c)=='}') || ((c)=='[') || ((c)==']') || ((c)==':') || ((c)=='=') || ((c)==',') || ((c)=='/') )
#define DJON_IS_TERMINATOR(c) ( (((c)<=32)) || (((c)>=128)) || DJON_IS_STRUCTURE(c) )
// this will work when char is signed or unsigned , note that '/' is the start of /* or // comments

#endif


#ifdef DJON_C

// compare lowercase null terminated string s, to the start of the cs buffer
// returns true if lowercase ( cs < cs+len ) buffer begins with the s string.
int djon_starts_with_string(char *cs,int len,char *s)
{
	char *ce=cs+len;
	char *sp;
	char *cp;
	char c;
	for( cp=cs,sp=s ; *sp ; cp++,sp++ )
	{
		if(cp>=ce) { return 0; } // out of buffer but not out of string
		c=*cp;
		if( c>='A' && c<='Z' ) { c=c+('a'-'A'); } // lowercase
		if( c!=*sp ) { return 0; } // no match
	}
	return 1; // cs starts with s
}

// check that a quote does not occur in the string, returns 1 if it does not
int djon_check_quote( char *cs , int len , char *quote )
{
	char *cq;
	char *ct;
	char *cp;
	char *ce;
	char c;
	for( cp=cs,ce=cs+len ; cp<ce ; cp++ ) // scan buffer
	{
		for( cq=quote , ct=cp ; *cq && (ct<=ce); cq++ , ct++ ) // step ct one char more
		{
			if( ct==ce ) // past end of input
			{
				c='`'; // fake end quote
			}
			else
			{
				c=*ct;
			}
			if( *cq!=c ) { break; }
		}
		if(*cq==0) { return 0; } // found quote string
	}
	return 1; // not found
}

// Pick a ` quote that does not occur in the string,
// this will be written into buf as it may be more than one char
// there are so many possibilities that although this could technically fail
// the string to cause this failure would have to be many gigabytes
char * djon_pick_quote( char *cs , int len , char *buf )
{
	if(len<=0) // use "" for empty string
	{
		buf[0]='"';buf[1]=0;
		return buf;
	}
// check single
	buf[0]='`';buf[1]=0;
	if(djon_check_quote(cs,len,buf)){return buf;}

// check 2^32 more
	unsigned int bs;
	unsigned int bm;
	char *cp;
	for(bs=0;bs<=0xffffffff;bs++)
	{
		cp=buf;
		*cp++='`'; // first `
		for( bm=0x80000000 ; bm>0x00000001 ; bm=bm>>1 ) // slide bit right
		{
			if( bm<=bs ) { break; } // until we find the top bit
		}
		while(bm>0)
		{
			if(bs&bm) { *cp++='\''; }
			else      { *cp++='"'; }
			bm=bm>>1; // keep sliding
		}
		*cp++='`'; // final `
		*cp++=0;
		if(djon_check_quote(cs,len,buf))
		{
			return buf;
		}
	}
	// we should never reach here
	printf("djon_pick_quote failure\n");
	exit(23);
	return 0;
}

// replace /n /t /uXXXX etc with utf8 chars, return new length of string
// new string should always be shorter due to encoding inefficiencies
int djon_unescape( char *cs , int len )
{
	char *cp2;
	char *cp=cs;     // input
	char *co=cs;     // output
	char *ce=cs+len; // end of input
	char c;
	int i;
	int t; // 16bit utf char
	int t2;
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
					
					// need to check for surrogate pair
					if( (t&0xFC00)==0xD800 ) // we got the first part, try and parse the second
					{
						if( (cp[0]=='\\') && (cp[1]=='u') )
						{
							cp2=cp; // might need to undo
							cp+=2;
							t2=0;
							for(i=0;i<4;i++) // grab *upto* four hex chars next
							{
								c=*cp;
								if( c>='0' && c<='9' )
								{
									t2=t2*16+(c-'0');
									cp++;
								}
								else
								if( c>='a' && c<='f' )
								{
									t2=t2*16+(c-'a'+10);
									cp++;
								}
								else
								if( c>='A' && c<='F' )
								{
									t2=t2*16+(c-'A'+10);
									cp++;
								}
							}
							if( (t2&0xFC00)==0xDC00 ) // we got the second part
							{
								t= 0x10000 + ((t&0x03FF)*0x0400) + (t2&0x03FF); // 21bitish
							}
							else // undo second part, number was not part of a pair
							{
								cp=cp2;
							}
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
					else
					if(t<=0xffff)
					{
						*co++=0xe0|((t>>12)&0x0f); // top 4 bits
						*co++=0x80|((t>>6) &0x3f); // next 6 bits
						*co++=0x80|( t     &0x3f); // last 6 bits
					}
					else // 1FFFFF
					{
						*co++=0xf0|((t>>18)&0x03); // top 3 bits
						*co++=0x80|((t>>12)&0x3f); // next 6 bits
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

// can this string be naked
int djon_is_naked_string( char *cp , int len )
{
	if(len<=0) { return 0; } // string may not empty
	char *ce=cp+len;
	char c=*cp;
	if(!(((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z'))) ) // check starting char is a letter
	{
		return 0; // does not start with a letter
	}
	// check for json keywords that will trip us up
	// a naked string may not begin with these words
	if( djon_starts_with_string(cp,len,"true") ) { return 0; }
	if( djon_starts_with_string(cp,len,"false") ) { return 0; }
	if( djon_starts_with_string(cp,len,"null") ) { return 0; }
	// string can not end with whitespace as it would be stripped
	c=*(cp+len-1);
	if( DJON_IS_WHITESPACE(c) ) { return 0; } // ends in whitespace
	// finally need to make sure that string down not contain a \n
	while( cp<ce )
	{
		if(*cp=='\n') { return 0; } // found a \n
		cp++;
	}
	return 1; // all chars OK
}

// can this key be naked
int djon_is_naked_key( char *cp , int len )
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

	it->write=&djon_write; // default write function
	it->fp=0;
	it->compact=0;


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

// get a index from pointer
int djon_idx(djon_state *it,djon_value *val)
{
	if(!val){return 0;}
	return val - it->values;
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
	v->prv=0;
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

// this function the user can replace
void djon_write(djon_state *it, char *ptr, int len )
{
	fwrite(ptr, 1, len, it->fp);
}

void djon_write_it(djon_state *it, char *ptr, int len )
{
	if(it->write)
	{
		(*it->write)(it,ptr,len);
	}
}

void djon_write_char(djon_state *it, char c)
{
	if(it->write)
	{
		(*it->write)(it,&c,1);
	}
}

// find length of a null term string and write it
void djon_write_string(djon_state *it, char *ptr)
{
	int len=0;
	char *cp=ptr;
	while( *cp ){ len++; cp++; }
	if(it->write)
	{
		(*it->write)(it,ptr,len);
	}
}

// write this char (16bit maybe) as a string escape sequence
void djon_write_string_escape(djon_state *it,int c)
{
	switch(c)
	{
		case '\\' : djon_write_string(it,"\\"  ); break;
		case '\b' : djon_write_string(it,"\\b" ); break;
		case '\f' : djon_write_string(it,"\\f" ); break;
		case '\n' : djon_write_string(it,"\\n" ); break;
		case '\r' : djon_write_string(it,"\\r" ); break;
		case '\t' : djon_write_string(it,"\\t" ); break;
		case '\'' : djon_write_string(it,"\\'" ); break;
		case '"'  : djon_write_string(it,"\\\""); break;
		case '`'  : djon_write_string(it,"\\`" ); break;
		default:
			char b[16];
			sprintf(b,"\\u%04x",c&0xffff); // 16bit hex only
			djon_write_string(it,b);
		break;
	}
}

int djon_write_indent(djon_state *it,int indent)
{
	if(indent<0) { return -indent; } // skip first indent
	else
	{
		if(!it->compact)
		{
			int i;
			for( i=0 ; i<indent ; i++)
			{
				djon_write_char(it,' ');
			}
		}
	}
	return indent;
}
// write json with indent state
void djon_write_json_indent(djon_state *it,int idx,int indent,char *coma)
{
	djon_value *v=djon_get(it,idx);
	djon_value *key;
	djon_value *val;
	int key_idx;
	int val_idx;
	int len;
	char *cp;
	char *ce;
	char c;
	if(v)
	{
		if(it->compact)
		{
			if(!coma){coma=v->nxt?",":"";} // auto coma
		}
		else
		{
			if(!coma){coma=v->nxt?" ,":"";} // auto coma
		}
		if((v->typ&DJON_TYPEMASK)==DJON_ARRAY)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"[");
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
			val_idx=v->val; val=djon_get(it,val_idx);
			while(val)
			{
				djon_write_json_indent(it,val_idx,indent+1,0);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"]");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_OBJECT)
		{
			djon_sort_object(it,idx); // sort
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"{");
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
			key_idx=v->key; key=djon_get(it,key_idx);
			while(key)
			{
				indent=djon_write_indent(it,indent+1)-1;
				djon_write_string(it,"\"");
				for( cp=key->str,ce=key->str+key->len ; cp<ce ; cp++ )
				{
					c=*cp;
					if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
					{
						djon_write_string_escape(it,c);
					}
					else
					{
						djon_write_char(it,c);
					}
				}
				if(it->compact)
				{
					djon_write_string(it,"\":");
					djon_write_json_indent(it,key->val,-(indent+1),key->nxt?",":"");
				}
				else
				{
					djon_write_string(it,"\" : ");
					djon_write_json_indent(it,key->val,-(indent+1),key->nxt?" ,":"");
				}
				key_idx=key->nxt; key=djon_get(it,key_idx);
			}
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"}");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_STRING)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"\"");
			for( cp=v->str,ce=v->str+v->len ; cp<ce ; cp++ )
			{
				c=*cp;
				if( ( (c>=0x00)&&(c<=0x1F) ) || (c=='"') || (c=='\\') ) // must escape
				{
					djon_write_string_escape(it,c);
				}
				else
				{
					djon_write_char(it,c);
				}
			}
			djon_write_string(it,"\"");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_NUMBER)
		{
			indent=djon_write_indent(it,indent);
			len=djon_double_to_str(v->num,it->buf);
			djon_write_it(it,it->buf,len);
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_BOOL)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,v->num?"true":"false");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_NULL)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"null");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
		else // should not get here
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"null");
			djon_write_string(it,coma);
			if(!it->compact)
			{
				djon_write_string(it,"\n");
			}
		}
	}
}
// write json to the it->fp file handle
void djon_write_json(djon_state *it,int idx)
{
	djon_write_json_indent(it,idx,0,0);
}

// write djon to the given file handle
void djon_write_djon_indent(djon_state *it,int idx,int indent)
{
	djon_value *v=djon_get(it,idx);
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
	int rawstr;
	if(v)
	{
		if( ((v->typ&DJON_TYPEMASK)!=DJON_COMMENT) && (v->com) )
		{
			djon_write_djon_indent(it,v->com,indent);
		}

		if((v->typ&DJON_TYPEMASK)==DJON_COMMENT)
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
					djon_write_string(it,"//");
					if(!it->compact)
					{
						djon_write_string(it," ");
					}
					djon_write_it(it,com->str,com->len);
					djon_write_string(it,"\n");
				}
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_ARRAY)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"[");
			djon_write_string(it,"\n");
			val_idx=v->val; val=djon_get(it,val_idx);
			while(val)
			{
				if(val->com)
				{
					djon_write_djon_indent(it,val->com,indent+1);
				}
				djon_write_djon_indent(it,val_idx,indent+1);
				val_idx=val->nxt; val=djon_get(it,val_idx);
			}
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"]");
			djon_write_string(it,"\n");
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_OBJECT)
		{
			djon_sort_object(it,idx); // sort
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"{");
			djon_write_string(it,"\n");
			key_idx=v->key; key=djon_get(it,key_idx);
			while(key)
			{
				if(key->com)
				{
					djon_write_djon_indent(it,key->com,indent+1);
				}
				indent=djon_write_indent(it,indent+1)-1;
				if( djon_is_naked_key(key->str,key->len) )
				{
					djon_write_it(it,key->str,key->len);
					if(it->compact)
					{
						djon_write_string(it,"=");
					}
					else
					{
						djon_write_string(it," = ");
					}
				}
				else
				{
					djon_pick_quote(key->str,key->len,it->buf);
					djon_write_string(it,it->buf);
					djon_write_it(it,key->str,key->len);
					djon_write_string(it,it->buf);
					if(it->compact)
					{
						djon_write_string(it,"=");
					}
					else
					{
						djon_write_string(it," = ");
					}
				}
				djon_write_djon_indent(it,key->val,-(indent+1));
				key_idx=key->nxt; key=djon_get(it,key_idx);
			}
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"}");
			djon_write_string(it,"\n");
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_STRING)
		{
			if( djon_is_naked_string(v->str,v->len) )
			{
				indent=djon_write_indent(it,indent);
				djon_write_it(it,v->str,v->len);
				djon_write_string(it,"\n");
			}
			else
			{
				indent=djon_write_indent(it,indent);
				djon_pick_quote(v->str,v->len,it->buf);
				djon_write_string(it,it->buf);
				djon_write_it(it,v->str,v->len);
				djon_write_string(it,it->buf);
				djon_write_string(it,"\n");
			}
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_NUMBER)
		{
			indent=djon_write_indent(it,indent);
			len=djon_double_to_str(v->num,it->buf);
			djon_write_it(it,it->buf,len);
			djon_write_string(it,"\n");
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_BOOL)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,v->num?"TRUE":"FALSE");
			djon_write_string(it,"\n");
		}
		else
		if((v->typ&DJON_TYPEMASK)==DJON_NULL)
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"NULL");
			djon_write_string(it,"\n");
		}
		else
		{
			indent=djon_write_indent(it,indent);
			djon_write_string(it,"NULL");
			djon_write_string(it,"\n");
		}
	}
}
// write djon to the it->fp file handle
void djon_write_djon(djon_state *it,int idx)
{
	djon_write_djon_indent(it,idx,0);
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
// skip whitespace and comments return amount skipped
int djon_skip_white(djon_state *it)
{
	int start=it->parse_idx;
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
					return it->parse_idx-start;
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
			return it->parse_idx-start; // file ending counts as a \n so this is OK
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
					return it->parse_idx-start;
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
			return it->parse_idx-start;
		}
	}
	return it->parse_idx-start;
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

	djon_value *v;
	int idx=0;

	idx=djon_alloc(it);
	v=djon_get(it,idx);
	if(v==0) { return 0; }

	v->typ=DJON_STRING;
	v->str=it->data+it->parse_idx;
	v->len=0;

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
			if( djon_peek_punct(it,"=:") ) { return key_idx; } // stop at punct
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
		djon_set_error(it,"missing :");
	}

error:
	return 0;
}




// remove from list
void djon_list_remove(djon_state *it, djon_value *v)
{
	djon_value *p;
	p = djon_get(it,v->nxt); if(p){ p->prv=v->prv; }
	p = djon_get(it,v->prv); if(p){ p->nxt=v->nxt; }
	v->nxt=0;
	v->prv=0;
}

// swap a and b
void djon_list_swap(djon_state *it, djon_value *a , djon_value *b )
{
	djon_value *p;
	int aidx=djon_idx(it,a);
	int bidx=djon_idx(it,b);
	
	// cache
	int anxt=a->nxt;
	int aprv=a->prv;
	int bnxt=b->nxt;
	int bprv=b->prv;
	
	// relink
	p = djon_get(it,aprv); if(p){ p->nxt=bidx; }
	p = djon_get(it,anxt); if(p){ p->prv=bidx; }
	p = djon_get(it,bprv); if(p){ p->nxt=aidx; }
	p = djon_get(it,bnxt); if(p){ p->prv=aidx; }

	// swap and fix for when ab are adjacent
	a->nxt = (bnxt==aidx) ? bidx : bnxt ;
	a->prv = (bprv==aidx) ? bidx : bprv ;
	b->nxt = (anxt==bidx) ? aidx : anxt ;
	b->prv = (aprv==bidx) ? aidx : aprv ;
}


// return a<b
int djon_sort_compare( djon_value *a , djon_value *b )
{
	int i=0;
	while( i<a->len && i<b->len )
	{
		if(a->str[i]<b->str[i]) { return 1; }
		if(a->str[i]>b->str[i]) { return 0; }
		i++;
	}
	if(a->len<b->len) { return 1; }
	return 0;
}
// dumb sort
void djon_sort_part(djon_state *it, djon_value *s, djon_value *e )
{
	djon_value *t,*i,*j;
	for( i=s ; i!=e ; i=djon_get(it,i->nxt) ) // start to end-1
	{
		for( j=djon_get(it,i->nxt) ; j!=e ; j=djon_get(it,j->nxt) ) // i+1 to end-1
		{
			if( djon_sort_compare(j,i) ) // should j come before i
			{
				djon_list_swap(it,i,j);
				t=i; i=j; j=t; // swap i/j
			}
		}
		if( djon_sort_compare(e,i) ) // should e come before i
		{
			djon_list_swap(it,i,e);
			t=i; i=e; e=t; // swap i/e
		}
	}
}

// force sort this object by its keys
void djon_sort_object(djon_state *it, int idx )
{
	djon_value *obj=djon_get(it,idx);
	if(!obj) { return; }
	if(!obj->key) { return; }
	
	djon_value *s=djon_get(it,obj->key);
	djon_value *e;
	for( e=s ; e && e->nxt ; e=djon_get(it,e->nxt) ) {;}

	djon_sort_part(it, s, e );
	while( s && s->prv ) { s = djon_get(it,s->prv); } // find new start
	obj->key=djon_idx(it,s); // save new start
}


// return a==b
int djon_clean_compare( djon_value *a , djon_value *b )
{
	int i=0;
	while( i<a->len && i<b->len )
	{
		if(a->str[i]!=b->str[i]) { return 0; }
		i++;
	}
	if(a->len!=b->len) { return 0; }
	return 1;
}

// remove any duplicate keys keeping the last one
void djon_clean_object(djon_state *it, int idx )
{
	djon_value *obj=djon_get(it,idx);
	if(!obj) { return; }
	if(!obj->key) { return; }
	
	djon_value *l;
	
	djon_value *s=djon_get(it,obj->key);
	djon_value *e;
	for( e=s ; e && e->nxt ; e=djon_get(it,e->nxt) ) {;}

	djon_value *i;
	djon_value *j;
	int ji;
	for( i=e ; i ; i=djon_get(it,i->prv) ) // loop backwards checking
	{
		ji=i->prv;
		while( j=djon_get(it,ji) ) // loop backwards deleting
		{
			ji=j->prv; // remember so we can move j
			if(djon_clean_compare(i,j)) // dupe
			{
				djon_list_remove(it,j);
			}
		}
	}
	// end will be the same but start may have changed so
	for( s=e ; s && s->prv ; s=djon_get(it,s->prv) ) {;}
	obj->key=djon_idx(it,s);
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

	int lst_idx=0;
	int key_idx=0;
	int val_idx=0;

	while(1)
	{
		djon_skip_white_punct(it,",");
		if( it->data[it->parse_idx]=='}' ) // found closer
		{
			djon_apply_comments(it,key_idx?key_idx:obj_idx); // apply any final comments to the last key or the object
			it->parse_idx++;
			djon_clean_object(it,obj_idx); // remove duplicate keys
			return obj_idx;
		}

		key_idx=djon_parse_key(it);
		if(!key_idx) { djon_set_error(it,"missing }"); return 0; }
		if( djon_skip_white_punct(it,"=:") != 1 ) { djon_set_error(it,"missing :"); return 0; } // required
		djon_apply_comments(it,key_idx); // apply any middle comments to the key
		val_idx=djon_parse_value(it); if(!val_idx){ djon_set_error(it,"missing value"); return 0; }

		if( obj->key==0) // first
		{
			obj->key=key_idx;
			obj->val=val_idx;
			key=djon_get(it,key_idx);
			key->val=val_idx; //  remember val for this key
			lst_idx=key_idx;
		}
		else // chain
		{
			key=djon_get(it,lst_idx); // last key
			key->nxt=key_idx;
			key=djon_get(it,key_idx);
			key->prv=lst_idx;
			key->val=val_idx; //  remember val for this key
			lst_idx=key_idx;
		}

		if( 0 == djon_skip_white(it) ) // check for whitespace after value
		{
			if( it->parse_idx+1 < it->data_len ) // EOF?
			{
				char c=it->data[it->parse_idx]; // if not white then must be one of these chars
				if( ! ( ( c==',' ) || ( c=='}' ) ) )
				{
					djon_set_error(it,"missing ,"); return 0;
				}
			}
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

	int lst_idx=0;
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

		if( arr->val==0) // first
		{
			arr->val=val_idx;
			lst_idx=val_idx;
		}
		else // chain
		{
			val=djon_get(it,lst_idx);
			val->nxt=val_idx;
			lst_idx=val_idx;
		}
		if( 0 == djon_skip_white(it) ) // check for whitespace after value
		{
			if( it->parse_idx < it->data_len ) // EOF?
			{
				char c=it->data[it->parse_idx]; // if not white then must be one of these chars
				if( ! ( ( c==',' ) || ( c==']' ) ) )
				{
					djon_set_error(it,"missing ,"); return 0;
				}
			}
		}

	}

	return arr_idx;
}

int djon_parse_value(djon_state *it)
{
	int idx;
	djon_value *v;

	if(it->error_string){ return 0; }
	if(!djon_check_stack(it)){ return 0; }

	djon_skip_white(it);

// check for special strings lowercase only
	if( djon_peek_string(it,"true" ) )
	{
		idx=djon_alloc(it);
		v=djon_get(it,idx);
		if(v==0) { return 0; }
		v->typ=DJON_BOOL;
		v->num=1.0;
		it->parse_idx+=4;
		return idx;
	}
	else
	if( djon_peek_string(it,"false" ) )
	{
		idx=djon_alloc(it);
		v=djon_get(it,idx);
		if(v==0) { return 0; }
		v->typ=DJON_BOOL;
		v->num=0.0;
		it->parse_idx+=5;
		return idx;
	}
	else
	if( djon_peek_string(it,"null" ) )
	{
		idx=djon_alloc(it);
		v=djon_get(it,idx);
		if(v==0) { return 0; }
		v->typ=DJON_NULL;
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

	int lst_idx=0;
	int idx=0;
	djon_value *v;
	while( idx=djon_parse_value(it) )
	{
		if(it->parse_first==0) // remember the first value
		{
			it->parse_first=idx;
			lst_idx=idx;
		}
		else
		{
			v=djon_get(it,lst_idx);
			v->nxt=idx;
			lst_idx=idx;
		}
		v=djon_get(it,idx);
		if(v==0){	goto error; }

		// , are ignored  between top level values as if this was an array
		djon_skip_white_punct(it,",");
	}
	djon_shrink(it);
	return it->parse_first;
error:
	it->parse_stack=0;
	return 0;
}

#ifdef __cplusplus
};
#endif

#endif

