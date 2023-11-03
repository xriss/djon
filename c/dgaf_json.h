

enum dgaf_json_enum
{
	NUL,
	NUMBER,
	BOOL,
	STRING,
	ARRAY,
	OBJECT,
};


struct dgaf_json_null
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
	struct dgaf_json_null   _;
	struct dgaf_json_number n;
	struct dgaf_json_bool   b;
	struct dgaf_json_string s;
	struct dgaf_json_array  a;
	struct dgaf_json_object o;
};

struct dgaf_json_value
{
	enum  dgaf_json_enum  t;
	union dgaf_json_union v;
};

struct dgaf_json_state
{
	char *data;
	int   data_len;

	struct dgaf_json_value *values;
	int                     values_len;
	int                     values_siz;
	int                     values_root;
};

extern struct dgaf_json_state * dgaf_json_setup();
extern int dgaf_json_alloc(struct dgaf_json_state *it);

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

	it->values=((struct dgaf_json_value *)it);
	// the first few slots are taken by this state structure (hax)
	it->values_len=(1+(sizeof(struct dgaf_json_state)/sizeof(struct dgaf_json_value)));

// the json root
	it->values_root=dgaf_json_alloc(it);
	
	return it;
}

// allocate a new value and return its index
int dgaf_json_alloc(struct dgaf_json_state *it)
{
	struct dgaf_json_value * v;
	if( it->values_len+1 >= it->values_siz ) // check for space
	{
		it->values_siz=it->values_siz+1024;
		it=(struct dgaf_json_state *)realloc( (void*)it , it->values_siz * sizeof(struct dgaf_json_value) );
		if(!it) { return 0; }
	}
	v=it->values + it->values_len ;
	v->t=NUL;
	
	return it->values_len++;
}


#endif

