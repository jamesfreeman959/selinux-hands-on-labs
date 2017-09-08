# Lab 6 - sealert and audit.log

We have successfully broken our application by making it switch to a confined context - we now need to understand why it is broken and how to fix it. This lab will focus on diagnosing the failures - something that will be useful as you start to debug SELinux issues and create your own policies.

When SELinux is enabled, it's actions are logged into **/var/log/audit/audit.log** - initially this file might be hard to deciper but with a little practise it becomes easier. There are also tools to help you. Let's take a little look at our failed run of testprog to start with - we can grep that as a keyword as we know with almost certainty that there won't be any other applications called testprog on our system:

```
[james@selinux-dev2 selinux-testprog]$ sudo grep testprog /var/log/audit/audit.log
type=USER_CMD msg=audit(1504795300.550:139): pid=11516 uid=1000 auid=0 ses=1 subj=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 msg='cwd="/home/james/selinux-testprog/testprog" cmd=79756D202D7920696E7374616C6C20676363206D616B65 terminal=pts/0 res=success'
type=USER_CMD msg=audit(1504795720.854:146): pid=11547 uid=1000 auid=0 ses=1 subj=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 msg='cwd="/home/james/selinux-testprog/testprog" cmd=6D616B6520696E7374616C6C terminal=pts/0 res=success'
type=USER_CMD msg=audit(1504795841.993:151): pid=11557 uid=1000 auid=0 ses=1 subj=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 msg='cwd="/home/james/selinux-testprog/testprog" cmd=73797374656D63746C2072656C6F6164202D2D616C6C terminal=pts/0 res=success'
...
<output truncated>
...
```

There's a huge amount of output including success messages so let's be a little more specific:

```
[james@selinux-dev2 selinux-testprog]$ sudo grep AVC /var/log/audit/audit.log | grep testprog
...
type=AVC msg=audit(1504818384.560:460): avc:  denied  { write } for  pid=12442 comm="testprog" name="testprog.pid" dev="tmpfs" ino=34664 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:var_run_t:s0 tclass=file
...
```

There are still lots of lines of output, which really show how many parts of the system even our simple little testprog tries to access. It also shows you how finely grained SELinux's access control is - it goes beyond filesystem restrictions to controlling socket creation, chr file access, console output and so much more. I have cherry picked a simple example from the output on my development system to share above - what it's saying is:

The process with PID 12442 (named testprog) tried to write to a file object which had context unconfined_u:object_r:var_run_t:s0 and was called testprog.pid, with inode number 34663 on the tmpfs device (/var/run is on tmpfs). Most importantly the source context of the process was unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023, which confirms our new policy did indeed switch successfully to the new **testprog_t** context we created.

Given this log we can also see how we could go about creating a full policy for our application so that it works like it used to, only confined to it's own context. We also see the power of SELinux - even though we ran testprog as root, it cannot write it's own PID file in /var/run, which is not what you would normally expect!

# sealert

Of course whilst this log file can be quite useful once you know how to interpret it, it's not always easy to decipher. The **sealert** tool can help you create new policies and fix issues like this, but it should be used with caution and awareness to create a truly secure application. 

```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install setroubleshoot-server
...
```

Having installed the package containing sealert, let's have a look at it's interpretation of the audit log:

```
[james@selinux-dev2 selinux-testprog]$ sudo sealert -a /var/log/audit/audit.log > sealert.txt
[james@selinux-dev2 selinux-testprog]$ cat sealert.txt
--------------------------------------------------------------------------------

SELinux is preventing /usr/bin/testprog from write access on the file testprog.pid.

*****  Plugin catchall (100. confidence) suggests   **************************

If you believe that testprog should be allowed write access on the testprog.pid file by default.
Then you should report this as a bug.
You can generate a local policy module to allow this access.
Do
allow this access for now by executing:
# ausearch -c 'testprog' --raw | audit2allow -M my-testprog
# semodule -i my-testprog.pp


Additional Information:
Source Context                unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c102
                              3
Target Context                unconfined_u:object_r:var_run_t:s0
Target Objects                testprog.pid [ file ]
Source                        testprog
Source Path                   /usr/bin/testprog
Port                          <Unknown>
Host                          <Unknown>
Source RPM Packages
Target RPM Packages
Policy RPM                    selinux-policy-3.13.1-166.el7_4.4.noarch
Selinux Enabled               True
Policy Type                   targeted
Enforcing Mode                Enforcing
Host Name                     selinux-dev2.lab.quru.com
Platform                      Linux selinux-dev2.lab.quru.com
                              3.10.0-693.2.1.el7.x86_64 #1 SMP Fri Aug 11
                              04:58:43 EDT 2017 x86_64 x86_64
Alert Count                   2
First Seen                    2017-09-07 22:06:11 BST
Last Seen                     2017-09-07 22:06:24 BST
Local ID                      05861116-def2-4893-a07c-03f096b185c7

Raw Audit Messages
type=AVC msg=audit(1504818384.560:460): avc:  denied  { write } for  pid=12442 comm="testprog" name="testprog.pid" dev="tmpfs" ino=34664 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:var_run_t:s0 tclass=file

type=SYSCALL msg=audit(1504818384.560:460): arch=x86_64 syscall=open success=no exit=EACCES a0=7fff3eca57d0 a1=241 a2=1b6 a3=24 items=0 ppid=12441 pid=12442 auid=0 uid=0 gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 tty=(none) ses=1 comm=testprog exe=/usr/bin/testprog subj=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 key=(null)

Hash: testprog,testprog_t,var_run_t,file,write
```

This again is just focussing on our PID file example for simplicity - the output from sealert is full of other alerts. However we can see that from the output above, sealert is 100% confident that it's found a fix for our inability to write our pid file and it even tells us how to automatically create a new policy to fix it whilst allowing testprog to run in it's confined context. How helpful! Let's run the first command it suggests and have a look at the results...

```
[james@selinux-dev2 selinux-testprog]$ sudo ausearch -c 'testprog' --raw | audit2allow -M my-testprog
******************** IMPORTANT ***********************
To make this policy package active, execute:

semodule -i my-testprog.pp

******************** IMPORTANT ***********************
To make this policy package active, execute:

semodule -i my-testprog.pp

[james@selinux-dev2 selinux-testprog]$ cat my-testprog.te

module my-testprog 1.0;

require {
	type var_run_t;
	type testprog_t;
	class file write;
}

#============= testprog_t ==============

#!!!! WARNING: 'var_run_t' is a base type.
allow testprog_t var_run_t:file write;

```

Now again I have sanitized the output so that it's only relevant to our PID file for simplicity. The output above is what sealert suggests we install as a new policy - but let's take a step back - what would be the consequences of doing that?

1. If you run this as is, you will end up with two policies governing testprog - one for the transition and one for the writing of the PID file. Given that we know that we have more to fix to make the application work, we could end up with a whole litter of policies just for one application which is an administrative headache. We could of course copy the output above and merge it with the policy we created in the previous lab - that would be better, but....

2. What the above policy is saying is **allow write access to files in the var_run_t type from the testprog_t domain**. We know (and can easily verify) that the directory **/var/run** itself has this type, and as a result we are giving testprog write access to anything in this directory. This may not sound terrible in the case of a PID file residing on tmpfs, but remember we also have a config file to deal with that is in **/etc**. Would we really want our application to have read access (as will be required) to the entire /etc directory? Including our shadow password file, sudoers, and such (which as the application is running at root it will be able to do). The whole idea is to block this kind of access to that in the event of an unexpected compromise, an attacker is contained and can read the testprog configuration file, and write the testprog PID file, but nothing more.

Make no mistake, using the above policy is better than just turning off SELinux to make the application work. It will be more contained if we do this than it would in the unconfined domain, or without SELinux at all. Simply we can do better. So let's discard this suggestion and write our own policy.

# Recommended reading

https://wiki.gentoo.org/wiki/SELinux/Tutorials/Where_to_find_SELinux_permission_denial_details

