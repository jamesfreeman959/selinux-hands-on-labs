# systemd

It's great that our application runs, but right now we are starting it using the shell. We know ultimately we want this to run through systemd, and we saw earlier that systemd processes run with a different context, as systemd itself runs in a separate context that it's children inherit unless a policy says otherwise.

From our policy that got us this far, we know that we defined:

```
type_transition unconfined_t testprog_exec_t : process testprog_t;
```

However that won't have any effect if we start the process with systemd. Let's take a look:

```
[james@selinux-dev2 lab10-systemd]$ sudo kill $(cat /var/run/testprog.pid)

[james@selinux-dev2 lab10-systemd]$ sudo systemctl start testprog
[james@selinux-dev2 lab10-systemd]$ ps -efZ | grep testprog
system_u:system_r:init_t:s0     root      26390      1  0 15:20 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

[james@selinux-dev2 lab10-systemd]$ sudo systemctl status testprog
● testprog.service - SELinux Test Program
   Loaded: loaded (/etc/systemd/system/testprog.service; enabled; vendor preset: disabled)
   Active: active (running) since Mon 2017-09-18 15:20:34 BST; 9s ago
 Main PID: 26390 (testprog)
   CGroup: /system.slice/testprog.service
           └─26390 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

Sep 18 15:20:34 selinux-dev2.lab.quru.com systemd[1]: Started SELinux Test Program.
Sep 18 15:20:34 selinux-dev2.lab.quru.com systemd[1]: Starting SELinux Test Program...
Sep 18 15:20:34 selinux-dev2.lab.quru.com testprog[26390]: Using configuration file: /etc/testprog.conf
Sep 18 15:20:34 selinux-dev2.lab.quru.com testprog[26390]: Wrote PID to /var/run/testprog.pid
Sep 18 15:20:34 selinux-dev2.lab.quru.com testprog[26390]: Writing output to: /var/testprog/testprg.txt
Sep 18 15:20:34 selinux-dev2.lab.quru.com testprog[26390]: Iteration count: -1
```

We can see that testprog in this case is running in the `init_t` context, and seems to be having no problems at all accessing its resources. If it did it would have exited straight away. We can validate this anyway by tailing its output file if you wish.

Our policy is working and we have already set up the file types, so clearly what we need to do now is to create a type transition that will make testprog switch into the `testprog_t` domain when it is launched from systemd.
