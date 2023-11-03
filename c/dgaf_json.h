

enum dgaf_json_enum
{
	NONE,
	NUMBER,
	BOOL,
	STRING,
	ARRAY,
	OBJECT,
};


struct dgaf_json_null
{
	int null;
};

struct dgaf_json_number
{
	double num;
};

struct dgaf_json_bool
{
	int bool;
};

struct dgaf_json_string
{
	char *str;
};

struct dgaf_json_array
{
	char *str;
};

struct dgaf_json_object
{
	char *str;
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
	enum dgaf_json_enum   t;
	union dgaf_json_union v;
};

#ifdef DGAF_JSON_CODE



#endif

