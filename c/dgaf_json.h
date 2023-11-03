

enum dgaf_json_enum
{
	NONE,
	NUMBER,
	BOOL,
	STRING,
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
	int   siz;
};

union dgaf_json_union
{
	struct dgaf_json_string a;
	struct dgaf_json_number b;
	struct dgaf_json_bool   c;
	struct dgaf_json_null   d;
};

struct dgaf_json_value
{
	enum dgaf_json_enum t;
	union dgaf_json_union v;
};

#ifdef DGAF_JSON_CODE



#endif

