# Filesystem contexts

Our application never switched contexts on execution because we didn't actually change the context on the file itself. Now there are two ways to do this.

The first is to use the **chcon** command - you would set the context of our binary like this:

```
[james@selinux-dev2 selinux-testprog]$ sudo chcon -t testprog_exec_t /usr/bin/testprog
[james@selinux-dev2 selinux-testprog]$ ls -lZ /usr/bin/testprog
-rwxr-xr-x. root root unconfined_u:object_r:testprog_exec_t:s0 /usr/bin/testprog
```

Initially this looks good - however there is something to be aware of, and that is filesystem relabelling, or context restoring. A full relabel of a filesystem should be a rare occurrence, but you cannot prevent another system administrator unknowingly coming along and restoring the context of all the files in a directory to their defaults. They might do this in a well-meaning effort to fix another problem, or as part of a policy enforcement - either way we have seen in previous labs that the default context of the files in **/usr/bin** is not **testprog_exec_t**:

```
[james@selinux-dev2 selinux-testprog]$ ls -ldZ /usr/bin
dr-xr-xr-x. root root system_u:object_r:bin_t:s0       /usr/bin
[james@selinux-dev2 selinux-testprog]$ ls -lZ /usr/bin/testprog
-rwxr-xr-x. root root unconfined_u:object_r:testprog_exec_t:s0 /usr/bin/testprog
[james@selinux-dev2 selinux-testprog]$ sudo restorecon -v /usr/bin/*
restorecon reset /usr/bin/testprog context unconfined_u:object_r:testprog_exec_t:s0->unconfined_u:object_r:bin_t:s0
```

Obviously if this were to happen on a live server it would break you application the next time it was launched - clearly not good. As a result we must declare the file context as part of the policy we have created. In addition to the **testprog.te** file which remains unchanged in this lab, we no introduce **testprog.fc**:

```
[james@selinux-dev2 lab5-file-contexts]$ cat testprog.fc
/usr/bin/testprog	-- system_u:object_r:testprog_exec_t:s0
```

Now we rebuild the policy as in the previous lab and install it:


