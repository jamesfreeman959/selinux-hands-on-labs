# Running testprog for the first time

Let's get stuck in and try running testprog for the first time. First of all let's check that we're in SELinux Enforcing mode:

```
[james@selinux-dev2 selinux-testprog]$ sestatus
SELinux status:                 enabled
SELinuxfs mount:                /sys/fs/selinux
SELinux root directory:         /etc/selinux
Loaded policy name:             targeted
Current mode:                   enforcing
Mode from config file:          enforcing
Policy MLS status:              enabled
Policy deny_unknown status:     allowed
Max kernel policy version:      28
```

Ok good. Now what happens first of all if we run the binary from the shell? We're in **Enforcing** mode so we would expect the application to fail....

```
[james@selinux-dev2 selinux-testprog]$ sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid &
[1] 11665
Using configuration file: /etc/testprog.conf
Wrote PID to /var/run/testprog.pid
Writing output to: /var/testprog/testprg.txt
Iteration count: -1
[james@selinux-dev2 selinux-testprog]$
```

It's working, and if you want to double check:

```
[james@selinux-dev2 selinux-testprog]$ tail -f /var/testprog/testprg.txt
Hello World
abcdefghij
Hello World
abcdefghij^C
[james@selinux-dev2 selinux-testprog]$ cat /var/run/testprog.pid
11666
```

Normally SELinux Enforcing mode strikes fear into people - otherwise they wouldn't turn it off when it comes to running bespoke applications. Why did this work then? Let's look at the process table:

```
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep $(cat /var/run/testprog.pid)
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 root 11666 11665  0 16:04 pts/0 00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
```

Note the output in the first column - this is the SELinux context that the process is running in. This gives us our clue as to why the application is running perfectly, and the clue is in the word **unconfined**.

# Targeted policies

Targeted mode on SELinux does more or less what it says on the tin - it **targets** specific applications on the system and applies policy to them. Everything else that is unknown to it has to come under a blanked policy, rather like a firewall. Typically a firewall denies all incoming connections and allows known ones through (i.e. targeted ones in SELinux parlance). SELinux takes the opposite approach - denying all applications from running if there is no known SELinux policy for them would make the system incredibly secure, but also painful to work with. As a result the default targetted policy is to run unknown applications in an unconfined context - that is to say that they run as if SELinux was **disabled**!

This obviously makes it very easy to get new applications running, but it's not very secure - our testprog application has access to the entire system, as root (we ran it with sudo) without restriction. Kind of negates the idea of running SELinux, especially if this was an Internet facing service accepting connection.

# What about systemd?

Now you may ask, is it more secure if you run it as a service? Let's try (don't forget to kill the process we backgrounded earlier first)....

```
[james@selinux-dev2 selinux-testprog]$ fg
sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
^C 
[james@selinux-dev2 selinux-testprog]$ sudo systemctl start testprog
[james@selinux-dev2 selinux-testprog]$ sudo systemctl status testprog
● testprog.service - SELinux Test Program
   Loaded: loaded (/etc/systemd/system/testprog.service; enabled; vendor preset: disabled)
   Active: active (running) since Thu 2017-09-07 16:17:04 BST; 15s ago
 Main PID: 11695 (testprog)
   CGroup: /system.slice/testprog.service
           └─11695 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

Sep 07 16:17:04 selinux-dev2.lab.quru.com systemd[1]: Started SELinux Test Program.
Sep 07 16:17:04 selinux-dev2.lab.quru.com systemd[1]: Starting SELinux Test Program...
Sep 07 16:17:04 selinux-dev2.lab.quru.com testprog[11695]: Using configuration file: /etc/testprog.conf
Sep 07 16:17:04 selinux-dev2.lab.quru.com testprog[11695]: Wrote PID to /var/run/testprog.pid
Sep 07 16:17:04 selinux-dev2.lab.quru.com testprog[11695]: Writing output to: /var/testprog/testprg.txt
Sep 07 16:17:04 selinux-dev2.lab.quru.com testprog[11695]: Iteration count: -1
```

Once again it's working! However note this time:

```
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep $(cat /var/run/testprog.pid)
system_u:system_r:unconfined_service_t:s0 root 11695 1  0 16:17 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
```

Notice the different context this time? We can still see that keyword in the type field of the context (more on that in a minute), **unconfined**, but other facets have changed. At this stage we haven't defined any SELinux policy for this application, so why is it behaving differently?

# A word on contexts

Red Hat's own documentation says: SELinux contexts follow the SELinux **user:role:type:level** syntax.

There is extensive reading available out there on this topic but in brief:

## user

This is an identity known to the SELinux policy that is mapped to users on the system, and is authorised through policy for a specific set of roles.

You can see the user mappings by installing and running the **semanage** tool:
```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install policycoreutils-python
...
<output truncated>
...
[james@selinux-dev2 selinux-testprog]$ sudo semanage login -l

Login Name           SELinux User         MLS/MCS Range        Service

__default__          unconfined_u         s0-s0:c0.c1023       *
root                 unconfined_u         s0-s0:c0.c1023       *
system_u             system_u             s0-s0:c0.c1023       *
```

Here we can see that the **root** user is mapped to the **unconfined_u** SELinux user, as is the **default** for all other users (so my **james** account will fall under this). A special user exists for system processes called **system_u** - we saw this when we ran testprog under systemd.

## role

SELinux users are authorized for roles, and roles are authorized for domains (more on them later). As a result this gives SELinux Role-based Access Control (RBAC) and helps reduce the risk of privilege escalation attacks. For example normally getting to root on a compromised system is the Holy Grail - however you can run a process under a role that even **unconfined_r** doesn't have access to, and as a result SELinux will block access to the domains that role is authorized for. This is an unlikely example as the whole nature of the targeted policy is to give access to everything from the unconfined role, but it is technically possible.

## type

Red Hat's own documentation says it best: "The type is an attribute of Type Enforcement. The type defines a domain for processes, and a type for files. SELinux policy rules define how types can access each other, whether it be a domain accessing a type, or a domain accessing another domain. Access is only allowed if a specific SELinux policy rule exists that allows it. "

## level

This is beyond the scope of this project at this stage, though it may be added if there is demand for it. This field is only of value if the MLS policy is loaded and in operation - we do not need to worry about it under the targeted policy. MLS stands for Multi-Level Security and provides an additional fine-grained layer of system security to meet even higher security requirements than the targeted policy, such as compliance with EAL4+.

# Inheritance

Ok so given all this, how did we end up running testprog in the contexts we saw earlier? SELinux behaviour may be summed up simply here as:

The type context of a process or file will always inherit the context of it's parent unless there is a policy to tell it otherwise.

Let's take for example our testprog binary. We just copied it into place - no SELinux configuration was done. Yet it will have a context:

```
[james@selinux-dev2 selinux-testprog]$ ls -lZ /usr/bin/testprog
-rwxr-xr-x. root root unconfined_u:object_r:bin_t:s0   /usr/bin/testprog
```

To see how it got that, let's have a look at it's parent item, in this case the **/usr/bin/** directory:

```
[james@selinux-dev2 selinux-testprog]$ ls -ldZ /usr/bin
dr-xr-xr-x. root root system_u:object_r:bin_t:s0       /usr/bin
```

Notice that our binary inherited the **bin_t** type from it's parent directory. However the **user** and **role** fields have stayed the same as the original file.

Similar can be observed of our configuration file:

```
[james@selinux-dev2 selinux-testprog]$ ls -lZ /etc/testprog.conf
-rw-r--r--. root root unconfined_u:object_r:etc_t:s0   /etc/testprog.conf
[james@selinux-dev2 selinux-testprog]$ ls -ldZ /etc
drwxr-xr-x. root root system_u:object_r:etc_t:s0       /etc
```

And even our data directory - note the inheritance across both the directory our **make install** command created, and also the data file created at runtime:

```
[james@selinux-dev2 selinux-testprog]$ ls -lZ /var/testprog/
-rw-r--r--. root root unconfined_u:object_r:var_t:s0   testprg.txt
[james@selinux-dev2 selinux-testprog]$ ls -ldZ /var/testprog /var
drwxr-xr-x. root root system_u:object_r:var_t:s0       /var
drwxr-xr-x. root root unconfined_u:object_r:var_t:s0   /var/testprog
```

Processes operate in a similar way. When we ran testprog from the shell, we were actually running it from bash via sudo:

```
[james@selinux-dev2 selinux-testprog]$ sudo systemctl stop testprog
[james@selinux-dev2 selinux-testprog]$ sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid &
[1] 11818
Using configuration file: /etc/testprog.conf
Wrote PID to /var/run/testprog.pid
Writing output to: /var/testprog/testprg.txt
Iteration count: -1
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep $(cat /var/run/testprog.pid)
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 root 11819 11818  0 16:49 pts/0 00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
...
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep 11818
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 root 11818 1221  0 16:49 pts/0 00:00:00 sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
...
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep 1221
unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 james 1221 1220  0 14:59 pts/0 00:00:00 -bash
...
```

And through systemd:

```
[james@selinux-dev2 selinux-testprog]$ fg
sudo /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
^C
[james@selinux-dev2 selinux-testprog]$ sudo systemctl start testprog
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep $(cat /var/run/testprog.pid)
system_u:system_r:unconfined_service_t:s0 root 11836 1  0 16:53 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid
...
[james@selinux-dev2 selinux-testprog]$ ps -efZ | grep systemd
system_u:system_r:init_t:s0     root          1      0  0 14:45 ?        00:00:01 /usr/lib/systemd/systemd --switched-root --system --deserialize 21
...
```

Wait! We didn't see the **type** get inherited here. If you ever see that happen, suspect that an SELinux policy rule has made this happen. If you believe this is happening, the **sesearch** tool is your friend:

```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install setools-consle
...
[james@selinux-dev2 selinux-testprog]$ sesearch -s init_t -t unconfined_service_t -Ad
Found 2 semantic av rules:
   allow init_t unconfined_service_t : process transition ;
   allow init_t unconfined_service_t : dbus send_msg ;

```

The latter command searches the currently loaded SELinux policy for any rule allowing the source type **init_t** and the target type **unconfined_service_t**. We can see there is a rule specifically allowing process transition, which confirms our original suspicion

# Recommended reading

https://wiki.gentoo.org/wiki/SELinux/Tutorials/How_does_a_process_get_into_a_certain_context

