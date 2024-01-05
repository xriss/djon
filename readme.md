
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
first library with minimal dependecies that compiles well to wasm, so 
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
for use in JS, 16bit strings cause us a bit of a problem in js so wasm 
may actually be the most performant way of dealing with them.

I'm going to assume that other languages I'm less of an expert in will 
also be able to use this C core so no need to rewrite it in other 
languages. I expect wasm to become a dominant way of providing portable 
code in the future so lets see if I am correct.


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

In order to deal with the need for a backtick inside these strings a 
double "``" can be used to open and subsequently close them with any 
number of other quotes inside them eg some examples:

	`this is a string`
	``this is a string``
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
ignores case so "null" can also be "Null" or "NULL".

These keywords require a deliminator character to follow them, eg 
whitespace or punctuation, so "NULLL" will never be recognized as the 
keyword NULL followed by an extra 'L'

The DJON string "NULL" on its own will be parsed correctly despite not 
having a terminator after the keyword as it is a C style string with an 
explicit 0x00 following it. The 0x00 is one of the terminator 
characters we check for and we should null terminate our strings even 
if the string also contains nulls. This is Lua style string rules.


Objects
-------

Allow = as a synonym for :

An assignment operator must be present as it stops object definitions 
getting out of sync between the keys and values but commas are optional 
between key value pairs, in fact they are considered whitespace in this 
situation so multiple commas will be ignored.

