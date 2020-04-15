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
- json-c 7fb8d56
- rabbitmq-c v0.10.0
- knot-protocol 891d01d
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

1. `git clone https://github.com/json-c/json-c.git && cd json-c`
2. `git checkout 7fb8d56 && cd ..`
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
2. `git checkout 891d01d ` to checkout to a hash on devel branch.
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


## Running the VirtualThing

Start the daemon:

`./src/thingd`

To see the daemon options:

`./src/thingd --help`

### How to test locally

Start the daemon with the options to indicate the configuration files path:

`./src/thingd -n -c confs/credentials.conf -d confs/device.conf -r confs/rabbitmq.conf`

### How to check for memory leaks and open file descriptors

`valgrind --leak-check=full --track-fds=yes ./src/thingd -n -c `
`confs/credentials.conf -d confs/device.conf -r confs/rabbitmq.conf`


## License

All KNoT VirtualThing files are under LGPL v2.1 license, you can check `COPYING`
file for details.

The `checkpatch.pl` and `spelling.txt` files, on hooks folder, are under the
Linux Kernel's GPL v2 license.
