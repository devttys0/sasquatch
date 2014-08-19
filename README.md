About
=========

A modified unsquashfs utility that attempts to support as many hacked-up vendor-specific SquashFS implementations as possible.

If the vendor has done something simple like just muck a bit with the header fields, this tool should sort it out.

If the vendor has made changes to the underlying LZMA compression options, or how these options are stored in the compressed data blocks, this tool will attempt to resolve such customizations via a brute-force method.

Very beta.

Prerequisites
=============

You need a C/C++ compiler, plus the liblzma and zlib libraries (if installing from a package repo, install the development packages).
