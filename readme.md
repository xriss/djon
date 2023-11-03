I honestly can not believe that I have to write this library, if feels 
like reinventing the wheel but apparently I live in a world full of 
square wheels.

This project will parse valid UTF8 JSON.

This project will also parse lax JSON that is easier to edit by hand.

This project will contain code written in C and Lua and Javascript that 
will do the parsing fast.


So pretty much JSON5? yup? Almost.

JSON5 is the closest to what I want so far but it still bitches about 
so many things that I do not consider it humane and it does not handle 
strings as well as I would like.

So Technically nothing like JSON5 :)


Mostly I want to allow multi line quoted strings with any quote so "'` 
or single line quote-less strings and not require , as a separator 
everywhere.

This fixes most things.

However since we are adding backtick strings here we can also make them 
special and not provide \ escape processing within them so everything 
inside a ` string is raw data. Not having to worry about escapes is 
often useful when inputting data.

In order to deal with the need for a possible ` inside such strings 
multiple ` can be used to open and subsequently close them. IE `` this 
can contain ` and ends with a `` As a side effect of this rule ` 
strings may *not* be empty.

Finally allow = as a synonym for : as they are interchangeable and I 
often type the wrong one.

Apart from that, the rest is JSON5 like, so better number handling, 
allow hex etc.

Will formalize this later...



My simple plan for parsing in C works like this.

Load entire string assuming utf8.

Split this string on any byte that is <0x20 and include these bytes in 
the resulting array as single character strings. This can be 
done live scanning the original string in C.

This array is then parsed into a tree structure with whitespace 
trimming where appropriate. Again we can just index into the 
original data for all strings, possibly even inserting null terminators 
as we go.

This parsing step is where the magic happens but is a pretty simple 
stepping state machine that should look mostly the same in all 3 target 
languages and might even end up being shared code.

We then return this parsed structure, which combined with the original 
string is a fully parsed json structure.


Lets us see how this works out...
