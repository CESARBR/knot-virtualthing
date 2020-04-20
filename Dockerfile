FROM alpine:3.10.3 AS builder

# Build arguments
ARG LIBELL_VERSION=0.18
ARG JSONC_VERSION=0.14-20200419
ARG RABBITMQC_VERSION=v0.10.0
ARG KNOT_PROTOCOL_VERSION=891d01d
ARG LIBMODBUS_VERSION=3.1.4

WORKDIR /usr/local

# Install dependencies
RUN apk update && apk add --no-cache make gcc autoconf libtool automake pkgconfig wget file musl-dev linux-headers cmake openssl-dev git

# Install libell
RUN mkdir -p /usr/local/ell
RUN wget -q -O- https://mirrors.edge.kernel.org/pub/linux/libs/ell/ell-$LIBELL_VERSION.tar.gz | tar xz -C /usr/local/ell --strip-components=1
RUN cd ell && ./configure --prefix=/usr && make install

# Install json-c dependency
RUN mkdir -p /usr/local/json-c
RUN wget -q -O- https://github.com/json-c/json-c/archive/json-c-0.14-20200419.tar.gz | tar xz -C /usr/local/json-c --strip-components=1
RUN mkdir -p json-c/build && cd json-c/build && cmake -DCMAKE_INSTALL_PREFIX=/usr .. && make && make install

# Install librabbitmq-c
RUN mkdir -p /usr/local/rabbitmq-c
RUN wget -q -O- https://github.com/alanxz/rabbitmq-c/archive/$RABBITMQC_VERSION.tar.gz | tar xz -C /usr/local/rabbitmq-c --strip-components=1
RUN cd rabbitmq-c && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr .. && make install

# Install knot-protocol
RUN mkdir -p /usr/local/knot-protocol
RUN git clone https://github.com/CESARBR/knot-protocol-source.git /usr/local/knot-protocol
RUN cd knot-protocol && git checkout $KNOT_PROTOCOL_VERSION && ./bootstrap-configure && make install

# Install libmodbus dependency
RUN mkdir -p /usr/local/libmodbus
RUN wget -q -O- https://libmodbus.org/releases/libmodbus-$LIBMODBUS_VERSION.tar.gz | tar xz -C /usr/local/libmodbus --strip-components=1
RUN cd libmodbus && ./configure --prefix=/usr -q && make install

# Copy files to source
COPY ./ ./

# Generate Makefile
RUN PKG_CONFIG_PATH=/usr/lib64/pkgconfig ./bootstrap-configure

# Build
RUN make install


# Copy files from builder
FROM alpine:3.10.3

ENV CRED_CONF_PATH /etc/knot/credentials.conf
ENV DEV_CONF_PATH /etc/knot/device.conf
ENV RABBITMQ_CONF_PATH /etc/knot/rabbitmq.conf

WORKDIR /usr/local

# Copy shared files .so from builder to target image
COPY --from=builder /usr/lib/libell.so* /usr/lib/
COPY --from=builder /usr/lib64/libjson-c.so* /usr/lib/
COPY --from=builder /usr/lib64/librabbitmq.so* /usr/lib/
COPY --from=builder /usr/lib64/librabbitmq.a* /usr/lib/
COPY --from=builder /usr/lib/libknotprotocol.a* /usr/lib/
COPY --from=builder /usr/lib/libknotprotocol.so* /usr/lib/
COPY --from=builder /usr/lib/libmodbus.so* /usr/lib/
COPY --from=builder /usr/lib/libmodbus.la* /usr/lib/

# Copy binary executables
COPY --from=builder /usr/local/src/thingd /usr/bin/thingd

# Copy configuration files
COPY --from=builder /usr/local/confs/credentials.conf $CRED_CONF_PATH
COPY --from=builder /usr/local/confs/device.conf $DEV_CONF_PATH
COPY --from=builder /usr/local/confs/rabbitmq.conf $RABBITMQ_CONF_PATH

# Run
CMD (thingd -n -c $CRED_CONF_PATH -d $DEV_CONF_PATH -r $RABBITMQ_CONF_PATH)
