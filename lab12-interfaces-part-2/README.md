# Interfaces - part 2

Let's build the policy in this directory and use it to replace the one we built in the last lab - this will enable our `testcat` tool to write to the console and let us focus on the task of reading the testprog data file through an interface:

```
[james@selinux-dev lab12-interfaces-part-2]$ make -f /usr/share/selinux/devel/Makefile testcat.pp
Compiling targeted testcat module
Creating targeted testcat.pp policy package
rm tmp/testcat.mod.fc tmp/testcat.mod
[james@selinux-dev lab12-interfaces-part-2]$ sudo semodule -r testcat
libsemanage.semanage_direct_remove_key: Removing last testcat module (no other testcat module exists at another priority).
[james@selinux-dev lab12-interfaces-part-2]$ sudo semodule -i testcat.pp
[james@selinux-dev lab12-interfaces-part-2]$ sudo testcat /var/testprog/testprg.txt
Segmentation fault (core dumped)
```

This might not look any better - we're still getting a segmentation fault - however if you query the audit log again, you'll see a reduced number of AVC denials for `testcat`:

```
[james@selinux-dev lab12-interfaces-part-2]$ sudo grep testcat /var/log/audit/audit.log | grep AVC
...
type=AVC msg=audit(1736176260.532:844): avc:  denied  { map } for  pid=16049 comm="testcat" path="/usr/bin/testcat" dev="vda4" ino=25196736 scontext=unconfined_u:unconfined_r:testcat_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:testcat_exec_t:s0 tclass=file permissive=0
```

Great - that means the console access must be working! So how do we allow `testcat` to access `testprog` data? For security reasons we don't want `testcat` to use the `testprog` domain, and remember in reality this would be a more complex application with access to lots of other things. Whilst we could specifically allow access to the data type of `testprog`, the best way is to present an interface in the `testprog` policy that other applications (such as `testcat`) can use.

Look at the new policy for `testprog` in this directory - only the `testprog.if` file is changed from before - however note it might look familiar to the output seen when we ran our `seshowif` commands previously. You will also note that the two allow rules look almost identical to the ones in the main `testprog` policy, except that in place of `testprog_t` there is a **$1** - this tells the macro to substitute the first argument it is passed with this value. The rules are also write centric in the main `testprog` policy but here you will notice they are read centric - this is only to allow others to read our data, not to modify it. In this way we can allow any arbitrary domain to access this type. Let's build and install the new policy module for `testprog`:

```
[james@selinux-dev lab12-interfaces-part-2]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev lab12-interfaces-part-2]$ sudo semodule -r testprog
[sudo] password for james:
libsemanage.semanage_direct_remove_key: Removing last testprog module (no other testprog module exists at another priority).
[james@selinux-dev lab12-interfaces-part-2]$ sudo semodule -i testprog.pp
```

There won't be much to see at this stage - however we should now generate a new version of the policy module for `testcat` to take advantage of our newly defined interface - proceed to lab13 to do this...

