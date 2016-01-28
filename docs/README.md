# Design Doc

The api for sending data to the service is via a regular HTTP POST request.

# Format of a message

Effively same as kafka: http://kafka.apache.org/documentation.html#messageformat

* Big difference: The same format as is on wire, is used on the payload of the
**HTTP POST**

* Because the primary target platform is x86, all ints are encoded as
little endian. *NOT* network order or big endian. This is true for
the **message length** and the **crc** check.

```

message length : 4 bytes (value: 1+4+n)
"magic" value  : 1 byte
crc            : 4 bytes
payload        : n bytes

```

* If you think of **ONE** message being in this format, the payload of the POST
will simply be the same format, just catted together.

There will be hooks in the API to verify each message, but the idea is to do
**ZERO** work on the server in terms of verification **unless** you tell the api
endpoint to do some checking.

# API

## Data API /v/d POST

**/0/d?t=fbar&p=0&v=0&to=-1&**

```txt
t  - topic                       [REQUIRED] -
p  - partition                   [REQUIRED] -
to - timeout in milliseconds     [OPTIONAL] - default -1
v  - verify                      [OPTIONAL] - default  0
```

### /0/ this is the version!!!

/0/? means version 0 of the API

### t=topic

name of the topic

### p=[0-65535] - int16_t - will start with this for now.

Added to name of file

### v=0 or v=1               - not yet supported

This verifies each request. So it is slow. It will do a crc32 on the payload

### to=-1 or to=milliseconds - not yet supported

timeout to expire these entires - not even sure it makes sense to have,
just copying the ProducerRequest.java code from kafka.

## Session API /v/s GET

**/0/s**

You get a Cookie: session=12341234

You have to renew this cookie every N - still being designed


## Headers

Content-Type: xsmrf - for the content type MUST be true
Cookie: session=123412341234 - not implemented yet
