# rmdup
A little program I wrote to clean up duplicate MP3 files that people keep giving me, but it doesn't care what the file type is. Will delete any
duplicates of a file in the entire directory tree (if recurse is specified) otherwise just in the specified directory. Duplicates are detected 
using the MD5 hash algorithm. Be careful, you can run safe mode first if you like so nothing is deleted, duplicates are just listed.

Supported parameters:

- -r		recurse through subdirectories
- -s		safe mode, don't actually remove any files
- -q		quiet mode, don't output anything
- -h		show help screen

Uses MD5 implementation from http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
