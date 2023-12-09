
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

* HJSON			https://hjson.github.io/

	Close, very close but has a python indent style multi line strings.

* relaxedjson	http://www.relaxedjson.org/

	Close, but unquoted naked strings end on any whitespace, 
	which_is_only_useful_if_you_type_like_this.

None of the above flaws are deal breakers, they are all steps in the 
right direction but none of them remove any of JSONs questionable 
edges and none of them have a binary string plan. 

If we are going to mess with what JSON is then we should take the 
opportunity to demand UTF8 and forbid BOMs I say UTF8 but what I really 
mean is 7bit ASCII with possible UTF8 strings. We do not force strict 
UTF8 encoding which allows us to include binary in strings.

Demanding UTF8, breaks us away from JSON slightly, this means a 
UTF16/UTF32 JSON file is an invalid DJON file. However nobody really 
wants or expects a UTF16 or UTF32 JSON file and a UTF8 BOM is the 
easiest way to poison text.

---

Numbers
=======

All numbers are text representations of 64bit IEEE floating point 
numbers. They wll be parsed into 64bit floats when scanned and that is 
all the information you can expect to get out of them. The following 
exceptional exceptional floating point values are map stringified like 
so so.

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
not start with {}[],:= or +-0123456789. or any of the three keywords, 
true/false/null. These strings are terminated at \n and are whitespace 
trimmed. 

Keywords
--------

Same as JSON so "true", "false" and "null" are special, except we 
ignore case so "null" can also be "Null" or "NULL".


Objects
-------

Allow = as a synonym for :

An assignment operator must be present as it stops object definitions 
getting out of sync between the keys and values but commas are optional 
between key value pairs, in fact they are considered whitespace in this 
situation so multiple commas will be ignored.

