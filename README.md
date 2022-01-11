# ParseR

A parser for the R language. Produces tokens, optionally as a JSON tree.

TODO:

Review code, check for memory leaks potentially.

Optimize, optimize. Currently does 600 lines/sec for large files but can still make faster.

Testing. Tested with 10 different large R scripts but variety is what is needed.

Integration into Python works, still need to do for R ironically.

Need to add extra input params when called from R, R can pass along env packages that are pre-loaded and
not specifically referenced via library(), require() or package::method().

Website/platform in Python-Flask to generate D3 graphs of dependencies. Tested this and it works but very rough script still.
