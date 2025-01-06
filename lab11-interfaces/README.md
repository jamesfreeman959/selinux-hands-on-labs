# Interfaces

So far we have largely ignored the `testprog.if` file that was created as part of the template for our policy. We know that the `.fc` file sets the default file contexts, and the `.te` fileis the actual policy. What about the `.if` file? This file denotes the interfaces for the policy, and in short creates macros that other modules will use to access our resources. So far we have treated `testprog` as totally self contained, but what if we wanted another process to access it's data from a different domain?

To do this, we'll take a copy of the standard `cat` command and set up a special policy called `testcat` that will confine it to its own domain. In real life this would be another daemon or service, but making use of this existing command keeps our lives simple for these labs.

To get started, let's take a copy of the `cat` command, and then set up a special policy module for it that causes it to switch to its own confined context:

```
[james@selinux-dev2 lab11-interfaces]$ sudo cp /usr/bin/cat /usr/bin/testcat
[james@selinux-dev2 lab11-interfaces]$ make -f /usr/share/selinux/devel/Makefile testcat.pp
Compiling targeted testcat module
Creating targeted testcat.pp policy package
rm tmp/testcat.mod.fc tmp/testcat.mod

[james@selinux-dev2 lab11-interfaces]$ sudo semodule -i testcat.pp

[james@selinux-dev2 lab11-interfaces]$ sudo restorecon -v /usr/bin/testcat
restorecon reset /usr/bin/testcat context unconfined_u:object_r:bin_t:s0->unconfined_u:object_r:testcat_exec_t:s0
```

Now let's see what happens when it runs:

```
[james@selinux-dev2 lab11-interfaces]$ sudo testcat /var/testprog/testprg.txt
Segmentation fault (core dumped)
[james@selinux-dev2 lab11-interfaces]$ sudo grep testcat /var/log/audit/audit.log | grep AVC
type=AVC msg=audit(1736175971.335:803): avc:  denied  { read write } for  pid=15978 comm="testcat" path="/dev/pts/0" dev="devpts" ino=3 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:user_devpts_t:s0 tclass=chr_file permissive=0
type=AVC msg=audit(1736175971.335:803): avc:  denied  { read append } for  pid=15978 comm="testcat" path="/dev/pts/0" dev="devpts" ino=3 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:user_devpts_t:s0 tclass=chr_file permissive=0
type=AVC msg=audit(1736175971.335:803): avc:  denied  { read append } for  pid=15978 comm="testcat" path="/dev/pts/0" dev="devpts" ino=3 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:user_devpts_t:s0 tclass=chr_file permissive=0
type=AVC msg=audit(1736175971.335:803): avc:  denied  { read append } for  pid=15978 comm="testcat" path="/dev/pts/0" dev="devpts" ino=3 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:user_devpts_t:s0 tclass=chr_file permissive=0
type=AVC msg=audit(1736175971.335:803): avc:  denied  { map } for  pid=15978 comm="testcat" path="/usr/bin/testcat" dev="vda4" ino=25196736 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:testcat_exec_t:s0 tclass=file permissive=0
```

We can see that the program exited immediately with a segmentation fault, and returned no actual data even though we generated test data in a previous lab. A quick look at the audit log shows us that `testcat` was successfully confined and that it could neither `write` to the terminal, nor could it perform file operations on our `testprog` data directory. Let's first of all fix the terminal errors so that we can focus on interface generation. We know how to do this from our previous labs so move on to the next lab to put this in place...

