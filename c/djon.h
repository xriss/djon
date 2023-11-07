

enum djon_enum
{
	NONE,
	NUMBER,
	BOOL,
	STRING,
	ARRAY,
	OBJECT,
};


struct djon_none
{
	int _;
};

struct djon_number
{
	double num;
};

struct djon_bool
{
	int num;
};

struct djon_string
{
	char *ptr;
	int   len;
};

struct djon_array
{
	int val;
	int len;
};

struct djon_object
{
	int nam;
	int val;
	int len;
};

union djon_union
{
	struct djon_none   _;
	struct djon_number n;
	struct djon_bool   b;
	struct djon_string s;
	struct djon_array  a;
	struct djon_object o;
};

struct djon_value
{
	int next; // idx to next value if this is an object key/value or array
	enum  djon_enum  t;
	union djon_union v;
};

struct djon_state
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
	int parse_char;
	int parse_line;
	int parse_first; // first output value
	char *parse_stack;
	
	FILE *fp; // where to print output

};

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


#ifdef DJON_CODE

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
		gotdata=0; // reset
		cp++;
		for( c=*cp ; (c>='0') && (c<='9') ; c=*++cp )// and 0 or more integer parts
		{
			m=m*0.1;
			d+=((c-'0')*m);
			gotdata++;
		}
		if(gotdata==0){goto error;} // require some numbers
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

		if(gotdata==0){goto error;} // require some numbers
	}
	
	d*=sign; // apply sign

	if(gotdata) // did we parse some data?
	{
		if(endptr){*endptr=cp;} // we used this many chars
		return d; // and parsed this number
	}

error:
	if(endptr){*endptr=cps;} // 0 chars used
	return nan;
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
	v->next=0;
	v->t=NONE;
	v->v._._=0;
	
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

int djon_print_indent(int indent)
{
	if(indent<0) { return -indent; } // skip first indent
	else
	{
		int i;
		for( i=0 ; i<indent ; i++)
		{
			putchar(' ');
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
	if(nxt)
	{
		if(nxt->t==ARRAY)
		{
//			if(indent<0)
//			{
//				indent=djon_print_indent(indent);
//				printf("\n");
//			}
			indent=djon_print_indent(indent);
			fprintf(it->fp,"[\n");
			val_idx=nxt->v.a.val; val=djon_get(it,val_idx);
			while(val)
			{
				djon_print(it,val_idx,indent+1);
				val_idx=val->next; val=djon_get(it,val_idx);
			}
			indent=djon_print_indent(indent);
			fprintf(it->fp,"]\n");
		}
		else
		if(nxt->t==OBJECT)
		{
//			if(indent<0)
//			{
//				indent=djon_print_indent(indent);
//				fprintf(it->fp,"\n");
//			}
			indent=djon_print_indent(indent);
			fprintf(it->fp,"{\n");
			nam_idx=nxt->v.o.nam; nam=djon_get(it,nam_idx);
			val_idx=nxt->v.o.val; val=djon_get(it,val_idx);
			while(nam&&val)
			{
				indent=djon_print_indent(indent+1)-1;
				fprintf(it->fp,"%.*s = ",nam->v.s.len,nam->v.s.ptr);
				djon_print(it,val_idx,-(indent+1));
				nam_idx=nam->next; nam=djon_get(it,nam_idx);
				val_idx=val->next; val=djon_get(it,val_idx);
			}
			indent=djon_print_indent(indent);
			fprintf(it->fp,"}\n");
		}
		else
		if(nxt->t==STRING)
		{
			indent=djon_print_indent(indent);
			fwrite(nxt->v.s.ptr, 1, nxt->v.s.len, it->fp);
			fwrite("\n", 1, 1, it->fp);
		}
		else
		if(nxt->t==NUMBER)
		{
			indent=djon_print_indent(indent);
			fprintf(it->fp,"%g\n",nxt->v.n.num);
		}
		else
		if(nxt->t==BOOL)
		{
			indent=djon_print_indent(indent);
			fprintf(it->fp,"%s\n",nxt->v.b.num?"TRUE":"FALSE");
		}
		else
		if(nxt->t==NONE)
		{
			indent=djon_print_indent(indent);
			fprintf(it->fp,"%s\n","NULL");
		}
		else
		{
			indent=djon_print_indent(indent);
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
int djon_parse_peek_white(struct djon_state *it)
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
int djon_parse_peek_punct(struct djon_state *it,char *punct)
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
int djon_parse_peek_string(struct djon_state *it,char *s)
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
void djon_parse_skip_white(struct djon_state *it)
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
int djon_parse_skip_white_punct(struct djon_state *it,char *punct)
{
	djon_parse_skip_white(it);

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
	
	djon_parse_skip_white(it);

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
	nxt->t=STRING;
	nxt->v.s.ptr=it->data+it->parse_idx;
	nxt->v.s.len=1;
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
		lst->v.s.len++; // grow string
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

	char *cps=lst->v.s.ptr;
	char *cpe;

//	double d=strtod(cps,&cpe);
	double d=djon_str_to_number(cps,&cpe);
	int len=cpe-cps;
	if( len > 0 )
	{
		it->parse_idx+=len-lst->v.s.len;

		lst->t=NUMBER;
		lst->v.n.num=d;
		
		return lst_idx;
	}
	return 0;
}

int djon_parse_name(struct djon_state *it)
{
	djon_parse_skip_white(it);

	int lst_idx=djon_parse_step(it);
	struct djon_value *lst=djon_get(it,lst_idx);
	if(lst==0){return 0;} // out of data

	char c;
	char *cp;

	while( it->parse_idx < it->data_len ) // while data
	{
		if( djon_parse_peek_white(it) ) { return lst_idx; } // stop at whitespace
		if( djon_parse_peek_punct(it,"=:") ) { return lst_idx; } // stop at punct

		lst->v.s.len++; // grow string
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

	lst->t=OBJECT;
	lst->v.o.nam=0;
	lst->v.o.val=0;
	lst->v.o.len=0;

	int nam_idx;
	int val_idx;

	while(1)
	{
		djon_parse_skip_white(it);
		if( it->data[it->parse_idx]=='}' ) { it->parse_idx++;  return lst_idx; } // found closer

		nam_idx=djon_parse_name(it);
		if(!nam_idx) { return 0; } // no value
		if( djon_parse_skip_white_punct(it,"=:") != 1 ) { return 0; } // required assignment
		val_idx=djon_parse_value(it); if(!val_idx){return 0;}
		djon_parse_skip_white_punct(it,",;"); // optional , seperators

		if( lst->v.o.len==0) // first
		{
			lst->v.o.nam=nam_idx;
			lst->v.o.val=val_idx;
			lst->v.o.len++;
			nam=djon_get(it,nam_idx);
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			nam->next=nam_idx;
			val->next=val_idx;
			lst->v.o.len++;
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

	lst->t=ARRAY;
	lst->v.o.val=0;
	lst->v.o.len=0;

	int val_idx;

	while(1)
	{
		djon_parse_skip_white(it);
		if( it->data[it->parse_idx]==']' ) { it->parse_idx++; return lst_idx; } // found closer

		val_idx=djon_parse_value(it);
		if(!val_idx) { return 0; } // no value
		djon_parse_skip_white_punct(it,",;"); // optional , separators

		if( lst->v.a.len==0) // first
		{
			lst->v.a.val=val_idx;
			lst->v.a.len++;
			val=djon_get(it,val_idx);
		}
		else // chain
		{
			val->next=val_idx;
			lst->v.a.len++;
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

	djon_parse_skip_white(it);

// check for special strings lowercase only
	if( djon_parse_peek_string(it,"true" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->t=BOOL;
		nxt->v.b.num=1;
		it->parse_idx+=4;
		return idx;
	}
	else
	if( djon_parse_peek_string(it,"false" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->t=BOOL;
		nxt->v.b.num=0;
		it->parse_idx+=5;
		return idx;
	}
	else
	if( djon_parse_peek_string(it,"null" ) )
	{
		idx=djon_alloc(it);
		nxt=djon_get(it,idx);
		if(nxt==0) { return 0; }
		nxt->t=NONE;
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
	char c=*(nxt->v.s.ptr);

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
	it->parse_char=0;
	it->parse_line=0;
	it->parse_first=0;
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
			nxt->next=idx;
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

