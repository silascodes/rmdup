# rmdup
A little program I wrote to clean up duplicate MP3 files that people keep giving me, but it doesn't care what the file type is. Will delete any
duplicates of a file in the entire directory tree (if recurse is specified) otherwise just in the specified directory. Duplicates are detected 
using the MD5 hash algorithm. Be careful, you can run safe mode first if you like so nothing is deleted, duplicates are just listed.

Contains Windows specific code at this point in time :(

## Build & Install
This program has a project file for CodeBlocks which can be used to build the program, alternately you could build it from the command line for 
whatever compiler pretty easily I imagine. If you want to "install" it I suppose you could put it in some directory that is in your path.

## Usage
	rmdup [options] <path>

## Examples
	# basic usage (will delete and list)
	rmdup C:\my\directory
	
	# get help
	rmdup -h
	
	# use safe mode (don't delete, just list)
	rmdup -s C:\my\directory
	
	# use recursive mode (recurse through subdirectories, deleting all duplicates of a file anywhere)
	rmdup -r C:\my\directory

	# recursively look for duplicates, but don't delete just list
	rmdup -rs C:\my\directory
	
	# use quiet mode, don't output anything
	rmdup -q C:\my\directory

## Supported parameters:

- -r		recurse through subdirectories
- -s		safe mode, don't actually remove any files
- -q		quiet mode, don't output anything
- -h		show help screen

Uses MD5 implementation from http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
