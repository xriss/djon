
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

	Close, but has a python style multi line strings.

* relaxedjson	http://www.relaxedjson.org/

	Close, but unquoted naked strings end on any whitespace, 
	which_is_only_useful_if_you_type_like_this.

None of the above flaws are deal breakers, they are all steps in the 
right direction but none of them remove any of JSONs questionable 
edges.

If we are going to mess with what JSON is then we should take the 
opportunity to demand UTF8 and forbid BOMs I say UTF8 but what I really 
mean is 7bit ASCII with possible UTF8 strings. We do not force strict 
UTF8 encoding which allows us to include binary in strings.

Demanding UTF8, breaks us away from JSON slightly, this means a 
UTF16/UTF32 JSON file is an invalid DJON file. However nobody really 
wants or expects a UTF16 or UTF32 JSON file and a UTF8 BOM is the 
easiest way to poison text.

---

Some notes.
-----------

Allow multi line quoted strings with any quote so "'` or single line 
quote-less naked strings and not require , as a separator everywhere.

However since we are adding backtick strings here we can also make them 
special and *allow nulls* and not provide \ escape processing within 
them so everything inside a ` string is raw data. Not having to worry 
about escapes is often useful when inputting data. Remember the file 
*must* be UTF8 but does not have to be valid UTF8 so any stream of 
bytes can be placed in such a string.

In order to deal with the need for a possible backtick inside these 
strings a double "``" can be used to open and subsequently close them 
with any number of other quotes inside them eg some examples:

	`this is a string`
	``this is a string``
	`"`this is a string`"`
	`'"`this is a string`'"`
	`'"'`this is a string`'"'`

this gives us range to pick a quote style that will not be found inside 
the string and treat everything inside as data, even \0 values. 
Remember we will not deal with back slashes as escapes inside these 
strings so what you see is what you get.

true, false and null keywords will ignore case.

Allow = as a synonym for : as they are both used in javascript and I 
often use the wrong one. An assignment operator must be present as it 
stops object definitions getting out of sync between the keys and 
values. Relaxed json echoes this reasoning here 
http://www.relaxedjson.org/musings/other-considerations

Allow a bit more flexibility in the numbers, so hex and + signs and not 
complaining about 1.e2 .1e2 missing numbers after or before the decimal 
point.

Including inf or nan is not a good idea, it adds more reserved words 
instead we just use null for nan and a huge number for inf. I think 
9e999 is a good choice for Inf and -9e999 for -Inf, being the bigest 
number you can write in the shortest string that will not fit in a 
double and become Inf. JSON does not specify what information can be 
stored in a number although double is implied, DJON explicitly demands 
that numbers are 64bit IEEE floats and must be parsed within these 
limits. This means we can get away with 9e999 parsing as Inf, without 
it being a special case.

If you want to complain that null is not a number they oh boy is your 
head going to explode when you find out what a Nan is.
