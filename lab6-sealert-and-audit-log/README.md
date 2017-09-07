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

The process with PID 12442 tried to write to 
