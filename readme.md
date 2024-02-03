
![underconstruction](underconstruction.gif)

DJON is JSON but we DGAF.
=========================

DJON is a UTF8 only relaxed superset of JSON. DJON supports round trip 
comments, numbers are explicitly 64bit floats and strings can contain 
raw binary data.

There are some similar projects but none of them fixes enough things, 
we should try and "fix" everything or not bother changing anything.

* JSON5			https://json5.org/

	Complains about too many things to be human edited and no naked 
	strings.

* relaxedjson	http://www.relaxedjson.org/

	Close, but unquoted naked strings end on any whitespace, 
	which_is_only_useful_if_you_type_like_this.

* HJSON			https://hjson.github.io/

	Close, very close but has a python indent style multi line strings.

None of the above flaws are deal breakers, they are all steps in the 
right direction but none of them remove any of JSONs questionable edges 
and none of them have a binary string plan. They also have a javascript 
first mindset which is not so good for other languages. DJON is a C 
first library with minimal dependencies that compiles well to wasm, so 
is as portable as possible without needing specific language rewrites.

If we are going to mess with what JSON is then we should also take the 
opportunity to demand UTF8 and forbid BOMs I say UTF8 but what I really 
mean is 7bit ASCII with possible UTF8 strings. We do not force strict 
UTF8 encoding which allows us to include binary bytes, even 0, in 
strings.

Demanding UTF8, breaks us away from JSON slightly, a UTF16/UTF32 JSON 
file is an invalid DJON file. However nobody really wants or expects a 
UTF16 or UTF32 JSON file and a UTF8 BOM is the easiest way to poison 
text in fun and unpredictable ways.

So with this mild incompatibility in mind, any valid json file encoded 
in UTF8 without a BOM is also valid DJON and will be parsed by this 
library.

This library is primarily written in a self contained C header style 
file ( https://github.com/nothings/single_file_libs ) with enough 
flexibility to run in memory constrained systems.

Possibly the only issue I can see is the explicit use of doubles as the 
number type which can be a problem on really old hardware and may cause 
number parsing issues.

This C file can be linked explicitly eg with Lua or compiled to wasm 
for use in JS, 16bit string issues (DJON is 8bit and may contain 
invalid UTF8) cause us a bit of a problem in JS so wasm may actually be 
the best way of dealing with them.

I'm going to assume that other languages I'm less of an expert in will 
also be able to use this C core so no need to rewrite it in other 
languages. I expect wasm to become a dominant way of providing portable 
code in the future so lets see if I am correct.


---

Format
======

This uses C style data so || is a logical OR and && is a logical AND.

To show optional characters we use the idiom of "a" || "" with "" 
meaning an empty string.

We also use ' or " interchangeably as string deliminator where in C ' 
is explicitly a single char. Mostly this is so we can use "'" or '"' 
without needing backslashes.


DJON:

A single 8bit string consisting of.

	WHITESPACE
	VALUE
	WHITESPACE


VALUE:

Any valid data type.

	OBJECT || ARRAY || STRING || NUMBER || BOOLEAN || NULL


NULL:

An explicit keyword.

	"NULL" || "null" || "Null"


TRUE:

An explicit keyword representing the 1 state of a boolean.

	"TRUE" || "true" || "True"


FALSE:

An explicit keyword representing the 0 state of a boolean.

	"FALSE" || "false" || "False"


BOOLEAN:

Must be one of two possible keywords.

	TRUE || FALSE


NUMBER:

	'-' || '+' || ""
	HEXADECIMAL || DECIMAL


HEXADECIMAL:

	'0'
	'x' || 'X'
	HEXADECIMAL_DIGITS


DECIMAL:

	( DECIMAL_DIGITS && DECIMAL_FRACTION ) || DECIMAL_FRACTION
	DECIMAL_EXPONENT || ""


DECIMAL_FRACTION:

	'.'
	DECIMAL_DIGITS


DECIMAL_EXPONENT:

	'e' || 'E'
	'+' || '-' || ""
	DECIMAL_DIGITS


DECIMAL_DIGITS:

A repeating stream of one or more.

	'0' || '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9'


HEXADECIMAL_DIGITS:

A repeating stream of one or more.

	DECIMAL_DIGITS || 'A' || 'a' || 'B' || 'b' || 'C' || 'c' || 'D' || 'd' || 'E' || 'e' || 'F' || 'f'

	
PUNCTUATION:

Characters used for data structure.

	'{' || '}' || '[' || ']' || ':' || '=' || ','


STRING:

Strings can contain 0x00 bytes and non UTF8 sequences so may need to be 
considered possible binary data when dealing with a language like 
java script where strings must be UTF16.

	STRING_NAKED || STRING_QUOTED


STRING_NAKED:

Must not begin with

	'+' || '-' || '.' || DECIMAL_DIGITS || "/*" || "//" || WHITESPACE || '`' || '"' || "'" || PUNCTUATION || NULL || TRUE || FALSE

Which is to say that it can not be a valid start to any other possible 
value.

Ends with

	'\n'

Any trailing WHITESPACE at the end will be trimmed for human 
legibility.


STRING_QUOTED:

Begins with

	'"' || "'" || '`' || LONG_QUOTE

Ends with the exactly same opening quote string and can contain all 
other byte values except for the quote string.

Escape values eg '\n' are only parsed inside strings that are contained 
in ' or " but not in ` or LONG_QUOTE.

Possible escapes are

	'\b'			=	0x08
	'\f'			=	0x0C
	'\n'			=	0x0A
	'\r'			=	0x0D
	'\t'			=	0x09
	'\uXXXX'		=	0xXXXX in UTF8 encoding
	'\uXXXX\uDCXX'	=	0xXXXXXX surrogate pair in UTF8 encoding
	'\?'			= 	the '\' is removed leaving only the '?'

The hexadecimal digits may be upper or lowercase.

As an edge case, If the 4 hexadecimal digits following "\u" is 
malformed then we will stop at the first non hexadecimal character and 
use the number parsed so far. Eg with "\uG" since "G" is not valid it 
would remain in the string and the \u would be replaced with an 0x00 
byte. Similarly "\u20G" would become " G" even though it is also 
technically invalid.

A \ followed by any other character, will simply have the \ removed and 
the second character  will remain, this will work with any character 
although it is mostly to allow quotes and \ characters within a string.

Note the surrogate pair encoding due to JSON being a UTF16 string, 
UTF16 surrogate pairs must be used to reach characters outside of that 
16bit range which we then convert back to UTF8.


LONG_QUOTE:

If you are wondering why this is so complex it is so we can generate a 
quote that will not occur in a given string, this way we can use 
literal strings with no escape character processing and have support 
for full binary data inside strings.

	'`'
		A repeating stream of one or more.
			"'" || '"' 
	'`'

Note, there must be at least one quote or double quote inside the two 
back ticks.

WHITESPACE:

	COMMENT || ' ' || '\n' || '\r' || '\t' 

COMMENT:

	COMMENT_LINE || COMMENT_STREAM

COMMENT_LINE:

	'//'
		A stream of any characters except '\n'
	'\n'

COMMENT_STREAM:

Standard C style comments that can not be nested.

	'/*'
		A stream of any characters except '*/'
	'*/'

ARRAY:

	'['
		Repeating the following for each array value, commas are 
		optional and trailing commas are ignored. Each value must be 
		separated by a comma or some whitespace.
		
			WHITESPACE || ""
			VALUE
			WHITESPACE || ","

	WHITESPACE || ""
	']'

OBJECT:

	'['
		Repeating the following for each object key value pair, commas 
		are optional and trailing commas are ignored. Each key value 
		pair must be separated by a comma or some whitespace. key value 
		pairs must be separated by either a ":" or a "="
		
			WHITESPACE || ""
			KEY
			WHITESPACE || ""
			":" || "="
			WHITESPACE || ""
			VALUE
			WHITESPACE || ","

	WHITESPACE || ""
	']'

KEY:

Keys have the same quote rules as strings but slightly different naked 
rules and keys unlike strings, must be valid UTF8 sequences.

	KEY_NAKED || STRING_QUOTED


KEY_NAKED:

Must not begin with

	'+' || '-' || '.' || DECIMAL_DIGITS || "/*" || "//" || WHITESPACE || '`' || '"' || "'" || PUNCTUATION || NULL || TRUE || FALSE

Which is to say that it can not be a valid start to any other possible 
value.

Ends with

	':' || '='

Any trailing WHITESPACE at the end will be trimmed for human 
legibility.

---

Bytes
======

DJON is a full byte ( 0x00 to 0xff inclusive ) stream with no special 
text mode processing of these bytes. Windows user beware, although it 
will mostly just work.

When parsing we will be talking about 7bit ASCII strings which we will 
place inside "quotes" and 7bit ascii bytes such as the letter 'A' in 
single quotes which is the byte 0x41 in hexadecimal so expect C style 
notation of strings, chars and bytes.

Numbers
=======

All numbers are text representations of 64bit IEEE floating point 
numbers. They will be parsed into 64bit floats when scanned and that is 
all the information you can expect to get out of them. The following 
exceptional exceptional floating point values are map stringified like 
so.

	Infinity	9e999
	-Infinity	-9e999
	Nan			null

9e999 should automagically become Infinity when parsed as it is too 
large to fit into a 64bit float. NaN and -NaN and all the other strange 
NaNs become a null, which is not a number so that seems reasonable.

When converting Numbers to digits we use large integers with positive e 
numbers and decimal fractions with -e numbers. This makes the numbers 
slightly easier to read and explain.

eg 123456789e4 would be 1234677890000 note how with large integer 
numbers the e4 at the end means place 4 zeros here.

eg 0.123456789e-4 becomes 0.0000123456789 note how the e-4 means add 4 
zeros after the decimal point.

Numbers can start with a decimal point omitting the leading 0 so we can 
write .123 instead of 0.123

Numbers may begin with a + sign as well as a - sign and so may 
exponents.

Numbers may be hexadecimal eg 0xdeadbeef remember these are 64bit 
floats which makes for 12 hex digits (48bits) of precision.

When writing numbers we try and use 0s rather than exponents until the 
length of the number becomes unwieldy ( starting around e8 or e-8 ). 
This is for large and small numbers.

Strings
=======

Normal JSON style strings wrapped in ' or " which may contain escapes 
such as \\ \b \f \n \r \t or \u0000 escapes including surrogate pairs. 
These larger unicode numbers will of course be converted to UTF8 
multibyte characters. Any other character after \ will be used as is, 
eg \a is a pointless escaping of the letter a. These strings can also 
contain newline characters, eg wrap across multiple lines.

'\n' explicitly means 0x0a we do not expect or do anything clever with 
CRLF windows style encoding, we are a full byte ( 0x00 to 0xff 
inclusive ) stream with no special processing.

A new type of string wrapped in back ticks, ` these can be binary 
strings and are taken as is, no need for \ anything and any \ in this 
string is just a \ The only special character is the quote used to open 
it which will also be used to close it.

In order to deal with the possible need for a backtick inside these 
strings a pair of backticks containing single and double quotes to 
construct a deliminator that does not occur in the string eg some 
examples:

	`this is a string`
	`"`this is a string`"`
	`'"`this is a string`'"`
	`'"'`this is a string`'"'`

this gives us range to pick a quote that will not be found inside the 
string and treat everything inside as data. Remember the file does not 
have to be valid UTF8 so any stream of bytes can be placed in such a 
string.

Unquoted strings can be used where we are expecting a value as long as 
they would not be mistaken for something else. So a naked string can 
not start with {}[],:= or look like a valid number or any of the three 
keywords, true/false/null. Note a keyword or number must end with 
whitespace or a deliminator character so for instance 100a is 
allowed to start a naked string as would nullly. These strings are 
terminated at \n and are whitespace trimmed. 

Keywords
--------

Same as JSON so "true", "false" and "null" are special, DJON also 
allows some case combinations so that "null" can also be "Null" or 
"NULL".

These keywords require a deliminator character to follow them, eg 
whitespace or punctuation, so "NULLL" will never be recognized as the 
keyword NULL followed by an extra 'L'

Parsing a string "NULL" on its own will be parsed correctly despite not 
technically having a terminator after the keyword as it is a C style 
string with an explicit 0x00 following it. The 0x00 is one of the 
terminator characters we check for and we should null terminate our 
strings even if the string also contains nulls. This is Lua style 
string rules.


Objects
-------

Allow = as a synonym for :

An assignment operator must be present as it stops object definitions 
getting out of sync between the keys and values but commas are optional 
between key value pairs, in fact they are considered whitespace in this 
situation so multiple commas will be ignored.

