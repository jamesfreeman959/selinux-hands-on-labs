# systemd

It's great that our application runs, but right now we are starting it using the shell. We know ultimately we want this to run through `systemd`, and we saw earlier that `systemd` processes run with a different context, as `systemd` itself runs in a separate context that it's children inherit unless a policy says otherwise.

From our policy that got us this far, we know that we defined:

```
type_transition unconfined_t testprog_exec_t : process testprog_t;
```

However that won't have any effect if we start the process with `systemd`. Let's take a look:

```
[james@selinux-dev lab10-systemd]$ sudo kill $(cat /var/run/testprog.pid)

[james@selinux-dev lab10-systemd]$ sudo systemctl start testprog
[james@selinux-dev lab10-systemd]$ ps -efZ | grep testprog
system_u:system_r:init_t:s0     root      26390      1  0 15:20 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

[james@selinux-dev lab10-systemd]$ sudo systemctl status testprog
× testprog.service - SELinux Test Program
     Loaded: loaded (/etc/systemd/system/testprog.service; enabled; preset: disabled)
     Active: failed (Result: exit-code) since Sun 2025-01-05 22:12:14 UTC; 14s ago
   Duration: 4ms
    Process: 13138 ExecStart=/usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid (code=exited, status=203/EXEC)
   Main PID: 13138 (code=exited, status=203/EXEC)
        CPU: 2ms

Jan 05 22:12:14 selinux-rocky9 systemd[13138]: testprog.service: Failed to locate executable /usr/bin/testprog: Permission denied
Jan 05 22:12:14 selinux-rocky9 systemd[13138]: testprog.service: Failed at step EXEC spawning /usr/bin/testprog: Permission denied
Jan 05 22:12:14 selinux-rocky9 systemd[1]: Started SELinux Test Program.
Jan 05 22:12:14 selinux-rocky9 systemd[1]: testprog.service: Main process exited, code=exited, status=203/EXEC
Jan 05 22:12:14 selinux-rocky9 systemd[1]: testprog.service: Failed with result 'exit-code'.
```

We can see that `testprog` is not running - interestingly if you had attempted the previous version of this lab which was written for and tested on EL7, you would have seen `testprog` running successfully, albeit in the wrong context! However times change policies get more secure, and so it's clear we'll need to fix our policy to enable `testprog` to be run from `systemd`.

We already know from earlier labs that `systemd` launches processes in a different context, so clearly what we need to do now is to create a type transition that will make `testprog` switch into the `testprog_t` domain when it is launched from `systemd`.

Now we have already seen from our earlier lab that we can create a manual transition from one domain to another. We have also seen, however, that most common SELinux permissions have already been defined for us in the reference policy. Rather than adding multiple lines of code to create a second type transition, it seems fair to surmise that someone has already done this for us. As before, let's pick on an example SELinux policy that we know does what we want already (again I will use `ntp` here). If we examine the policy definition we can see:

```
[james@selinux-dev lab10-systemd]$ cat ~/refpolicy/selinux-policy-0113b35519369e628e7fcd87af000cfcd4b1fa6c/policy/modules/contrib/ntp.te
...
type ntpd_t;
type ntpd_exec_t;
init_daemon_domain(ntpd_t, ntpd_exec_t)
...
```

Just from the name, that piece of code looks useful, so let's use our SELinux functions we downloaded previously to examine it:

```
[james@selinux-dev lab10-systemd]$ seshowif init_daemon_domain
interface(`init_daemon_domain',`
        gen_require(`
                attribute direct_run_init, direct_init, direct_init_entry;
                type init_t;
                role system_r;
                attribute daemon;
                attribute initrc_transition_domain;
                attribute initrc_domain;
        ')

        typeattribute $1 daemon;
        typeattribute $2 direct_init_entry;

        domain_type($1)
        domain_entry_file($1, $2)

        type_transition initrc_domain $2:process $1;

        ifdef(`direct_sysadm_daemon',`
                type_transition direct_run_init $2:process $1;
                typeattribute $1 direct_init;
        ')

        optional_policy(`
                systemd_connectto_socket_proxyd_unix_sockets($1)
        ')
')

```

I will leave full expansion of this to the reader, but suffice to say this does everything we need to do, and lots more besides. In short it's perfect, so we just need to add one single line to our testprog policy:

```
[james@selinux-dev lab10-systemd]$ diff testprog.te ../lab9-putting-it-all-together/testprog.te
53,55d52
< init_daemon_domain(testprog_t, testprog_exec_t)
<
<
```

It should be that simple! Let's build and install the policy, then we can test:

```
[james@selinux-dev lab10-systemd]$ sudo systemctl stop testprog
[james@selinux-dev lab10-systemd]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev lab10-systemd]$ sudo semodule -r testprog
libsemanage.semanage_direct_remove_key: Removing last testprog module (no other testprog module exists at another priority).
[james@selinux-dev lab10-systemd]$ sudo semodule -i testprog.pp

[james@selinux-dev lab10-systemd]$ sudo systemctl start testprog
[james@selinux-dev lab10-systemd]$ systemctl status testprog
● testprog.service - SELinux Test Program
     Loaded: loaded (/etc/systemd/system/testprog.service; enabled; preset: disabled)
     Active: active (running) since Sun 2025-01-05 22:21:20 UTC; 1s ago
   Main PID: 15323 (testprog)
      Tasks: 1 (limit: 22920)
     Memory: 200.0K
        CPU: 3ms
     CGroup: /system.slice/testprog.service
             └─15323 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

Jan 05 22:21:20 selinux-rocky9 systemd[1]: Started SELinux Test Program.
Jan 05 22:21:20 selinux-rocky9 testprog[15323]: Using configuration file: /etc/testprog.conf
Jan 05 22:21:20 selinux-rocky9 testprog[15323]: Wrote PID to /var/run/testprog.pid
Jan 05 22:21:20 selinux-rocky9 testprog[15323]: Writing output to: /var/testprog/testprg.txt
Jan 05 22:21:20 selinux-rocky9 testprog[15323]: Iteration count: -1

[james@selinux-dev lab10-systemd]$ ps -efZ | grep testprog
system_u:system_r:testprog_t:s0 root      26513      1  0 15:34 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

```

Excellent result! Our policy is more or less complete... for now.
