#!/bin/env python
import random
from random import choice
import string
import xxhash

def ips(size):
    max_int_32 = 2147483647
    retval = []
    while size > 0:
        size = size - 1
        retval.append(random.randint(0, max_int_32))
    return retval

def key():
    length=8
    chars=string.letters + string.digits
    return ''.join([choice(chars) for i in range(length)])


def brokers_for_key(servers, key32bit):
    # (2**0) | 2**1 | 2**2 | 2**3 | 2**4 | 2**5 | 2**6 | 2**7 | 2**8 | 2**9 | 2**10
    k1 = key32bit & 2047 # lower 10 bits
    # (2**11) | 2**12 | 2**13 | 2**14 | 2**15 | 2**16 | 2**17 | 2**18 | 2**19 | 2**20 | 2**21
    k2 = key32bit & 4192256
    # (2**22) | 2**23 | 2**24 | 2**25 | 2**26 | 2**27 | 2**28 | 2**29 | 2**30 | 2**31 | 2**32
    k3 = key32bit & 8585740288
    # print "Keys: ", (k1,k2,k3)
    server_size = len(servers)
    return (servers[k1 % server_size],
            servers[k2 % server_size],
            servers[k3 % server_size])

def any_brokers_equals(s1,s2,s3):
    return s1 == s2 or s1 == s3 or s2 == s3;

# start with 100 servers
cluster_servers = ips(10)
# print "servers: ", cluster_servers
for i in range(0, 1000):
    producer_key = key()
    rpc_chain_path = xxhash.xxh32(producer_key).intdigest()
    server_selection = brokers_for_key(cluster_servers, rpc_chain_path)
    (s1,s2,s3) = server_selection
    if any_brokers_equals(s1,s2,s3):
        print "Found a collision: ", (s1, s2, s3), ",  rpc_chain: ", rpc_chain_path, ", producer key: ", producer_key
