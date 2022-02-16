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
- json-c v0.14-20200419
- rabbitmq-c v0.10.0
- knot-protocol 891d01d
- libmodbus v3.1.4
- libplctag v2.5.0

Test:
- check v0.10.0

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

### Install KNoT Cloud SDK in C

KNoT Cloud SDK in C is a client-side library that provides an AMQP
abstraction to the KNoT Cloud for C applications.

1. `git clone git@github.com:CESARBR/knot-cloud-sdk-c.git`
2. `git checkout 1bdb2dd` to checkout to a hash
3. Follow instructions on `README.md` file

### Install KNoT Protocol

KNOT Application layer protocol library provides the application layer messages
definition for exchanging messages between KNoT Nodes (KNoT Thing Devices),
KNoT Gateway and KNoT Apps.
To install KNoT Protocol, you have to follow the instructions below:

1. `git clone git@github.com:CESARBR/knot-protocol-source.git`
2. `git checkout ead9e66 ` to checkout to a hash on devel branch.
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

### Install libplctag
libplctag is an open source C library for Linux, Windows and macOS uses
EtherNet/IP or Modbus TCP to read and write tags in PLCs.
To install libplctag, you must follow the instructions:
1. `git clone git@github.com:libplctag/libplctag.git`
2. `git checkout v2.5.0` to checkout to version v2.5.0
3. Follow instructions on `BUILD.md` file
4. Install lib files with `sudo make install`

## Building

Run `./bootstrap-configure` and then `make`


## Running the VirtualThing

Start the daemon:

`./src/thingd`

To see the daemon options:

`./src/thingd --help`

### How to test locally

Start the daemon with the options to indicate the configuration files path:

`./src/thingd -n -c confs/credentials.conf -d confs/device.conf -p confs/cloud.conf`

### How to check for memory leaks and open file descriptors

`valgrind --leak-check=full --track-fds=yes ./src/thingd -n -c `
`confs/credentials.conf -d confs/device.conf -p confs/cloud.conf`


## Automated Testing
Run `./bootstrap-configure --with-check`, `make` and then `make check`


## How to run on Docker

You can run the KNoT Virtual Thing on Docker using the configuration files
located under the confs/ folder.

### Building

To build:

`docker build -t thingd .`

You can also use the options --build-arg ENV=value to install other dependencies
versions.

The build arguments available are:

- LIBELL_VERSION (Default: v0.18)
- JSONC_VERSION (Default: 0.14-20200419)
- RABBITMQC_VERSION (Default: v0.10.0)
- KNOT_PROTOCOL_VERSION (Default: 891d01d)
- LIBMODBUS_VERSION (Default: v3.1.4)
- LIBPLCTAG_VERSION (Default: v2.5.0)

### Running

To run the container:

`docker run --network=host -it thingd`

#### Warning

The docker environment has it own set of configuration files.

To edit them you first need to identify the thingd container id:

`docker ps --filter "ancestor=thingd"`

Then you need to open its bash and navigate to the configuration
files path:

`docker exec -it <CONTAINER_ID> /bin/sh`
`cd /etc/knot`

Now you can open and edit using vi:

`vi <file>`

## License

All KNoT VirtualThing files are under LGPL v2.1 license, you can check `COPYING`
file for details.

The `checkpatch.pl` and `spelling.txt` files, on hooks folder, are under the
Linux Kernel's GPL v2 license.
