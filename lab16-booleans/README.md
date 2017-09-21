# Booleans

Following on from the previous lab, we have decided that we want the network functionality to be an option within the SELinux policy that we can turn on or off as required. How do we modify the policy to do that?

First of all, at the top of the policy, we need to declare our new boolean in the same way that we have declared all the other types before. The following line will go somewhere with the other type declarations just below the **require** section:

```
bool allow_testprog_use_network true;
```

This declares a new boolean, called `allow_testprog_use_network`, which is true by default. Then we simply find the block of code that we wish to be optional - in this case it will be the 4 lines at the end that we added previously, and ensure it is in a block marked as a tunable_policy:

```
# Allow our new network capabilities
tunable_policy(`allow_testprog_use_network',`
	allow testprog_t self:tcp_socket create_stream_socket_perms;
	corenet_tcp_sendrecv_generic_node(testprog_t)
	corenet_tcp_bind_generic_node(testprog_t)
	allow testprog_t testprog_port_t:tcp_socket { name_bind };
')
```

Note the 4 lines in the middle are exactly the same as before, we have just decided to allow them to be all on, or all off as required. Now we can test out our new functionality:

```
[james@selinux-dev2 lab16-booleans]$ make -f /usr/share/selinux/devel/Makefile testprog.pp
Compiling targeted testprog module
/usr/bin/checkmodule:  loading policy configuration from tmp/testprog.tmp
/usr/bin/checkmodule:  policy configuration loaded
/usr/bin/checkmodule:  writing binary representation (version 17) to tmp/testprog.mod
Creating targeted testprog.pp policy package
rm tmp/testprog.mod tmp/testprog.mod.fc
[james@selinux-dev2 lab16-booleans]$ sudo semodule -i testprog.pp
```

Booleans can be managed through `semanage`, or `setsebool` and `getsebool`. Note that if you are using `setsebool`, changes do not persist across reboots unless you specify the `-P` flag in the command. Change made is `semanage` persist by default.

```
[james@selinux-dev2 lab16-booleans]$ getsebool -a | grep test
allow_testprog_use_network --> on
[james@selinux-dev2 lab16-booleans]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=8234/g' /etc/testprog-net.conf
[james@selinux-dev2 lab16-booleans]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[1] 60733
[james@selinux-dev2 lab16-booleans]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8234
Socket created
bind done
Waiting for incoming connections...
```

Se we can see above that our new boolean was created, and it is set to **on** as per our default setting. Our network enabled version of testprog runs as normal. Now what happens if we turn off this boolean:

```
[james@selinux-dev2 lab16-booleans]$ fg
sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid
^C
[james@selinux-dev2 lab16-booleans]$ sudo setsebool allow_testprog_use_network=off
[james@selinux-dev2 lab16-booleans]$ getsebool -a | grep test
allow_testprog_use_network --> off
[james@selinux-dev2 lab16-booleans]$ sudo /usr/bin/testprog-net /etc/testprog-net.conf /var/run/testprog-net.pid &
[2] 60746
[james@selinux-dev2 lab16-booleans]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality enabled on port 8234
Could not create socket
Socket created
bind failed. Error
```

Notice how as soon as we disable the boolean we stop being able to run our program, as it now lacks the permissions to create sockets. You can verify this through the audit log if you wish. However if we disable the networking function:

```
[james@selinux-dev2 lab16-booleans]$ sudo sed -i 's/NETWORKPORT=.*/NETWORKPORT=0/g' /etc/testprog-net.conf
[james@selinux-dev2 lab16-booleans]$ getsebool -a | grep test
allow_testprog_use_network --> off
[james@selinux-dev2 lab16-booleans]$ Using configuration file: /etc/testprog-net.conf
Wrote PID to /var/run/testprog-net.pid
Writing output to: /var/testprog/testprg-net.txt
Iteration count: -1
Network functionality disabled

[james@selinux-dev2 lab16-booleans]$ tail -f /var/testprog/testprg-net.txt
Hello World
abcdefghij
Hello World
abcdefghij
```

The rest of our application works just fine, as the rest of our loadable policy remains untouched.

# The End!

I hope you have found running through these labs useful - if you have any feedback please do let me know!
