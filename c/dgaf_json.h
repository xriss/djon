

enum dgaf_json_enum
{
	NONE,
	NUMBER,
	BOOL,
	STRING,
	ARRAY,
	OBJECT,
};


struct dgaf_json_none
{
	int _;
};

struct dgaf_json_number
{
	double num;
};

struct dgaf_json_bool
{
	int bol;
};

struct dgaf_json_string
{
	char *ptr;
	int   len;
};

struct dgaf_json_array
{
	int val;
	int len;
};

struct dgaf_json_object
{
	int nam;
	int val;
	int len;
};

union dgaf_json_union
{
	struct dgaf_json_none   _;
	struct dgaf_json_number n;
	struct dgaf_json_bool   b;
	struct dgaf_json_string s;
	struct dgaf_json_array  a;
	struct dgaf_json_object o;
};

struct dgaf_json_value
{
	int next; // idx to next value if this is an object key/value or array
	enum  dgaf_json_enum  t;
	union dgaf_json_union v;
};

struct dgaf_json_state
{
	char *data;
	int   data_len;

	int values_len;
	int values_siz;
	int values_root;
	
	int parse_mode;
	int parse_idx;
	int parse_char;
	int parse_line;
};

extern struct dgaf_json_state * dgaf_json_setup();
extern struct dgaf_json_value * dgaf_json_get(struct dgaf_json_state *it,int idx);
extern int dgaf_json_load_file(struct dgaf_json_state *it,char *fname);
extern int dgaf_json_parse(struct dgaf_json_state *it);

int dgaf_json_alloc(struct dgaf_json_state *it);
int dgaf_json_free(struct dgaf_json_state *it,int idx);
int dgaf_json_parse_value(struct dgaf_json_state *it);


#ifdef DGAF_JSON_CODE

// allocate a new parsing state
struct dgaf_json_state * dgaf_json_setup()
{
	struct dgaf_json_state *it;
	
	it=(struct dgaf_json_state *)malloc( 1024 * sizeof(struct dgaf_json_value) );
	if(!it) { return 0; }
	it->values_siz=1024;
	
	it->data="";
	it->data_len=0;

	// the first few slots are taken by this state structure (hax)
	it->values_len=(1+(sizeof(struct dgaf_json_state)/sizeof(struct dgaf_json_value)));

// the json root
//	it->values_root=dgaf_json_alloc(it);
	
	return it;
}

// get a value by index
struct dgaf_json_value * dgaf_json_get(struct dgaf_json_state *it,int idx)
{
	if( idx <= 0 )              { return 0; }
	if( idx >= it->values_siz ) { return 0; }
	return ((struct dgaf_json_value *)(it)) + idx ;
}

// debug print a value
void dgaf_json_debug_print(struct dgaf_json_state *it,int idx,int indent)
{
	struct dgaf_json_value *nxt=dgaf_json_get(it,idx);
	struct dgaf_json_value *nam;
	struct dgaf_json_value *val;
	int nam_idx;
	int val_idx;
	if(nxt)
	{
		if(nxt->t==ARRAY)
		{
			printf("[\n");
			val_idx=nxt->v.a.val; val=dgaf_json_get(it,val_idx);
			while(val)
			{
				dgaf_json_debug_print(it,val_idx,indent+1);
				val_idx=val->next; val=dgaf_json_get(it,val_idx);
			}
			printf("]\n");
		}
		else
		if(nxt->t==OBJECT)
		{
			printf("{\n");
			nam_idx=nxt->v.o.nam; nam=dgaf_json_get(it,nam_idx);
			val_idx=nxt->v.o.val; val=dgaf_json_get(it,val_idx);
			while(nam&&val)
			{
				printf("%.*s = ",nam->v.s.len,nam->v.s.ptr);
				dgaf_json_debug_print(it,val_idx,indent+1);
				nam_idx=nam->next; nam=dgaf_json_get(it,nam_idx);
				val_idx=val->next; val=dgaf_json_get(it,val_idx);
			}
			printf("}\n");
		}
		else
		if(nxt->t==STRING)
		{
			printf("%.*s\n",nxt->v.s.len,nxt->v.s.ptr);
		}
		else
		{
			printf("%d=\n",idx);
		}
	}
}


// allocate a new value and return its index, it is never 0
int dgaf_json_alloc(struct dgaf_json_state *it)
{
	struct dgaf_json_value * v;
	if( it->values_len+1 >= it->values_siz ) // check for space
	{
		it->values_siz=it->values_siz+1024;
		it=(struct dgaf_json_state *)realloc( (void*)it , it->values_siz * sizeof(struct dgaf_json_value) );
		if(!it) { return 0; }
	}
	v=dgaf_json_get(it,it->values_len);
	v->next=0;
	v->t=NONE;
	v->v._._=0;
	
	return it->values_len++;
}

// we can only free the top most allocated idx
int dgaf_json_free(struct dgaf_json_state *it,int idx)
{
	if( idx == (it->values_len-1) )
	{
		it->values_len--;
	}
	return idx;
}

// load a new file ( need to fix for pipes )
int dgaf_json_load_file(struct dgaf_json_state *it,char *fname)
{
FILE * fp;
int fs;

	it->data="";
	it->data_len=0;

	fp = fopen( fname , "rb" );
	if(!fp) { return 0; }
	
	fseek( fp , 0 , SEEK_END );
	fs = ftell( fp );
	fseek( fp , 0 , SEEK_SET );

	it->data = malloc( fs+1 );
	it->data[fs] = 0;
	it->data_len = fs+1;

	fread( it->data , fs, 1, fp);

	fclose( fp );
	
	return fs;
}

// peek for whitespace or comments
int dgaf_json_parse_peek_white(struct dgaf_json_state *it)
{
	char c1=it->data[ it->parse_idx ];
	char c2=it->data[ it->parse_idx+1 ];
	if( (c1==' ') || (c1=='\t') || (c1=='\n') || (c1=='\r') )
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
int dgaf_json_parse_peek_punct(struct dgaf_json_state *it,char *punct)
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

// skip whitespace and comments
void dgaf_json_parse_skip_white(struct dgaf_json_state *it)
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
int dgaf_json_parse_skip_white_punct(struct dgaf_json_state *it,char *punct)
{
	dgaf_json_parse_skip_white(it);

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
	
	dgaf_json_parse_skip_white(it);

	return ret;
}

int dgaf_json_parse_step(struct dgaf_json_state *it)
{
	const char *cp;
	char *cd;
	char c;	
	struct dgaf_json_value *nxt;
	int idx=0;

	for( cd=it->data+it->parse_idx; cd<it->data+it->data_len ; cd++ , it->parse_idx++ )
	{
		c=*cd;
		for( cp="\\\"\n,;:='[]{}/*" ; *cp ; cp++ )
		{
			if( *cp == c ) // found a special char
			{
				c=0;
				break;
			}
		}
		if(c==0) // special
		{
			if(idx) // just return string built so far
			{
				return idx;
			}
			else // alloc nxt and return it
			{
				idx=dgaf_json_alloc(it);
				nxt=dgaf_json_get(it,idx);
				if(nxt==0) { return 0; }
				nxt->t=STRING;
				nxt->v.s.ptr=cd;
				nxt->v.s.len=1;
				it->parse_idx++;
				return idx;
			}
		}
		else // start or continue a generic string
		{
			if(idx==0) // start
			{
				idx=dgaf_json_alloc(it);
				nxt=dgaf_json_get(it,idx);
				if(nxt==0) { return 0; }
				nxt->t=STRING;
				nxt->v.s.ptr=cd;
				nxt->v.s.len=1;
			}
			else // continue
			{
				nxt->v.s.len++;
			}
		}
	}
	return idx;
}

int dgaf_json_parse_string(struct dgaf_json_state *it,int lst_idx,char *term)
{
	struct dgaf_json_value *lst=dgaf_json_get(it,lst_idx);

	int idx;
	struct dgaf_json_value *nxt;
	char c;
	char *cp;

	while(1)
	{
		idx=dgaf_json_parse_step(it);
		nxt=dgaf_json_get(it,idx);
		if(nxt==0){return lst_idx;} // out of data
		c=*(nxt->v.s.ptr);
		lst->v.s.len+=nxt->v.s.len;
		dgaf_json_free(it,idx);
		for( cp=term ; *cp ; cp++ )
		{
			if( c==*cp ) // a termination char
			{
				return lst_idx;
			}
		}
	}
	
	return 0;
}

int dgaf_json_parse_name(struct dgaf_json_state *it)
{
	dgaf_json_parse_skip_white(it);

	int lst_idx=dgaf_json_parse_step(it);
	struct dgaf_json_value *lst=dgaf_json_get(it,lst_idx);
	if(lst==0){return 0;} // out of data

	int idx;
	struct dgaf_json_value *nxt;
	char c;

	while(1)
	{
		idx=dgaf_json_parse_step(it);
		nxt=dgaf_json_get(it,idx);
		if(nxt==0){return lst_idx;} // out of data
		c=*(nxt->v.s.ptr);
		lst->v.s.len+=nxt->v.s.len;
		dgaf_json_free(it,idx);
		if(c=='"') { return lst_idx; } // end on closing quote
		if( dgaf_json_parse_peek_white(it) ) { return lst_idx; } // stop at whitespace
		if( dgaf_json_parse_peek_punct(it,"=:") ) { return lst_idx; } // stop at punct
	}
	
	return 0;
}

int dgaf_json_parse_object(struct dgaf_json_state *it,int lst_idx)
{
	struct dgaf_json_value *lst=dgaf_json_get(it,lst_idx);
	struct dgaf_json_value *nam;
	struct dgaf_json_value *val;

	lst->t=OBJECT;
	lst->v.o.nam=0;
	lst->v.o.val=0;
	lst->v.o.len=0;

	int nam_idx;
	int val_idx;

	while(1)
	{
		nam_idx=dgaf_json_parse_name(it); if(!nam_idx){return 0;}
		if( dgaf_json_parse_skip_white_punct(it,"=:") != 1 ) { return 0; } // required assignment
		val_idx=dgaf_json_parse_value(it); if(!val_idx){return 0;}
		dgaf_json_parse_skip_white_punct(it,",;"); // optional , seperators

		if( lst->v.o.len==0) // first
		{
			lst->v.o.nam=nam_idx;
			lst->v.o.val=val_idx;
			lst->v.o.len++;
			nam=dgaf_json_get(it,nam_idx);
			val=dgaf_json_get(it,val_idx);
		}
		else // chain
		{
			nam->next=nam_idx;
			val->next=val_idx;
			lst->v.o.len++;
			nam=dgaf_json_get(it,nam_idx);
			val=dgaf_json_get(it,val_idx);
		}

		if(1==dgaf_json_parse_skip_white_punct(it,"}")) { break; } // closer
	}
	
	return lst_idx;
}

int dgaf_json_parse_array(struct dgaf_json_state *it,int lst_idx)
{
	struct dgaf_json_value *lst=dgaf_json_get(it,lst_idx);
	struct dgaf_json_value *val;

	lst->t=ARRAY;
	lst->v.o.val=0;
	lst->v.o.len=0;

	int val_idx;

	while(1)
	{
		val_idx=dgaf_json_parse_value(it);
		dgaf_json_parse_skip_white_punct(it,",;"); // optional , seperators

		if( lst->v.a.len==0) // first
		{
			lst->v.a.val=val_idx;
			lst->v.a.len++;
			val=dgaf_json_get(it,val_idx);
		}
		else // chain
		{
			val->next=val_idx;
			lst->v.a.len++;
			val=dgaf_json_get(it,val_idx);
		}


		if(1==dgaf_json_parse_skip_white_punct(it,"]")) { break; } // closer
	}
	
	return lst_idx;
}

int dgaf_json_parse_value(struct dgaf_json_state *it)
{
	dgaf_json_parse_skip_white(it);
	
	int idx=dgaf_json_parse_step(it);
	struct dgaf_json_value *nxt=dgaf_json_get(it,idx);
	if(nxt==0){return 0;}
	char c=*(nxt->v.s.ptr);

	switch( c )
	{
		case '{' :
			return dgaf_json_parse_object(it,idx);
		break;
		case '[' :
			return dgaf_json_parse_array(it,idx);
		break;
		case '\'' :
			return dgaf_json_parse_string(it,idx,"`");
		break;
		case '"' :
			return dgaf_json_parse_string(it,idx,"\"");
		break;
		case '`' :
			return dgaf_json_parse_string(it,idx,"`");
		break;
	}		
	return dgaf_json_parse_string(it,idx,"\n");
}


int dgaf_json_parse(struct dgaf_json_state *it)
{
	it->parse_idx=0;
	it->parse_char=0;
	it->parse_line=0;
	int first=0;
	int idx;
	while( idx=dgaf_json_parse_value(it) )
	{
		if(first==0){first=idx;}
		struct dgaf_json_value *nxt=dgaf_json_get(it,idx);
		if(nxt==0){return 0;}
	}
	
	dgaf_json_debug_print(it,first,0);
	
	return 1;
}

#endif

