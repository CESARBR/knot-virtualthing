# knot-virtualthing

KNoT VirtualThing is part of the KNoT project.
It aims to provide an abstraction to allow certain protocols to interact with a
cloud service using the KNoT AMQP Protocol, by virtualizing a KNoT Device.

## Dependencies

Build:
- build-essential
- autoconf v2.69
- libtool v2.4.6-11
- automake v1.16.1
- pkg-config v0.29.1
- ell v0.18
- json-c v0.13.1

*Other versions might work, but aren't officially supported*

## Building
Run `./bootstrap-configure` and then `make`

## License

All KNoT VirtualThing files are under LGPL v2.1 license, you can check `COPYING`
file for details.

The `checkpatch.pl` and `spelling.txt` files, on hooks folder, are under the
Linux Kernel's GPL v2 license.
