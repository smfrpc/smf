# RPC

High level design of a “services” DSL and code generation based on the Google
Flatbuffers project - a fast cross language serialization format - and seastar
for it’s networking and thread model.

Behind the scenes Flatbuffers is a backing array + a field lookup table. Every
method call gets looked up on a vtable as an offset into the underlying array.
`struct`s and native (ints, doubles, bools) types are inlined. Although
Flatbuffers offers a C++, C#, C, Go, Java, JavaScript, PHP, and Python
serialization format, this proposal only focuses on the standardization of a
header and payload format, plus the seastar code generation and services
integration. This proposal doesn’t require any fundamental changes to any
seastar core functionality other than merging with, and extending the existing
RPC found in /seastar/rpc/*.{hh,cc}. Schema Definition

The schema definition is the full schema supported by Flatbuffers. The types
supported by the services framework, however, only operates on Flatbuffers
`Table`s. A table is a class in C++ that can contain pointer fields. This is the
main difference between a struct and a Table in flatbuffers terminology. To have
Table types is a requirement of the IDL (Flatbuffers) for both input (arguments)
and outputs (returned values). Example

```cpp
namespace smf_gen.fbs.rpc;

table Request { name: string; }
table Response { name: string; }

rpc_service SmfStorage {
    Get(Request):Response;
}
```
Generated code

The above IDL generates a client and a server per `rpc_service` as such (please
see full generated code here):

```cpp

class SmfStorageStorage: public smf::rpc_service { …
    virtual future<smf::rpc_envelope> Get(smf::rpc_recv_context rec); };
Smfs SmfStorageStorageClient: public smf::rpc_client { …
    future<smf::rpc_recv_ctx_t<Response>> GetSend(smf::rpc_envelope req); };

```

# High level design

The high level design for smf (this rpc system) is inspired by
facebook::wangle libraries with a usability improvement over regular RPC, i.e.:
just the method call by having hooks/callbacks at different stages of the
request lifecycle. The idea is that you can have hooks to deny or approve a
request as it traverses through the request pipeline.

```cpp
struct rpc_incoming_filter {
  future<std::experimental::optional<rpc_recv_context>>
  apply(rpc_recv_context ctx) = 0
  };
```

![alt text](services.png "Service architecture")


We want to support this for any number of “services” registered with the
`rpc_server` that runs this services. To achieve that there has to be someone
responsible for accepting or rejecting the requests handles. The current design
allows a user to register an infinite number of services, since the alternative
would be to run in multiple ports. This is true even if the services from from
different files/modules (generated or not). To achieve that, services register
with a router.

```cpp
class rpc_handle_router {
    bool can_handle_request(const uint32_t &request_id,
                            const flatbuffers::Vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>>);

    void register_service(std::unique_ptr<rpc_service> s);

    future<rpc_envelope> handle(rpc_recv_context recv);

    void register_rpc_service(rpc_service *s);
};
```


The router is in fact very similar to the current seastar rpc system method function
dispatch by id(int). However the main design difference is that we should keep
the same id=same_handle even through multiple code generation phases, i.e.: when
you extend the schema. To achieve this the register services uses a simple XOR
hashing algorithm. Each method has an id + metadata, same with the service.

```cpp
struct rpc_service {

    virtual const char *service_name() const = 0;

    virtual uint32_t service_id() const = 0;

    virtual std::vector<rpc_service_method_handle> methods() = 0;
};


struct rpc_service_method_handle {
    const char *method_name;
    const uint32_t method_id;
    fn_t apply;
};
```



Given these 2 interfaces, we can come up with a request id.
`request_id = service->service_id() ^ method->method_id()`
This is how the requests are
tracked. Consistent requests_ids after multiple generations

```cpSmf_service SmfStorageStorage {
    Put(Request):Response; // new method Put() added before the old method
    Get(Request):Response;
}
```

To keep the same `request_id` and
not break existing old clients, the `method_id` is simply the `crc_32` of the
`method name`. This is also true for service_id being the `crc_32` of the service
name. This guarantees consistent `request_id`’s allowing for the evolution of
services without breaking existing client’s api. Tie it all together, how does
it work:

```cpp
    bool rpc_handle_router::can_handle_request(request_id, header_map);
```

The last part we have yet to explain is how does request lookup work on
the server side.
Assuming a fully parsed request (see low level details section), we have a
`request_id` that is set by the client, which we generate. The service driver,
aka the `rpc_server.h` will perform a map lookup of the
`request_id ( XOR( service_id, method_id ) )`
and determine if we have a function handler for it or not. If we
do, we simply call:

```cpp
    future<rpc_envelope>  rpc_handle_router::handle(rpc_recv_context recv);
```

To handle the actual method call.
[Here are fully worked out examples]()


# Frame format

The frame format is simply a 12 byte (4 int32_t) header and a payload.

This is in fact similar how the current rpc framework works for seastar,
after frame
negotiation. This fixed format makes it very easy to work w/ the existing
abstractions of seastar input_stream<char>::read_exactly(12). The reason for
this format also being generated by flatbuffers is that this ensures
compatibility w/ other languages such as Python, Ruby, Java which is a major
goal for even building this proof of concept, without having to manually
replicate the struct alignment and bit shifting to construct a header and a
payload in the right format. At it’s core, the header is a tuple of:
`(payload size, bit flags, checksum)`
The payload, just ships around a byte array. There are 2
extra fields in the payload.

```cpp

struct Payload{
    ...
    meta: uint
    dynamic_headers: [DynamicHeader] .
};

```

The headers are used for tracing (ala Google Dapper) as well as extensibility,
i.e.: supporting json as the payload. All values are **binary**, the keys are
the only plain text.
The meta field is used on the client side
to send the request_id and on it’s way back it’s used to report status of a call
(when a call has no returned payload). Together, the header and payload are
called the “envelope format” Frame Format [full flatbuffers]():

```cpp
enum Flags:uint (bit_flags) {
    CHECKSUM,
    VERIFY_FLATBUFFERS,
    COMPRESS_SNAPPY
}


struct Header {
    size: uint;
    flags: Flags = 0; // bit flags. i.e.: 2,4,8,16…
    checksum: uint;
};

table Payload {
    meta: uint
    dynamic_headers: [DynamicHeader];
    body: [ubyte];
}


```

# Compiler / Codegen

The compiler is based on the flatbuffers::Parser, which makes it easy to extend
since we get a fully parsed structure including “rpc_service”, so the only thing
to do is to actually output the string. The parsing, tokenization, validation is
left up to the flatbuffers::Parser class. The code generation is only a few
hundred lines of code and can be found here.

# Metrics

The prototype will also include the
famous High Dynamic Range Histogram (HDR Hist) from Gil Tene’s High to track the
latency distribution of RPC requests. Future extensions of the RPC planned to
have Google dapper style tracing for RPC calls which I’ve already implemented on
a proprietary system at concord.io and can easily port.

## Similarities with existing rpc

This is very similar to cap'n'proto. In fact look at the future API used and
returned. However, the networking mechanism used is seastar and the
serialization mechanism is flatbuffers. This project just glues them together.

**No serialization**. There is no manual or codegen serialization code. In fact the
“serialization” is done at the client side, aligning the bytes in a way that the
receiving party doesn’t have to do any work other than a pointer cast to your
type:

```cpp
    auto t = (MyType*)payload->mutable_body();
    t->name(); // or any other method

```

Internally this works in a similar way to a std::vector<uint8_t>, where
every time you save a field, it will try to write it, and if it ran out of
scape, it will realloc to a new location so that it’s all on a single byte array
before sending to the server.
