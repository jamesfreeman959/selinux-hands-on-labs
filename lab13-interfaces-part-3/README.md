# Interfaces - part 3

Looking at the new policy file in this directory for `testcat`, you will see that it has a new rule in it:

```
testprog_read_data(testcat_t);
```

This references the macro we created in the testprog interface previously. Now we have to build the policy and test it. Note that when you run the `make` command, a number of known locations are searched for interface files to pull into the module. By default on RHEL `/usr/share/selinux/devel/include/` is searched, and if you look in here you will find all the interface files for the base policy. However the actual source for the policy definitions and file contexts are missing, hence we had to extract them manually in an earlier lab to analyze them.

We have not put our new interface file for `testprog` into this central directory, so we have to make sure it's available to the build system. If the interface definition isn't in the aforementioned place, make searches the current directory. This is why we have a full copy of the `testprog` policy source code in here, even though we won't be using it directly in this lab - it is identical to the previous lab's version and is simply needed so that the interface definitions can be found to build the policy for testcat.

Let's build the new policy for testcat and get it loaded:

```
mes@selinux-dev2 lab13-interfaces-part-3]$ make -f /usr/share/selinux/devel/Makefile testcat.pp
Compiling targeted testcat module
/usr/bin/checkmodule:  loading policy configuration from tmp/testcat.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testcat.mod
Creating targeted testcat.pp policy package
rm tmp/testcat.mod.fc tmp/testcat.mod
[james@selinux-dev2 lab13-interfaces-part-3]$ sudo semodule -r testcat
libsemanage.semanage_direct_remove_key: Removing last testcat module (no other testcat module exists at another priority).
[james@selinux-dev2 lab13-interfaces-part-3]$ sudo semodule -i testcat.pp
```

Let's now see the effects of this:

```
[james@selinux-dev2 lab13-interfaces-part-3]$ sudo testcat /var/testprog/testprg.txt | head

Hello World
abcdefghij
Hello World
abcdefghij
Hello World
abcdefghij
Hello World
abcdefghij
Hello World
```

Excellent - we have access to our data. However to show the power of confining a binary to a domain:

```
[james@selinux-dev2 lab13-interfaces-part-3]$ sudo testcat /etc/shadow
testcat: /etc/shadow: Permission denied
```

This might seem a trivial example from the shell, but suppose `testcat` was actually a system service with network ports open to the Internet. Even if not designed to read the shadow password file, suppose some attacker found a vulnerability in the code that enabled us to point `testcat` at the shadow file - SELinux would deny their permission even running with root privileges - hopefully this shows how powerful this is, and how important it is to enable SELinux.

We can also show that we didn't give write access to testcat, so when giving out access to our data we maintain control over who can write to it:

```
[james@selinux-dev2 lab13-interfaces-part-3]$ echo foo | sudo testcat > /var/testprog/testprg.txt
-bash: /var/testprog/testprg.txt: Permission denied
```

With the normal `cat` command this would have overwritten everything in our testprog data file with the word **foo**. 

