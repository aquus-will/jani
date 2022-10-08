# Read Me

This project refers to a master's degree deliverable of a compiler for a reconfigurable environment.

The work is all detailed in: https://www.dcce.ibilce.unesp.br/spd/pubs/Diss-10-WLima.pdf (_portuguese_)

The main article about it is in: https://www.dcce.ibilce.unesp.br/spd/pubs/ISCC2009.pdf (_english_)

## Install

Just run the `make` command on your machine and the build should work fine.

The Makefile was created for Windows platform, so, if you're working on another OS, you might want to edit it in the `clean` section.

All object files should be inside the directory `obj`.

## Usage

Once compiled, you can run JaNi without any arguments (and the you'll be prompted for an input file) or specifying which `class` you want to compile.

Any compilation errors should appear in the same command prompt.

If sucessful, a file with extension `.bin` will be created, which is the binary intended for Nios II. Other files should be generated inside directory `output`, which are the temporary data files for code analysis, specially the graphs.

## Contributing

PRs accepted, but make sure you've understanded its goal by reading the mentioned dissertation and/or the IEEE article.

You may wanna check, also, how Java bytecodes are organized nowadays and whether the project should be evolved from the present state or updated to match some new standards.

As hints for contribution, these can be mentioned (not limited to them, though):
- code optmization evaluating the results of the intermediate graphs;
- extension of bytecodes supported (check `globals.c`, at `is_implemented` array);
- extension of compiler arguments (check `jani.c`, function `process_arg`).

## License

Author: Willian dos Santos Lima - [@aquus-will](https://github.com/aquus-will)

Distributed and Parallel Systems Group - [GSPD](https://www.dcce.ibilce.unesp.br/spd/pindex.php)

UNESP - Sao Paulo State University - Sao Jose do Rio Preto - Brazil

_Since 2007_
