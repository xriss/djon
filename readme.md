
![underconstruction](underconstruction.gif)

DJON or DGAFJSON is JSON but we DGAF.
=====================================

I honestly can not believe that I have to write this library, it feels 
like reinventing the wheel but apparently I live in a world full of 
square wheels.

There are some close projects but JSON5 still bitches about too many 
things and HJSON has some white space counting style long strings, 
relaxedjson is close but it considers any whitespace to be the end of a 
unquoted string which makes unquoted strings mostly useless.

Finally all of these alternative JSON flavours consider the text 
encoding of the JSON file someone else's problem. This makes including 
non text data in such files an escaping nightmare.

In some ways DJON can be considered a "luafication" of JSON so the 
first rule is.

This project will parse valid UTF8 JSON and only UTF8 encoding. We will 
not care about every UTF8 character being valid (because we DGAF) and 
will even allow literal \0 characters inside strings.

Text encoding is a big bucket of sick and you can not ignore how text 
is encoded. Demanding UTF8 removes many possible encoding issues even 
if it does make it a little bit harder to support in javascript with 
its internal UTF16 strings.

This encoding rule is why DJON is not just JSON, it not only excludes 
some valid JSON data but opens the possibility to include binary data 
within the file. 

This project will contain code written in C and Lua and Javascript that 
will do the parsing reasonably fast.

It is intended for configuration files, that are mostly edited by human 
hand and where strict JSON is often a curse. However it would not be 
unreasonable to use this for machine communication mostly to take 
advantage of the ``can contain every possible character`` long strings 
without escapes that we are adding.

Exactly where and how to reduce the strictness is an art form in 
itself, the following list should be the template for decisions.

- Why do you not require , between values in arrays? Because we DGAF.

- Why do you allow accidental characters at the end of a file? Because 
we DGAF.

- Why do you allow slightly malformed floating point numbers? Because 
we DGAF.

- Why do you allow all characters, even \0 in strings? Because we DGAF.

- And so on. Because we DGAF.

---

Some notes that will be moved and rewritten,

Mostly I want to allow multi line quoted strings with any quote so "'` 
or single line quote-less naked strings and not require , as a 
separator everywhere.

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
strings and that \r\n will remain \r\n.

Here we are copying lua style long strings but using quote characters 
rather than the sometimes confusing double square bracket which can 
naturally occur with nested arrays.

true, false and null keywords will ignore case.

Allow = as a synonym for : as they are both used in javascript and I 
often use the wrong one. An assignment operator must be present as it 
stops object definitions getting out of sync between the keys and 
values. This makes reading unquoted keys much easier as well.  Relaxed 
json echoes this reasoning here 
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

Then we add some special processing of for `` strings and finally a 
mostly safe way of dropping quotes entirely and using \n as the end of 
unquoted strings.

Unquoted strings is the most icky and also most demanded by humans. I 
plan to rewind time when I get an error and try parsing again assuming 
we are dealing with an unquoted string. This is slightly crazy but 
should work most of the time, allowing us to guess correctly with 
unquoted string provided thay are not also valid json.

so

	true

Is a boolean but

	true.

Is a string, since the . at then end is invalid json it triggers string 
parsing.

Will formalize this all later once I have a C reference lib up and 
running...

