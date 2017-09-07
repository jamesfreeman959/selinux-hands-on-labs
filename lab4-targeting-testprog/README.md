# Targeting testprog

By this stage we've established that we can run testprog on our lab system, either from the shell or through systemd. This is all well and good but remember the binary is running in the **unconfined** space which is largely unrestricted - we don't actually want that - we want to know that our application is secured by SELinux and that even if an attacker compromised it, SELinux would contain the attack from further compromise of the system.

Now you might thing, "yes but we have the source code so we can fix it or make more secure". Remember that in many cases software vendors only provide pre-compiled binaries and as such you won't have this privilege. Hence we want to target testprog and confine it.

To do this we need to write a simple SELinux policy. I will leave reading the documentation about how this policy is created as an exercise for the reader (hint: https://selinuxproject.org/page/TypeRules) - however the files you need are provided in this lab subdirectory. To build and test the policy do the following:

```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install selinux-policy-devel
...
[james@selinux-dev2 selinux-testprog]$ cd lab4-targeting-testprog/
[james@selinux-dev2 lab4-targeting-testprog]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab4-targeting-testprog]$ sudo semodule -i testprog.pp
```

We can check that the policy module is now loaded:

```
[james@selinux-dev2 lab4-targeting-testprog]$ sudo semodule -l | grep testprog
testprog	0.1
```

Good - now what does the module do? Let's take a look at it:

```
[james@selinux-dev2 lab4-targeting-testprog]$ grep -v -e ^# testprog.te
policy_module(testprog, 0.1)

require {
	type unconfined_t;
	role unconfined_r;
	class file { ioctl getattr setattr create read write unlink open relabelto };
	class process transition;
}

type testprog_t;
domain_type(testprog_t);
type testprog_exec_t;
files_type(testprog_exec_t);

role unconfined_r types testprog_t;

allow testprog_t testprog_exec_t : file { ioctl read getattr lock execute execute_no_trans entrypoint open } ;
type_transition unconfined_t testprog_exec_t : process testprog_t;
allow unconfined_t testprog_t : process transition ;
```

First off we're giving our module a name and version number so that it is easily identified. Then we declare all the objects we require that are already found elsewhere in the existing policy. We then define two new types just for our application - **testprog_t** which is the domain under which the process will run, and **testprog_exec_t** which is the type that our binary file will possess. Finally we have 4 rules:

1. Allow the **unconfined_r** role access to the **testprog_t** domain (remember RBAC?)
2. Allow the testprog binary (identified by it's SELinux type of **testprog_exec_t**) to be an entrypoint (amongst other things) to the **testprog_t** domain we have defined for the process.
3. Define a type transition to tell SELinux to transition out process to the **testprog_t** domain whenever it is launched from the **unconfined_t** domain and the binary has the **testprog_exec_t** type.
4. Allow the process transition we specified previously to actually happen (this is not implicit - SELinux expects you to be very explicit in your policy).

Now our policy is loaded so let's see what happens when we run from the shell:

```
[james@selinux-dev2 lab4-targeting-testprog]$ sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid &
[1] 12090
Using configuration file: /etc/testprog.conf
Wrote PID to /var/run/testprog.pid
Writing output to: /var/testprog/testprg.txt
Iteration count: -1
[james@selinux-dev2 lab4-targeting-testprog]$ ps -efZ | grep $(cat /var/run/testprog.pid)
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 root 12094 12090  0 17:31 pts/0 00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
```

Something is wrong - it's still working in the unconfined space. Proceed to Lab 5 to find out why and what to do...


