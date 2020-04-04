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
- rabbitmq-c v0.10.0
- knot-protocol v02.01
- libmodbus v3.1.4

*Other versions might work, but aren't officially supported*


## How to install dependencies:

`$ sudo apt-get install automake libtool libssl-dev valgrind`

### Install libell

The Embedded Linux Library (ELL) provides core, low-level functionality for
system daemons.
To install libell, you have to follow the instructions below:

1. `git clone git://git.kernel.org/pub/scm/libs/ell/ell.git`
2. `git checkout 0.18` to checkout to version 0.18
3. Follow instructions on `INSTALL` file

### Install json-c

json-c provides helpers functions to manipulate JSON datas.
To install json-c lib, you have to follow the instructions below:

1. `git clone https://github.com/json-c/json-c/releases/tag/json-c-0.13.1-20180305`
2. `git checkout json-c-0.13.1-20180305` to checkout to version 0.13.1
3. Follow instructions on `README.md` file

### Install rabbitmq-c

rabbitmq-c is a C-language AMQP client library for use with v2.0+ of the
[RabbitMQ](http://www.rabbitmq.com/) broker.
After install cmake, install rabbitmq-c. You have to follow the instructions
below to install it:

1. `git clone https://github.com/alanxz/rabbitmq-c`
2. `git checkout v0.10.0` to checkout to version 0.10.0
3. Follow instructions on `README.md` file

### Install KNoT Protocol

KNOT Application layer protocol library provides the application layer messages
definition for exchanging messages between KNoT Nodes (KNoT Thing Devices),
KNoT Gateway and KNoT Apps.
To install KNoT Protocol, you have to follow the instructions below:

1. `git clone git@github.com:CESARBR/knot-protocol-source.git`
2. `git checkout KNOT-v02.01` to checkout to version 02.01
3. Follow instructions on `README` file

### Install libmodbus
libmodbus is a free software library to send/receive data according to the
Modbus protocol. To install libmodbus, you must follow the instructions:

1. Grab the release package for version 3.1.4 from libmodus website
	`https://libmodbus.org/releases/libmodbus-3.1.4.tar.gz`
2. Extract it with `tar -xzvf libmodbus-3.1.4.tar.gz`
3. Change into the extracted folder with `cd libmodbus-3.1.4`
4. Finally, build and install libmodbus with
	`./configure && make && sudo make install`

## Building

Run `./bootstrap-configure` and then `make`


## License

All KNoT VirtualThing files are under LGPL v2.1 license, you can check `COPYING`
file for details.

The `checkpatch.pl` and `spelling.txt` files, on hooks folder, are under the
Linux Kernel's GPL v2 license.
