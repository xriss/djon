

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
};

extern struct dgaf_json_state * dgaf_json_setup();
extern struct dgaf_json_value * dgaf_json_get(struct dgaf_json_state *it,int idx);
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

	// the first few slots are taken by this state structure (hax)
	it->values_len=(1+(sizeof(struct dgaf_json_state)/sizeof(struct dgaf_json_value)));

// the json root
	it->values_root=dgaf_json_alloc(it);
	
	return it;
}

// get a value by index
struct dgaf_json_value * dgaf_json_get(struct dgaf_json_state *it,int idx)
{
	if( idx == 0 )              { return 0; }
	if( idx >= it->values_siz ) { return 0; }
	return ((struct dgaf_json_value *)(it)) + idx ;
}

// allocate a new value and return its index, it is never 0;
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

#endif

