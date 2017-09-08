# Finding examples

We know now that we want to create our own bespoke policy for our application, and that the output from sealert, whilst helpful, isn't going to achieve for us the level of security we really want. However SELinux is complex and there are many many permissions to have to deal with, so how can we make our lives easier.

Well first of all, we know that good SELinux policies already exist, and are backed by Red Hat (or whatever distribution you are using). Whilst no-one is infallible, not even big corporates, it does make sense to start by looking at how other confined applications are secured with SELinux rather than reinventing the wheel, or spending hours iterating through audit.log, fixing denials to see what the next one is (believe me you'll end up doing this anyway!).

Many applications have common features and so it's a matter of choosing one that isn't too complex to analyze, but that helps us get our application working in a confined context. We know that (and even if we didn't have the source code we could see the behaviour when we ran it as unconfined) testprog needs:

* To display test on the terminal
* To read it's config file from /etc
* To write it's PID file to /var/run
* To write it's data to /var/testprog
* To write to syslog

There are many suitable examples, but I have picked for these labs the **ntp** package - this does all these things, and more, but not so much as to be overly complex for us to understand and analyze.

# A note on disassembly

The beauty of open-source software is that, even in the case of the loaded SELinux policies, you can find the source code and see how they were written. You can actually extract and disassemble the loaded polcies without the source code, but doing so will give you details of the policy without any comments or the macros that make writing them easier (more on macros later). If you wish to to disassemble the existing policy, you can do so as follows:

```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install policycoreutils-devel checkpolicy
...
[james@selinux-dev2 selinux-testprog]$ sudo semodule -E ntp
Module 'ntp' does not exist at the default priority '400'. Extracting at highest existing priority '100'.
[james@selinux-dev2 selinux-testprog]$ sedismod ntp.pp
Reading policy...
libsepol.policydb_index_others: security:  0 users, 3 roles, 95 types, 2 bools
libsepol.policydb_index_others: security: 1 sens, 1024 cats
libsepol.policydb_index_others: security:  62 classes, 0 rules, 0 cond rules
libsepol.policydb_index_others: security:  0 users, 3 roles, 95 types, 2 bools
libsepol.policydb_index_others: security: 1 sens, 1024 cats
libsepol.policydb_index_others: security:  62 classes, 0 rules, 0 cond rules
Binary policy module file loaded.
Module name: ntp
Module version: 1.11.0


Select a command:
1)  display unconditional AVTAB
2)  display conditional AVTAB
3)  display users
4)  display bools
5)  display roles
6)  display types, attributes, and aliases
7)  display role transitions
8)  display role allows
9)  Display policycon
0)  Display initial SIDs

a)  Display avrule requirements
b)  Display avrule declarations
c)  Display policy capabilities
l)  Link in a module
u)  Display the unknown handling setting
F)  Display filename_trans rules

f)  set output file
m)  display menu
q)  quit

Command ('m' for menu):  
```

Through the menu system you can explore the policy, but I personally found it non-intuitive. Rather than do that, let's take advantage of open-source and get hold of the source code.

# Finding the source code

On EL7, you need to file and download a copy of the **source** rpm for **selinux-policy**. At the time of writing this was **selinux-policy-3.13.1-166.el7.src.rpm** which I downloaded from Red Hat.

Once you have a copy of the rpm extract the contents or install it as you prefer, also remembering to extract the tarballs of source code within the rpm:

```
[james@selinux-dev2 selinux-testprog]$ mkdir refpolicy
[james@selinux-dev2 selinux-testprog]$ cd refpolicy/
[james@selinux-dev2 refpolicy]$ rpm2cpio ../selinux-policy-3.13.1-166.el7.src.rpm | cpio -idm
21976 blocks
[james@selinux-dev2 refpolicy]$ for tarball in *.tgz; do tar -xzf $tarball; done
[james@selinux-dev2 refpolicy]$ cd serefpolicy-contrib-3.13.1/
[james@selinux-dev2 serefpolicy-contrib-3.13.1]$ ls ntp*
ntp.fc  ntp.if  ntp.te
```

Fantastic! There's our raw policy files for ntp just as we wanted. In the next lab we'll start to understand this policy.

# Finding documentation

As well as accessing the source, all distribution provided modules usually come with good documentation. This won't show you the source code for the policy, but it will tell you how the policy is intended to be used, what booleans and other aspects are available, and so on. To ensure the documentation is on your system:

```
[root@selinux-dev2 ~]# subscription-manager repos --enable=rhel-7-server-optional-rpms
[root@selinux-dev2 ~]# yum -y install selinux-policy-doc lynx
[root@selinux-dev2 ~]# /etc/cron.daily/man-db.cron
```

The last line rebuilds the man page database so you can search for any new pages installed.

You can search for selinux related man pages as follows:

```
[root@selinux-dev2 ~]# man -k _selinux
```

Or view the policy documentation:

```
[root@selinux-dev2 ~]# lynx /usr/share/doc/selinux-policy/html/index.html
```
