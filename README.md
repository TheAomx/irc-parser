IRC Parser  [![Build Status](https://travis-ci.org/TheAomx/irc-parser.svg?branch=master)](http://travis-ci.org/TheAomx/irc-parser)
==========
This is a reentrant and asynchronous IRC parser written in C. It is capable of
parsing messages from IRC servers and clients. It is modeled after Ryan Dahl's
HTTP Parser library and was originally designed to be used with libuv for
RethinkIRCd.

Documentation
-------------
The best and most recent documentation can be found inline in the header file
([here](https://github.com/TheAomx/irc-parser/blob/master/irc_parser.h))

I'll develop a method to automate the abstraction of the inline documentation
and parse it into the README here and also to dedicated a third party doc site.
This is far from the highest of priorities right now though.

Important note
------------------
 * This is an optimized version of JosephMoniz parser. It fixes some bugs in the parser and has some more test cases, that check for error states.
