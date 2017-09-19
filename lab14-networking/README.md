# Adding networking

Let's suppose now that someone releases a new version of testprog. This version is identical to the previous in all respects, except that an optional networking feature has now been aded. When turned off (signified by setting `NETWORKPORT=0` in the config file) the program functions exactly as it always did. However when a positive integer is specified in this field, the program opens a TCP socket and listens for traffic on that port. Any data written to the port is echoed back to the client, and also written to the data file in place of our test strings.

For simplicity the new program is called `testprog-net` - this enables us to keep and play with the old version of `testprog` if required. However in real life this might be a version 2 of the code, and simply installed over the top of the existing code.

Let's make a start by building and installing the code itself:

```
[james@selinux-dev2 selinux-testprog]$ cd testprog-net
[james@selinux-dev2 testprog-net]$ make
gcc -o testprog-net testprog-net.c
[james@selinux-dev2 testprog-net]$ sudo make install
mkdir -p /usr/bin
cp testprog-net /usr/bin/testprog-net
cp testprog-net.conf /etc/testprog-net.conf
mkdir -p /var/testprog-net
cp testprog-net.service /etc/systemd/system/testprog-net.service
[james@selinux-dev2 testprog-net]$ sudo systemctl enable testprog-net
Created symlink from /etc/systemd/system/multi-user.target.wants/testprog-net.service to /etc/systemd/system/testprog-net.service.
```

By default networking is disabled in the configuration file, so our existing SELinux policy should work (if you look at the C code, the network section is completely conditional and will not run unless `NETWORKPORT` is > 0). However our filenames have changed so we need to update `testprog.fc` to cover our new files such as `/usr/bin/testprog-net`. Now note before we have been tidying up after ourselves, removing the old `testprog.pp` policy using `sudo semodule -r testprog` before installing a new version. Before we go any further let's try that now:

```
[james@selinux-dev2 testprog-net]$ sudo semodule -r testprog
libsemanage.semanage_direct_remove_key: Removing last testprog module (no other testprog module exists at another priority).
Failed to resolve typeattributeset statement at /etc/selinux/targeted/tmp/modules/400/testcat/cil:21
semodule:  Failed!
```

We haven't seen that before! The issue here is that we create a policy called `testcat` that depends on an interface provided by `testprog`. As a result we can no longer remove the policy and reload it - instead we have to start working with the version number field at the top of the `testprog.te` file. In previous labs this was set to **0.1.10** consistently - clearly this is incorrect behaviour and we need to start bumping up this version number. If you look at the new `testprog.te` that comes with this lab, you'll see the version number has been bumped to **0.2.0**. From now on we'll have to increment it every time we make a change, which is the practise we should be undertaking consistently. 

Also take a look inside `testprog.fc` - previously we specified only one file in each of the first 3 lines - however now we've added wildcards to pick up all variants of testprog. Of course this could pick up too many files if someone else out there wants to create say testprog3 which needs a different policy - some care and planning is required when defining your file contexts not to make them too broad - however for the sake of example in this lab we are demonstrating the transition from specifying individual files to wildcard matching.

Let's go ahead and build and install the new policy, then reset file contexts:

```
[james@selinux-dev2 lab14-networking]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab14-networking]$ sudo semodule -i testprog.pp
[james@selinux-dev2 lab14-networking]$ sudo restorecon -v /usr/bin/testprog*
restorecon reset /usr/bin/testprog-net context unconfined_u:object_r:bin_t:s0->unconfined_u:object_r:testprog_exec_t:s0
[james@selinux-dev2 lab14-networking]$ sudo restorecon -v /etc/testprog*
restorecon reset /etc/testprog-net.conf context unconfined_u:object_r:etc_t:s0->unconfined_u:object_r:testprog_conf_t:s0
[james@selinux-dev2 lab14-networking]$ sudo restorecon -v /var/run/testprog*
restorecon:  lstat(/var/run/testprog*) failed:  No such file or directory
[james@selinux-dev2 lab14-networking]$ sudo restorecon -rv /var/testprog
[james@selinux-dev2 lab14-networking]$
```

We expect our initial run of `testprog-net` to proceed unhindered as networking is disabled in the config file - verify this:

```
[james@selinux-dev2 lab14-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 28449
[james@selinux-dev2 lab14-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality disabled

[james@selinux-dev2 lab14-networking]$ tail -f /var/testprog/testprg-net.txt
Hello World
abcdefghij
Hello World
abcdefghij
Hello World
abcdefghij
Hello World
abcdefghij
^C
[james@selinux-dev2 lab14-networking]$ ps -efZ | grep 28449
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 root 28449 26149  0 10:57 pts/0 00:00:00 sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 root 28450 28449  0 10:57 pts/0 00:00:00 /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
```

Excellent - we're up and running, and in our confined domain!

Now enable networking - change the port in `/etc/testprog-net.conf` from **0** to **8123** and try running the program again:

```
[james@selinux-dev2 lab14-networking]$ sudo sed -i 's/NETWORKPORT=0/NETWORKPORT=8123/g' /etc/testprog-net.conf
[james@selinux-dev2 lab14-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[2] 28512
[james@selinux-dev2 lab14-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8123
Could not create socket
Socket created
bind failed. Error

[2]+  Exit 1                  sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
```

Oh dear - we've failed. By now you will have guessed that SELinux also restricts access to network ports as well as files, tty devices and so on. This is over and above any firewall that you may or may not have running on your server - as with everything else we have explored so far it is an additional layer to the usual protections provided on a Linux system. Very that SELinux did indeed cause this:

```
[james@selinux-dev2 lab14-networking]$ sudo grep testprog-net /var/log/audit/audit.log | grep AVC
type=AVC msg=audit(1505815302.569:3618): avc:  denied  { create } for  pid=28513 comm="testprog-net" scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tclass=tcp_socket
```

As suspected! We were denied from creating a TCP socket. As with our previous labs, we can either manually add allow rules for each socket action required to allow our new program to communicate on the network, or we can see what macros are available to us, and how other reference policies do this...

