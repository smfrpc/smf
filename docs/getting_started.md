---
layout: page
title: getting started 
---

<p class="message">
  Nice! thanks for trying and don't forget to 
  send the mailing list an email if you get stuck! 
</p>


Currently we have tested the system to work on Fedora, Debian, Ubuntu. 

We require gcc6 or greater.



```bash
git clone https://github.com/senior7515/smf --recursive
cd smf

# alternatively if you didn't clone recursively
#
# git submodule update --init --recursive
#

mkdir build

cd build
cmake ..
make 

```

Please drop us a line and tell us what you are using 
**smf** for. 


