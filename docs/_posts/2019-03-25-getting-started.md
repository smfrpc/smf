---
title: "Getting started with the smf rpc framework"
layout: post
---

Cross-posted from
[https://nwat.xyz/blog/2019/03/22/getting-started-with-the-smf-rpc-framework/][nwat].

In this post we are going to take a high-level look at the [smf rpc
framework][smf], and walk through the process of creating a minimal
client/server example. This post will show to setup a build tree, and bootstrap
the client and server to exchange messages. We'll then briefly expand the
example to include server-side state management.

The goal of the smf project is provide a high-performance rpc implementation for
the [Seastar][seastar] framework.  Seastar is a C++ framework for building
high-performance server applications. It uses a shared-nothing server design and
makes extensive use of modern C++ to provide a futures-based programming model.
And while becoming acquainted with Seastar is necessary to build real-world
smf-based applications, the content in this post should be understandable by
anyone familiar with modern C++.

Here are some helpful links:

* [smf github project][smf]
* [smf documentation / news][smf-docs]
* [smf starter project][smf-starter]

# Service definition

The smf framework makes extensive use of Google's [flatbuffers serialization
library][fbs] as a structured container for the network protocol as well as for
defining rpc service interfaces. In this post we are going to be putting
together a basic key/value store service, and we'll start by examining the
structure of the service in terms of the message types. The following is the
contents of a file called `kvstore.fbs` that contains the `PutRequest` message
type holding a single string-based key/value pair. Our service is very simple,
so the response message for a `PutRequest` is empty---though as we'll see smf
provides a built-in mechanism for signaling basic return codes.

Finally at the bottom of the file is the `rpc_service` called `MemoryNode`
containing a single rpc method `Put` that takes a `PutRequest` and responds with
a `PutResponse`.

```cpp
namespace kvstore.fbs;

table PutRequest {
  key: string;
  value: string;
}

table PutResponse {
}

rpc_service MemoryNode {
  Put(PutRequest):PutResponse;
}
```

Once message types and service interfaces are defined a tool in the smf
framework is used to perform code generation that produces the C++
implementation of the message types and stub methods for the service interfaces.
Next we'll use the smf-provided cmake tools to invoke code generation within a
cmake-based build.

# Build scaffolding

Below is the complete `CMakeLists.txt` that we are going to use to build the
sample project. The bottom three lines import smf as a sub-directory (e.g. via a git
submodule) and configures smf to build and manage all of its dependencies.

```cmake
cmake_minimum_required(VERSION 3.7.1)
project(smf-kvstore VERSION 0.0.1 LANGUAGES CXX)

set(SMF_MANAGE_DEPS ON CACHE "" INTERNAL FORCE)
set(SMF_BUILD_PROGRAMS OFF CACHE "" INTERNAL FORCE)
add_subdirectory(smf)
```

Next we use the `smfc_gen` macro provided by smf to compile the flatbuffers
definition, and store the resulting source files in the `kvstore_fbs` variable.

```cmake
smfc_gen(CPP TARGET_NAME kvstore_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES kvstore.fbs)
```

And finally we define client and server executables that link against the smf
library and include the source produced by `smfc_gen`.

```cmake
link_libraries(smf::smf)
include_directories(PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_executable(server server.cc ${kvstore_fbs})
add_executable(client client.cc ${kvstore_fbs})
```

In the next two sections we'll work through the creation of the server and the
client. Once the `server.cc` and `client.cc` files have been created, the
project can be compiled and tested:

```bash
cmake .
make

./server
./client # in another terminal
```

Note that smf currently builds its dependencies at _configure time_ which is not
a standard technique and may come as a surprise when first starting with the
project. For example, when initially running `cmake .` to configure the tree,
smf will build Seastar and other dependencies from source. This can take quite a
while to complete, but subsequent builds are fast.

# The rpc server

When the flatbuffers file above is compiled it generates a couple C++ header
files. These should be used in the server and client by including
`"kvstore.smf.fb.h"` (which includes all the other generated files). The
generated code contains the definition of the `MemoryNode` rpc service that we
saw above.

```cpp
class MemoryNode: public smf::rpc_service {
 private:
  std::vector<smf::rpc_service_method_handle> handles_;
 public:
  virtual ~MemoryNode() = default;
  MemoryNode() {
```

Recall that our service included a single `Put` method. The stub for this rpc
method is auto-generated. Our implementation will override this default
implementation.

```cpp
  inline virtual
  seastar::future<smf::rpc_typed_envelope<kvstore::fbs::PutResponse>>
  Put(smf::rpc_recv_typed_context<kvstore::fbs::PutRequest> &&rec) {
    using out_type = kvstore::fbs::PutResponse;
    using env_t = smf::rpc_typed_envelope<out_type>;
    env_t data;
    // User should override this method.
    //
    // Helpful for clients to set the status.
    // Typically follows HTTP style. Not imposed by smf whatsoever.
    // i.e. 501 == Method not implemented
    data.envelope.set_status(501);
    return seastar::make_ready_future<env_t>(std::move(data));
  }
```

Now let's look at our specific server implementation. It starts with a class
that will specialize `MemoryNode` and implement the rpc methods of our key/value
service.

```cpp
#include <seastar/core/app-template.hh>
#include <seastar/core/sharded.hh>
#include <smf/rpc_server.h>
#include "kvstore.smf.fb.h"

class kvstore_service final : public kvstore::fbs::MemoryNode {
  virtual seastar::future<smf::rpc_typed_envelope<kvstore::fbs::PutResponse>> Put(
    smf::rpc_recv_typed_context<kvstore::fbs::PutRequest>&& req) final;
};
```

Next we provide the implementation of `Put`. At a high-level this
method does two things. First, it performs any handling for the
request, and second it creates and returns a response. Notice that the
status value is set to `200`. Every response in smf contains a helpful
status code that can be used by implementations in any way
they see fit.

Notice also that handling of the request is optional. This is because
rpc handlers can be organized into a pipeline within the server, and
handlers may decide to not forward the request to downstream handlers.

In any case, this handler doesn't do much other than print a message with the
key and value that are contained in the request. It also prints the identifier
of the CPU where this request is being handled. In Seastar-based servers, work
is generally sharded across CPUs. For networking applications like smf, Seastar
assigns client connections to CPU cores in round-robin assignments to balance
load.

```cpp
seastar::future<smf::rpc_typed_envelope<kvstore::fbs::PutResponse>>
kvstore_service::Put(smf::rpc_recv_typed_context<kvstore::fbs::PutRequest>&& req)
{
  if (req) {
    LOG_INFO("recv core={} key={} value={}", seastar::engine().cpu_id(),
        req->key()->str(), req->value()->str());
  }

  smf::rpc_typed_envelope<kvstore::fbs::PutResponse> data;
  data.envelope.set_status(200);

  return seastar::make_ready_future<
    smf::rpc_typed_envelope<kvstore::fbs::PutResponse>>(std::move(data));
}
```

Now to finish up the server. The first important bit is
`seastar::sharded<smf::rpc_server> rpc`. The `sharded` object is a special
Seastar object that replicates instances of its template parameter onto each
physical CPU. In this case, we are asking Seastar to replicate the rpc server
across all the cores and to handle connections in parallel.

```cpp
int main(int args, char **argv, char **env)
{
  seastar::app_template app;

  app.add_options()
    ("ip", po::value<std::string>()->default_value("127.0.0.1"), "bind to")
    ("port", po::value<uint16_t>()->default_value(20776), "listen on")
    ;

  seastar::sharded<smf::rpc_server> rpc;
```

Next the application itself is started by calling `rpc.start(args)` which will
setup the service on all cores. Once this bootstrap phase completes the rpc
service established on all cores (via `invoke_on_all`) will be configured to
serve the `kvstore_service` that we built above. Finally each instance of the
service is started.

```cpp
  return app.run_deprecated(args, argv, [&app, &rpc] {
    seastar::engine().at_exit([&] { return rpc.stop(); });

    auto&& cfg = app.configuration();

    smf::rpc_server_args args;
    args.ip = cfg["ip"].as<std::string>().c_str();
    args.rpc_port = cfg["port"].as<uint16_t>();

    return rpc.start(args).then([&rpc] {
      return rpc.invoke_on_all(
          &smf::rpc_server::register_service<kvstore_service>);
    }).then([&rpc] {
      return rpc.invoke_on_all(&smf::rpc_server::start);
    });
  });
}
```

The server can be started using the default arguments by running `./server`. You
should see some output similar to the trace below. I'm running this example on a
node with 8 cores and the log reflects this by printing that an rpc server is
starting 8 times. Notice too the log prefix containing the CPU id `[shard 0..7]`.

```
INFO  2019-03-16 13:26:50,603 [shard 0] smf - rpc_server.cc:73] Starging server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 0] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 1] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 2] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 3] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 4] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 5] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 7] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 6] smf - rpc_server.cc:73] Starting server:rpc_server{args.ip=127.0.0.1, args.flags=0, args.rpc_port=20776, args.http_port=33140, rpc_routes=rpc_handle_router{rpc_service{name=MemoryNode,handles=rpc_service_method_handle{name=Put}}}, limits=rpc_connection_limits{max_mem:2.000 GB, max_body_parsing_duration: 60000ms, res_avail:2.000 GB (2147483648)}, incoming_filters=0, outgoing_filters=0}
INFO  2019-03-16 13:26:50,603 [shard 2] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 3] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 1] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 5] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 4] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 6] smf - rpc_server.cc:94] Starting rpc server
INFO  2019-03-16 13:26:50,603 [shard 7] smf - rpc_server.cc:94] Starting rpc server
```

That's it for the server. Later in this post we'll expand the implementation so
that the server is actually storing the key/value pairs, but right now let's
move on and put together a client that can generate a workload.

# Client driver

The client that we'll walk through below is very basic. It calls the `Put` rpc
endpoint with a few different key/value pairs. The code resembles the server,
but in this case we do not spin-up a connection per core but rather use a single
connection to keep things simple. Like the server, there a couple arguments that
can change the settings if you happen to run the server on a non-default address
or port.

The most notable difference is that a `client` object is created. This
is of type `MemoryNodeClient` and is created when the flatbuffers
definition is compiled.

```cpp
  return app.run(args, argv, [&app] {
    auto&& cfg = app.configuration();
    auto ip = cfg["ip"].as<std::string>().c_str();
    auto port = cfg["port"].as<uint16_t>();

    smf::rpc_client_opts opts{};
    opts.server_addr = seastar::ipv4_addr{ip, port};

    auto client = seastar::make_shared<
      kvstore::fbs::MemoryNodeClient>(std::move(opts));

    seastar::engine().at_exit([client] { return client->stop(); });
```

The client's `connect` method establishes the connection and returns a future.
When that future is complete we can begin invoking the rpc service methods. Here
we build 10 key/value pairs, and in the next code block we call `Put` for
each one.

```cpp
    return client->connect().then([client] {
      boost::counting_iterator<int> from(0);
      boost::counting_iterator<int> to(10);
      return seastar::do_for_each(from, to, [client](int i) mutable {
        seastar::sstring key = fmt::format("key.{}", i);
        seastar::sstring val = fmt::format("val.{}", i);
```

The `PutRequest` object is created and populated with the key/value strings,
then the `Put` method is invoked and the response is logged. After the workload
generator completes, the client and Seastar application is shutdown.

```cpp
        smf::rpc_typed_envelope<kvstore::fbs::PutRequest> req;
        req.data->key = std::move(key);
        req.data->value = std::move(val);

        return client->Put(std::move(req)).then([](auto res) {
          if (res) {
            LOG_INFO("recv core={} status={}",
                seastar::engine().cpu_id(),
                res.ctx->status());
          }
          return seastar::make_ready_future<>();
        });
      });
    }).then([client] {
      return client->stop();
    }).then([] {
      return seastar::make_ready_future<int>(0);
    });
  });
}
```

Now we can see the entire thing in action. First start the server, then run the
client a few times. Each time you run the client you should see something like
the following where we receive the response with a code of `200` (see above in
the server with we set the `200` code).

```
INFO  2019-03-16 08:54:57,229 [shard 0] smf - client.cc:53] recv core=0 status=200
INFO  2019-03-16 08:54:57,229 [shard 0] smf - client.cc:53] recv core=0 status=200
...
```

Since the client is running as a single thread, you should generally only see it
scheduled on core 0 (i.e. `recv core=0` in the log). However, recall that the
server shards its connections across cores. If you examine the log of the
server, you should see that each new connection is handled by a different core.
The first connection is handled on core 0.

```
INFO  2019-03-16 08:54:42,607 [shard 0] smf - server.cc:20] recv core=0 key=key.0 value=key.0.val
INFO  2019-03-16 08:54:42,607 [shard 0] smf - server.cc:20] recv core=0 key=key.1 value=key.1.val
...
```

And subsequent connections are assigned to additional cores until all cores are
assigned connections and the assignment wraps back around to core 0.

```
INFO  2019-03-16 08:54:44,816 [shard 1] smf - server.cc:20] recv core=1 key=key.0 value=key.0.val
INFO  2019-03-16 08:54:44,816 [shard 1] smf - server.cc:20] recv core=1 key=key.1 value=key.1.val
INFO  2019-03-16 08:54:44,816 [shard 1] smf - server.cc:20] recv core=1 key=key.2 value=key.2.val
...
INFO  2019-03-16 08:54:56,616 [shard 7] smf - server.cc:20] recv core=7 key=key.0 value=key.0.val
INFO  2019-03-16 08:54:56,616 [shard 7] smf - server.cc:20] recv core=7 key=key.1 value=key.1.val
INFO  2019-03-16 08:54:56,616 [shard 7] smf - server.cc:20] recv core=7 key=key.2 value=key.2.val
...
INFO  2019-03-16 08:54:56,832 [shard 0] smf - server.cc:20] recv core=0 key=key.0 value=key.0.val
INFO  2019-03-16 08:54:56,832 [shard 0] smf - server.cc:20] recv core=0 key=key.1 value=key.1.val
INFO  2019-03-16 08:54:56,832 [shard 0] smf - server.cc:20] recv core=0 key=key.2 value=key.2.val
...
```

# Managing server state

The server we've built so far doesn't do much other than throw the key/value
bits from the client on the floor. In this section we are going to modify the
server so that the data from the client is stored. We'll keep it simple and only
store the values in memory. This should add just enough detail to our example
project to demonstrate how the sharding works when managing server state.

The first thing is to define a new per-core object that will hold
key/value pairs. This class contains a private `std::unordered_map` to
store client key/value pairs, and a handler for inserting data into
the store. This handler also prints out the core on which it is
running, which will be helpful later when we examine the exact
behavior of the server. Finally, when the server shuts down, we also
print out the contents of the index to examine. We'll see this in
action after we walk through the rest of the code.

```diff
 #include <smf/rpc_server.h>
 #include "kvstore.smf.fb.h"
 
+class kvstore_db {
+ public:
+  seastar::future<> put(seastar::sstring key, seastar::sstring value) {
+    LOG_INFO("handling put core={} key={} value={}",
+        seastar::engine().cpu_id(), key, value);
+    store_.insert(std::make_pair(std::move(key), std::move(value)));
+    return seastar::make_ready_future<>();
+  }
+
+  seastar::future<> stop() {
+    for (auto kv : store_) {
+      LOG_INFO("{}/{}: {} {}", &store_,
+          seastar::engine().cpu_id(),
+          kv.first, kv.second);
+    }
+    return seastar::make_ready_future<>();
+  }
+
+ private:
+  std::unordered_map<seastar::sstring, seastar::sstring> store_;
+};
```

Next we modify the key/value service to track a reference to an instance of the
store class we introduced above. Note that _both_ of these will be sharded
across the cores in the system. We'll see shortly how the rpc service is given
an instance of the store.

```diff
 class kvstore_service final : public kvstore::fbs::MemoryNode {
   virtual seastar::future<smf::rpc_typed_envelope<kvstore::fbs::PutResponse>> Put(
     smf::rpc_recv_typed_context<kvstore::fbs::PutRequest>&& req) final;
+
+ public:
+  kvstore_service(seastar::sharded<kvstore_db>& store) :
+    store_(store)
+  {}
+
+ private:
+  seastar::sharded<kvstore_db>& store_;
 };
```

The other change that is made to the rpc service is to forward the key/value
pair request from the client to the store instance. In order to _balance_ state
across all cores in the system we adopt a simple scheme in which the key is
hashed to a core number. The final piece of this puzzle is the magical
`invoke_on` method that arranges for a particular method to be executed on a
specific core. In this case, we execute the `put` operation on the core that is
responsible for the particular key being stored.

```diff
@@ -14,6 +44,13 @@ kvstore_service::Put(smf::rpc_recv_typed
   if (req) {
     LOG_INFO("recv core={} key={} value={}", seastar::engine().cpu_id(),
         req->key()->str(), req->value()->str());
+
+    seastar::sstring key = req->key()->str();
+    auto hash = std::hash<seastar::sstring>{}(req->key()->str());
+    auto cpu_id = hash % seastar::smp::count;
+
+    store_.invoke_on(cpu_id, &kvstore_db::put,
+        std::move(key), req->value()->str());
```

The store is also treated as a sharded object by Seastar, creating an instance
per physical core.

```diff
@@ -35,10 +72,12 @@ int main(int args, char **argv, char **e
       ;
   }
 
+  seastar::sharded<kvstore_db> store;
   seastar::sharded<smf::rpc_server> rpc;
 
-  return app.run_deprecated(args, argv, [&app, &rpc] {
+  return app.run_deprecated(args, argv, [&app, &store, &rpc] {
     seastar::engine().at_exit([&] { return rpc.stop(); });
+    seastar::engine().at_exit([&] { return store.stop(); });
```

And finally there are some changes to the setup. The piece to pay attention to
is that when the rpc service is registered, an instance of the store is passed in
which gives it access to the per-core storage service we created at the
beginning of this section.

```diff
@@ -46,9 +85,12 @@ int main(int args, char **argv, char **e
     args.ip = cfg["ip"].as<std::string>().c_str();
     args.rpc_port = cfg["port"].as<uint16_t>();
 
-    return rpc.start(args).then([&rpc] {
-      return rpc.invoke_on_all(
-          &smf::rpc_server::register_service<kvstore_service>);
+    return rpc.start(args).then([&store] {
+      return store.start();
+    }).then([&rpc, &store] {
+      return rpc.invoke_on_all([&store](smf::rpc_server& server) {
+        server.register_service<kvstore_service>(store);
+      });
     }).then([&rpc] {
       return rpc.invoke_on_all(&smf::rpc_server::start);
     });
```

The client and server are ran in the same way as before, and now we can examine
what is happening in the server as the client sends new key/value pairs. As
before we log a message when we first receive a request from the client. In this
case that connection is handled on core 0. The server then hashes the key and
forwards the request to the core that owns that key. In this case the key hashes
to core 5 which is where the key/value pair is stored.

```
INFO  2019-03-17 16:26:02,922 [shard 0] smf - server.cc:51] recv core=0 key=key.0 value=key.0.val
INFO  2019-03-17 16:26:02,922 [shard 5] smf - server.cc:12] handling put core=5 key=key.0 value=key.0.val
```

The same pattern holds for the rest of the requests which are all handled
initially by core 0, and then forwarded to a different core, including core 0
which is also able to store key/value pairs.

```
INFO  2019-03-17 16:26:02,923 [shard 0] smf - server.cc:51] recv core=0 key=key.1 value=key.1.val
INFO  2019-03-17 16:26:02,923 [shard 6] smf - server.cc:12] handling put core=6 key=key.1 value=key.1.val
INFO  2019-03-17 16:26:02,923 [shard 0] smf - server.cc:51] recv core=0 key=key.2 value=key.2.val
INFO  2019-03-17 16:26:02,923 [shard 6] smf - server.cc:12] handling put core=6 key=key.2 value=key.2.val
INFO  2019-03-17 16:26:02,924 [shard 0] smf - server.cc:51] recv core=0 key=key.3 value=key.3.val
INFO  2019-03-17 16:26:02,924 [shard 1] smf - server.cc:12] handling put core=1 key=key.3 value=key.3.val
INFO  2019-03-17 16:26:02,924 [shard 0] smf - server.cc:51] recv core=0 key=key.4 value=key.4.val
INFO  2019-03-17 16:26:02,924 [shard 5] smf - server.cc:12] handling put core=5 key=key.4 value=key.4.val
INFO  2019-03-17 16:26:02,924 [shard 0] smf - server.cc:51] recv core=0 key=key.5 value=key.5.val
INFO  2019-03-17 16:26:02,924 [shard 3] smf - server.cc:12] handling put core=3 key=key.5 value=key.5.val
INFO  2019-03-17 16:26:02,925 [shard 0] smf - server.cc:51] recv core=0 key=key.6 value=key.6.val
INFO  2019-03-17 16:26:02,925 [shard 5] smf - server.cc:12] handling put core=5 key=key.6 value=key.6.val
INFO  2019-03-17 16:26:02,925 [shard 0] smf - server.cc:51] recv core=0 key=key.7 value=key.7.val
INFO  2019-03-17 16:26:02,925 [shard 0] smf - server.cc:12] handling put core=0 key=key.7 value=key.7.val
INFO  2019-03-17 16:26:02,925 [shard 0] smf - server.cc:51] recv core=0 key=key.8 value=key.8.val
INFO  2019-03-17 16:26:02,925 [shard 6] smf - server.cc:12] handling put core=6 key=key.8 value=key.8.val
INFO  2019-03-17 16:26:02,926 [shard 0] smf - server.cc:51] recv core=0 key=key.9 value=key.9.val
INFO  2019-03-17 16:26:02,926 [shard 7] smf - server.cc:12] handling put core=7 key=key.9 value=key.9.val
```

We only added 10 key/value pairs in this example, so it's feasible to
dump out the contents of the index on each core. Below is what that
dump looks like.  We can see that there are 8 different index
instances on each of the eight cores. Note that none of the key/value
pairs happened to hash to cores 0 or 2.

```
INFO  2019-03-19 20:44:33,824 [shard 1] smf - server.cc:19] 0x605000011560/1: key.3 val.3
INFO  2019-03-19 20:44:33,824 [shard 3] smf - server.cc:19] 0x606000011790/3: key.5 val.5
INFO  2019-03-19 20:44:33,825 [shard 5] smf - server.cc:19] 0x604000011600/5: key.6 val.6
INFO  2019-03-19 20:44:33,825 [shard 7] smf - server.cc:19] 0x6020000115b0/7: key.9 val.9
INFO  2019-03-19 20:44:33,825 [shard 5] smf - server.cc:19] 0x604000011600/5: key.0 val.0
INFO  2019-03-19 20:44:33,825 [shard 5] smf - server.cc:19] 0x604000011600/5: key.4 val.4
INFO  2019-03-19 20:44:33,825 [shard 6] smf - server.cc:19] 0x6010000115b0/6: key.8 val.8
INFO  2019-03-19 20:44:33,825 [shard 6] smf - server.cc:19] 0x6010000115b0/6: key.1 val.1
INFO  2019-03-19 20:44:33,825 [shard 6] smf - server.cc:19] 0x6010000115b0/6: key.2 val.2
```

If you found [smf][smf] or [Seastar][seastar] intriguing I encourage you to
check out these projects in more detail. Building high-performance network
servers with these two technologies could not be easier or faster. I'm really
exciting to keep exploring what they have to offer.

Thanks to [Alex](https://twitter.com/emaxerrno) for providing feedback on this post!

[smf]: https://github.com/smfrpc/smf
[fbs]: https://google.github.io/flatbuffers
[seastar]: http://seastar.io
[smf-starter]: https://github.com/smfrpc/smf-getting-started-cpp
[smf-docs]: https://smfrpc.github.io/smf
[nwat]: https://nwat.xyz/blog/2019/03/22/getting-started-with-the-smf-rpc-framework/
