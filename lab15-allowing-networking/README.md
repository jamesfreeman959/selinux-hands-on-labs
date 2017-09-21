# Allowing networking

We know from before that looking at the policy for the NTP daemon helped us immensely in moving forwards with our policy for testprog. However we also know that ntpd relies primarily on UDP for its communications, and our appliation is TCP based. As a result we'll focus on another example - here I have picked the ssh daemon policy module (aptly named `ssh.te`). If you read through the policy file, you'll come across this section somewhere near the middle of the file:

```
[james@selinux-dev2 selinux-testprog]$ less $HOME/selinux-testprog/refpolicy/serefpolicy-3.13.1/policy/modules/services/ssh.te
...
allow ssh_t self:tcp_socket create_stream_socket_perms;
...
corenet_all_recvfrom_unlabeled(ssh_t)
corenet_all_recvfrom_netlabel(ssh_t)
corenet_tcp_sendrecv_generic_if(ssh_t)
corenet_tcp_sendrecv_generic_node(ssh_t)
corenet_tcp_sendrecv_all_ports(ssh_t)
corenet_tcp_connect_ssh_port(ssh_t)
corenet_sendrecv_ssh_client_packets(ssh_t)
...
       corenet_tcp_bind_ssh_port(ssh_t)
       corenet_tcp_bind_generic_node(ssh_t)
...
corenet_tcp_bind_xserver_port(sshd_t)
corenet_sendrecv_xserver_server_packets(sshd_t)
...
```

There's a pretty massive hint there! Much of the networking policy rules is performed using a set of macros that all being **corenet**. The exception is the **tcp_socket** line above them all - however this sets up the most basic level of allowing the program to set up a TCP socket before any other work relating to connections or traffic is allowed. SELinux with networking is very powerful, and there are ways to restrict communications by address and many other things. This is beyond the scope of this lab for now - the most common task you will face in an EL7 environment is to define a policy to allow access to a given port, and possibly customize that port. 

As previously, it would help us to expand the above macros and see what they do so that we can pick the appropriate one. However if you run `selist` from the `selinux-funcs.txt` file downloaded earlier, you'll not see any of these macros. Yet they must exist otherwise the ssh policy would be invalid. Let's fix this so we can move forwards...

In an earlier lab we installed the `selinux-policy-devel` package which gave us our Makefile amongst other things. If we look into the files that this package installed, we can see:

```
[james@selinux-dev2 selinux-testprog]$ rpm -ql selinux-policy-devel | grep core
/usr/share/selinux/devel/html/abrt_retrace_coredump.html
/usr/share/selinux/devel/include/kernel/corecommands.if
/usr/share/selinux/devel/include/kernel/corenetwork.if
```

It turns out that some of the interfaces for the policy are built as part of the actual build process of the source RPM and as a result we can't just go into the source tree as we have done so far and find the interfaces we need. The `selinux-funcs.txt` functions have been an amazing help to us, but they have hard coded paths in them which don't work with the tree installed by `selinux-policy-devel`. We need to modify these functions and then we can use this development package.

We can patch the original file to use the actual location the development RPM uses without the need to set the `POLICY_LOCATION` environment variable. Given that this RPM will always install to the same location (unless we force otherwise) this actually simplified the operation of these functions, though it does limit their use to EL7 and any other distros that use the same paths. You can patch the original file as follows:

```
[james@selinux-dev2 selinux-testprog]$ cp selinux-funcs.txt selinux-funcs-el7.txt
[james@selinux-dev2 selinux-testprog]$ sed -i 's/\${POLICY_LOCATION}\/policy\/modules/\/usr\/share\/selinux\/devel\/include/g' selinux-funcs-el7.txt
[james@selinux-dev2 selinux-testprog]$ sed -i 's/\${POLICY_LOCATION}\/policy\/support/\/usr\/share\/selinux\/devel\/include\/support/g' selinux-funcs-el7.txt
[james@selinux-dev2 selinux-testprog]$ source selinux-funcs-el7.txt
```

A modified copy of the EL7 specific script is available for download here:

https://gist.githubusercontent.com/jamesfreeman959/40d41810beccc4ded23dc049d6ed570d/raw/42acb82b86a8b6893e8e43be7fe602fdea223a94/selinux-funcs-el7.txt

With the modified functions in place, try expanding the macros and definitions from those lines I have cherry picked from the `ssh.te` file and see what they do. 

Now, let's say that we have decided on a port of 8123/tcp for testprog-net. How do we make this work? From the previous lab we saw that the app was denied the chance to create a **tcp_socket** and the policy for sshd creates this manually, so let's add the following line to our policy (along with the appropriate require declarations at the top:

```
allow testprog_t self:tcp_socket create_stream_socket_perms;
```

If you add the above and try running `sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &` again, you'll see it still fails to run, only now with a different error in the audit logs:

```
type=AVC msg=audit(1505914319.690:3959): avc:  denied  { bind } for  pid=42656 comm="testprog-net" scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tclass=tcp_socket
```

Whilst we could just allow a straightforward bind to our socket, we actually want to restrict it to a specific port as we know the one that "belongs" to our application. This is where it gets interesting. We have already seen that we can generate new file contexts, load them through a policy module, and then apply or restore them. However the same is not true of ports - these must (at the time of writing) be labelled as part of the base policy, and updating this is a job for the distribution maintainer. You could rebuild it, but on EL7 a simple rpm update would remove your custom changes so this is not recommended. Instead we need to create a type for our port, then add a label for it on the command line. First off add the following to the testprog.te policy:

```
...
require
{
...
	attribute port_type;
}
...
type testprog_port_t;
typeattribute testprog_port_t port_type;
```

Now we have an SELinux port type defined just for testprog. Build and install the policy module in the usual way, then let's create a port type definition for testprog_t...

```
[james@selinux-dev2 lab15-allowing-networking]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab15-allowing-networking]$ sudo semodule -i testprog.pp
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8123
ValueError: Port tcp/8123 already defined
```

Oops! Now we see why port definitions can only be managed at the base policy level - it would be too easy to overlap ports or re-assign something. Let's instead find a free port and reconfigure testprog to use it...

```
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -l | grep 8123
http_cache_port_t              tcp      8080, 8118, 8123, 10001-10010
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -l | grep 8234
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8234
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -l | grep test
testprog_port_t                tcp      8234
[james@selinux-dev2 lab15-allowing-networking]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=8234/g' /etc/testprog-net.conf
```

Excellent! Now what happens if we run our program:

```
mes@selinux-dev2 lab15-allowing-networking]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab15-allowing-networking]$ sudo semodule -i testprog.pp
[sudo] password for james:
[james@selinux-dev2 lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60255
[james@selinux-dev2 lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8234
Socket created
bind failed. Error

[1]+  Exit 1                  sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
[james@selinux-dev2 lab15-allowing-networking]$ sudo grep AVC /var/log/audit/audit.log | grep testprog
...
type=AVC msg=audit(1505991057.017:4446): avc:  denied  { name_bind } for  pid=60256 comm="testprog-net" src=8234 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=system_u:object_r:testprog_port_t:s0 tclass=tcp_socket
```

Now we're getting somewhere! You can see from above that the audit daemon has picked up that we tried to connect to our newly labelled testprog port, but were denied the correct permissions to do so. If you follow through the usual process of testing the application and inserting the missing rules, you should end up with something like this at the bottom of your policy module file:

```
# Allow our new network capabilities
allow testprog_t self:tcp_socket create_stream_socket_perms;
corenet_tcp_sendrecv_generic_node(testprog_t)
corenet_tcp_bind_generic_node(testprog_t)
allow testprog_t testprog_port_t:tcp_socket { name_bind };
```

That literally is all that is required - but then the SSH daemon is a lot more feature rich that our little test program. Build and install the policy and try running testprog one more time...

```
[james@selinux-dev2 lab15-allowing-networking]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab15-allowing-networking]$ sudo semodule -i testprog.pp
[james@selinux-dev2 lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60296
[james@selinux-dev2 lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8234
Socket created
bind done
Waiting for incoming connections...
```

Success! Try out the program and make sure it works - you can use telnet to connect to it on port 8234, and anything you type in should get echoed back to you, and also written to the data file.

Now one final question remains. We allocated and labelled a specific port for testprog-net. But what if we need to change it? Try the following:

```
[james@selinux-dev2 lab15-allowing-networking]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=8235/g' /etc/testprog-net.conf
[james@selinux-dev2 lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60301
[james@selinux-dev2 lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8235
Socket created
bind failed. Error

[1]+  Exit 1                  sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
```

This fails because our program tried to access a port that it didn't have permission to bind to. We can verify this from the audit logs:

```
[james@selinux-dev2 lab15-allowing-networking]$ sudo grep AVC /var/log/audit/audit.log | grep testprog
type=AVC msg=audit(1505991433.207:4475): avc:  denied  { name_bind } for  pid=60302 comm="testprog-net" src=8235 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=system_u:object_r:unreserved_port_t:s0 tclass=tcp_socket
```

As you can see, we've falled into a different type because port 8235 is labelled at `unreserved_port_t`. We can fix this quite simply by relabelling this port:

```
[james@selinux-dev2 lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8235
[james@selinux-dev2 lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60312
[james@selinux-dev2 lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8235
Socket created
bind done
Waiting for incoming connections...
```

Job done! One thing remains though - our network functionality is optional, and can be turned off by setting the `NETWORKPORT` paramter to `0`. If we are running with the networking off, surely we want to disable this SELinux access to ensure everything is locked down as tight as it can be! To do that we could create a separate policy and load that (after first removing the original policy and all it's dependencies such as `testprog_port_t` and `testcat`), but that is a lot of work for a simple parameter change. There must be a better way...

