sasquatch
=========

A modified unsquashfs utility that attempts to support as many hacked-up vendor-specific SquashFS implementations as possible.

Very beta.

Notes
=====

If the vendor has done something simple like just muck a bit with the header fields, this tool should sort it out.

Commonly, changes are made to the underlying LZMA compression, most commonly relating to LZMA property fields and/or how LZMA property fields are stored. Currently this tool works against all the SquashFS variations in the Firmware-Mod-Kit project, but a future feature will be to brute-force the property fields and LZMA header to better support these types of variations.
