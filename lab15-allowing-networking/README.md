# Allowing networking

We know from before that looking at the policy for the NTP daemon helped us immensely in moving forwards with our policy for `testprog`. However we also know that `ntpd` relies primarily on UDP for its communications, and our appliation is TCP based. As a result we'll focus on another example - here I have picked the ssh daemon policy module (aptly named `ssh.te`). If you read through the policy file, you'll come across this section somewhere near the middle of the file:

```
[james@selinux-dev selinux-hands-on-labs]$ find ~ -name ssh.te
/home/jamesf_local/refpolicy/selinux-policy-0d528aef37153a0bcf9cdd987a32b1ccf0da7068/policy/modules/services/ssh.te
[james@selinux-dev selinux-hands-on-labs]$ less /home/jamesf_local/refpolicy/selinux-policy-0d528aef37153a0bcf9cdd987a32b1ccf0da7068/policy/modules/services/ssh.te
...
allow ssh_t self:tcp_socket create_stream_socket_perms;
...
corenet_all_recvfrom_netlabel(ssh_t)
corenet_tcp_sendrecv_generic_if(ssh_t)
corenet_tcp_sendrecv_generic_node(ssh_t)
corenet_tcp_sendrecv_all_ports(ssh_t)
corenet_tcp_connect_ssh_port(ssh_t)
corenet_tcp_connect_all_unreserved_ports(ssh_t)
corenet_sendrecv_ssh_client_packets(ssh_t)
corenet_tcp_bind_generic_node(ssh_t)
#corenet_tcp_bind_all_unreserved_ports(ssh_t)
corenet_rw_tun_tap_dev(ssh_t)
...
        corenet_tcp_bind_ssh_port(ssh_t)
        corenet_tcp_bind_generic_node(ssh_t)
        corenet_tcp_bind_all_unreserved_ports(ssh_t)
...
corenet_tcp_bind_xserver_port(sshd_t)
corenet_tcp_bind_vnc_port(sshd_t)
corenet_sendrecv_xserver_server_packets(sshd_t)
...
```

There's a pretty massive hint there! Much of the networking policy rules is performed using a set of macros that all begin with `corenet`. The exception is the `tcp_socket` line above them all - however this sets up the most basic level of permissions to allow the program to set up a TCP socket before any other work relating to connections or traffic is allowed. SELinux with networking is very powerful, and there are ways to restrict communications by address and many other things. However, the most common task you will face in an EL9/Fedora 41 environment is to define a policy to allow access to a given port, and possibly customize that port. 

As previously, it would help us to expand the above macros and see what they do so that we can pick the appropriate one. In an earlier lab we installed the `selinux-policy-devel` package which gave us our Makefile amongst other things. If we look into the files that this package installed, we can see:

```
[james@selinux-dev selinux-hands-on-labs]$ rpm -ql selinux-policy-devel | grep core
/usr/share/selinux/devel/html/abrt_retrace_coredump.html
/usr/share/selinux/devel/include/kernel/corecommands.if
/usr/share/selinux/devel/include/kernel/corenetwork.if
```

It turns out that some of the interfaces for the policy are built as part of the actual build process of the source RPM and as a result we can't just go into the source tree of the `refpolicy` directory we created earlier and find them - we have to actually look on the host itself. Thankfully the `selinux-funcs-el9.txt` has been modified from the author's original version to take account of this and search these additional directories.

Now, let's say that we have decided on a port of `8123/tcp` for `testprog-net`. How do we make this work? From the previous lab we saw that the app was denied the chance to create a `tcp_socket`  and the policy for sshd creates this manually, so let's add the following line to our policy (along with the appropriate require declarations at the top):

```
allow testprog_t self:tcp_socket create_stream_socket_perms;
```

If you add the above and try running `sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &` again, you'll see it still fails to run, only now with a different error in the audit logs:

```
type=AVC msg=audit(1736178175.452:1322): avc:  denied  { name_bind } for  pid=16747 comm="testprog-net" src=8123 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=system_u:object_r:http_cache_port_t:s0 tclass=tcp_socket permissive=0
```

Whilst we could just allow a straightforward bind to our socket, we actually want to restrict it to a specific port as we know the one that "belongs" to our application. This is where it gets interesting. We have already seen that we can generate new file contexts, load them through a policy module, and then apply or restore them. However the same is not true of ports - these must (at the time of writing) be labelled as part of the base policy, and updating this is a job for the distribution maintainer. You could rebuild it, but on EL9/Fedora 41 a simple rpm update would remove your custom changes so this is not recommended. Instead we need to create a type for our port, then add a label for it on the command line. First off add the following to the testprog.te policy:

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

# Allow our new network capabilities
allow testprog_t self:tcp_socket create_stream_socket_perms;
corenet_tcp_sendrecv_generic_node(testprog_t)
corenet_tcp_bind_generic_node(testprog_t)
allow testprog_t testprog_port_t:tcp_socket { name_bind };
```

Now we have an SELinux port type defined just for `testprog`. Build and install the policy module in the usual way, then let's create a port type definition for `testprog_t`...

```
[james@selinux-dev lab15-allowing-networking]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev lab15-allowing-networking]$ sudo semodule -i testprog.pp
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8123
Port tcp/8123 already defined, modifying instead
```

Oops! Now we see why port definitions can only be managed at the base policy level - it would be too easy to overlap ports or re-assign something. Let's instead find a free port and reconfigure `testprog` to use it - we'll also clean up that duplicate mapping we've created...

```
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -l | grep 8123
http_cache_port_t              tcp      8080, 8118, 8123, 10001-10010
testprog_port_t                tcp      8123
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -d -t testprog_port_t -p tcp 8123
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -l | grep 8234
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8234
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -l | grep test
testprog_port_t                tcp      8234
[james@selinux-dev lab15-allowing-networking]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=8234/g' /etc/testprog-net.conf
```

That literally is all that is required to get our network enabled version of `testprog` working - much simpler than SSH, but then the SSH daemon is a lot more feature rich that our little test program. Build and install the policy if you haven't already, and try running `testprog` one more time...

```
[james@selinux-dev lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60296
[james@selinux-dev lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8234
Socket created
bind done
Waiting for incoming connections...
```

Success! Try out the program and make sure it works - you can use `telnet` or `nc` to connect to it on port 8234, and anything you type in should get echoed back to you, and also written to the data file.

Now one final question remains. We allocated and labelled a specific port for `testprog-net`. But what if we need to change it? Try the following:

```
[james@selinux-dev lab15-allowing-networking]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=8235/g' /etc/testprog-net.conf
[james@selinux-dev lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60301
[james@selinux-dev lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
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
[james@selinux-dev lab15-allowing-networking]$ sudo grep AVC /var/log/audit/audit.log | grep testprog
type=AVC msg=audit(1736178782.210:1394): avc:  denied  { name_bind } for  pid=16914 comm="testprog-net" src=8235 scontext=unconfined_u:unconfined_r:testprog_t:s0-s0:c0.c1023 tcontext=system_u:object_r:unreserved_port_t:s0 tclass=tcp_socket permissive=0
```

As you can see, we've falled into a different type because port 8235 is labelled at `unreserved_port_t`. We can fix this quite simply by relabelling this port:

```
[james@selinux-dev lab15-allowing-networking]$ sudo semanage port -a -t testprog_port_t -p tcp 8235
[james@selinux-dev lab15-allowing-networking]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60312
[james@selinux-dev lab15-allowing-networking]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8235
Socket created
bind done
Waiting for incoming connections...
```

Job done! One thing remains though - our network functionality is optional, and can be turned off by setting the `NETWORKPORT` paramter to `0`. If we are running with the networking off, surely we want to disable this SELinux access to ensure everything is locked down as tight as it can be! To do that we could create a separate policy and load that (after first removing the original policy and all it's dependencies such as `testprog_port_t` and `testcat`), but that is a lot of work for a simple parameter change. There must be a better way...

