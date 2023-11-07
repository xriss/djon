
DJON or DGAFJSON is JSON but we DGAF.
=====================================

I honestly can not believe that I have to write this library, it feels 
like reinventing the wheel but apparently I live in a world full of 
square wheels.

This project will parse valid UTF8 JSON and only UTF8 and will allow 
literal \0 characters.

This project will also parse lax JSON that is easier to edit by hand, 
so some invalid JSON will parse without complaint.

This project will contain code written in C and Lua and Javascript that 
will do the parsing reasonably fast.

It is intended for configuration files, that are mostly edited by human 
hand and where strict JSON is often a curse. However it would not be 
unreasonable to use this for machine communication mostly to take 
advantage of the ``can contain every possible character`` long strings 
without escapes that we are adding.

Exactly where and how to reduce the strictness is where this differs, 
the following should be the template for decisions.

- Why do you not require , between elements in arrays? Because we DGAF.

- Why do you allow accidental characters at the end of a file? Because 
we DGAF.

- Why do you allow slightly malformed floating point numbers? Because 
we DGAF.

And so on. Because we DGAF.

---

So pretty much JSON5? or HJSON yup? Almost.

JSON5 is the closest to what I want so far but it still bitches about 
so many things that I do not consider it humane and it does not handle 
strings as well as I would like.

Mostly I want to allow multi line quoted strings with any quote so "'` 
or single line quote-less strings and not require , as a separator 
everywhere.

This fixes most things.

However since we are adding backtick strings here we can also make them 
special and *allow nulls* and not provide \ escape processing within 
them so everything inside a ` string is raw data. Not having to worry 
about escapes is often useful when inputting data. Remember the file 
*must* be utf8.

In order to deal with the need for a possible ` inside such strings a 
double `` can be used to open and subsequently close them with any 
number of other quotes inside them eg `'` `"` `''` `""` `'"` `"'` and 
so on as many as you need. As an example `'"`thisisthestring`'"` this 
gives us reasonable range to pick a quote style that will not be found 
inside the string and treat everything inside as data, even \0 values.

Here we are copying lua style long strings but using quote characters 
rather than the sometimes confusing [[ which can naturally occur with 
nested arrays.

Finally allow = as a synonym for : as they are interchangeable and I 
often type the wrong one. An assignment operator must be present as it 
stops object definitions getting out of sync between the names and the 
values.

A simple way to think of this is to take json and only 
require {} [] : operators and dont bitch about the presence or lack of 
, chars.

Allow a bit more flexibility in the numbers, so hex and + signs.

Then we add some special processing of for `` strings and finally a 
mostly safe way of dropping quotes entirely and using \n as the end of 
such strings.

Will formalize this all later...

