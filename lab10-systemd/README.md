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

Now we have already seen from our earlier lab that we can create a manual transition from one domain to another. We have also seen, however, that most common SELinux permissions have already been defined for us in the reference policy. Rather than adding multiple lines of code to create a second type transition, it seems fair to surmise that someone has already done this for us. As before, let's pick on an example SELinux policy that we know does what we want already (again I will use ntp here). If we examine the policy definition we can see:

```
[james@selinux-dev2 lab10-systemd]$ cat /root/selinux-refpolicy/serefpolicy-contrib-3.13.1/ntp.te
...
type ntpd_t;
type ntpd_exec_t;
init_daemon_domain(ntpd_t, ntpd_exec_t)
...
```

Just from the name, that piece of code looks useful, so let's use our SELinux functions we downloaded previously to examine it:

```
[james@selinux-dev2 lab10-systemd]$ seshowif init_daemon_domain
interface(`init_daemon_domain',`
	gen_require(`
		attribute direct_run_init, direct_init, direct_init_entry;
		type initrc_t;
		role system_r;
		attribute daemon;
	')

	typeattribute $1 daemon;

	domain_type($1)
	domain_entry_file($1, $2)

	role system_r types $1;

	domtrans_pattern(initrc_t, $2, $1)

	# daemons started from init will
	# inherit fds from init for the console
	init_dontaudit_use_fds($1)
	term_dontaudit_use_console($1)

	# init script ptys are the stdin/out/err
	# when using run_init
	init_use_script_ptys($1)

	ifdef(`direct_sysadm_daemon',`
		domtrans_pattern(direct_run_init, $2, $1)
		allow direct_run_init $1:process { noatsecure siginh rlimitinh };

		typeattribute $1 direct_init;
		typeattribute $2 direct_init_entry;

		userdom_dontaudit_use_user_terminals($1)
	')

	ifdef(`hide_broken_symptoms',`
		# RHEL4 systems seem to have a stray
		# fds open from the initrd
		ifdef(`distro_rhel4',`
			kernel_dontaudit_use_fds($1)
		')
	')

	optional_policy(`
		nscd_use($1)
	')
')

```

I will leave full expansion of this to the reader, but suffice to say this does everything we need to do, and lots more besides. In short it's perfect, so we just need to add one single line to our testprog policy:

```
[james@selinux-dev2 lab10-systemd]$ diff testprog.te ../lab9-putting-it-all-together/testprog.te
53,55d52
< init_daemon_domain(testprog_t, testprog_exec_t)
<
<
```

It should be that simple! Let's build and install the policy, then we can test:

```
[james@selinux-dev2 lab10-systemd]$ sudo systemctl stop testprog
[james@selinux-dev2 lab10-systemd]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab10-systemd]$ sudo semodule -r testprog
libsemanage.semanage_direct_remove_key: Removing last testprog module (no other testprog module exists at another priority).
[james@selinux-dev2 lab10-systemd]$ sudo semodule -i testprog.pp

[james@selinux-dev2 lab10-systemd]$ sudo systemctl start testprog
[james@selinux-dev2 lab10-systemd]$ systemctl status testprog
● testprog.service - SELinux Test Program
   Loaded: loaded (/etc/systemd/system/testprog.service; enabled; vendor preset: disabled)
   Active: active (running) since Mon 2017-09-18 15:34:41 BST; 3s ago
 Main PID: 26513 (testprog)
   CGroup: /system.slice/testprog.service
           └─26513 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

Sep 18 15:34:41 selinux-dev2.lab.quru.com systemd[1]: Started SELinux Test Program.
Sep 18 15:34:41 selinux-dev2.lab.quru.com systemd[1]: Starting SELinux Test Program...
Sep 18 15:34:41 selinux-dev2.lab.quru.com testprog[26513]: Using configuration file: /etc/testprog.conf
Sep 18 15:34:41 selinux-dev2.lab.quru.com testprog[26513]: Wrote PID to /var/run/testprog.pid
Sep 18 15:34:41 selinux-dev2.lab.quru.com testprog[26513]: Writing output to: /var/testprog/testprg.txt
Sep 18 15:34:41 selinux-dev2.lab.quru.com testprog[26513]: Iteration count: -1

[james@selinux-dev2 lab10-systemd]$ ps -efZ | grep testprog
system_u:system_r:testprog_t:s0 root      26513      1  0 15:34 ?        00:00:00 /usr/bin/testprog /etc/testprog.conf /var/run/testprog.pid

```

Excellent result! Our policy is more or less complete... for now.
