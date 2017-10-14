---
layout: page
title: getting started 
---

<p class="message">
  Nice! thanks for trying and don't forget to 
  send the mailing list an email if you get stuck! 
</p>


The process of compiling the project including
system level dependencies is automated via 
ansible. 

Currently we have tested the system to work on Fedora25. 
It is also known to compile on Ubuntu with gcc6 and above.

```bash

# assuming you are in the smf folder
cd meta/

source source_ansible_bash

ansible-playbook -K playbooks/devbox_all.yml

# wait a while for seastar and boost to compile
# ... a bit more...

cd .. # back to smf

./debug # for debug builds
./release # for release builds and packaging .deb & .rpm files


```

That's about it! 


We have provided a vagrant file for additional testing
if you are in a non-linux system. Though be warned, 
sometimes Vagrant gets stuck and ICE's the compiler. 
Try running one more time before filing a github issue.

```bash


cd $(git rev-parse --show-toplevel)/misc
./provision_vagrant.sh


```

Please drop us a line and tell us what you are using 
**smf** for. 


