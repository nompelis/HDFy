# HDFy
Utilities to turn various file formats into HDF5 equivalents


SUMMARY

This is a collection of utilities to turn a file from its native (text of
binary) format to HDF5 (binary) format. The software will HDFy existing files.

Current (fully and partially) supported formats:

- STL (coming right up)


MOTIVATION

It is really nice to work with text files, because a human can go in there
and "look" or change things with a text editor. But, try doing this to a
large file without actually having to do it programmatically. Also, try
changing anything, especially if the file has numerical data with variable
length formatting (e.g. "1" can be a real number, as can "1.0" and "1.0e+00",
 and will be parsed correctly, but have different ASCII representations).
Eventually one would need to do things programmatically. So, let's just do
it in a cross-platform manner, which can be parallelizeable, etc. Embrace
the "HDF everything"/


LIMITATIONS

Be creative here... Endless.


HOW TO USE

Give me some time, and I will get back to this...


LICENSE

I will think about it, but some kind of "open source" sounds good.


IN 2023/01/06 (initial repository creation)

