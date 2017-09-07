# Introduction

Welcome to testprog! This is a little C program that was designed to perform a series of common functions on a RHEL 7 system to help you learn SELinux. It is by it's nature very simple and also at this stage quite crude. If anyone wants to help improve the code I would be very grateful - however for now it meets my original goals:

* A native binary (as opposed to a script requiring another binary to run it)
* Can be controlled by systemd
* Write to syslog and the terminal
* Writes output data to a known unique data directory on the system
* Reads its configuration data from a unique config file in /etc

In time additional functionality may be added, especially network connectivity - however at this stage it touches enough parts of the system to learn the fundamentals of SELinux on an EL7 system operating the targeted policy in enforcing mode.

If you get a **Segmentation Fault**, this is almost certainly because of one of these things:

* The configuration isn't correct (the default config should work though as it has been tested on a plain EL7 system)
* The configuration is correct but the binary cannot perform an operation it needs to

Error handling is currently limited resulting in **Segmentation Faults** - this may be improved if time allows or someone can contribute to the code.

# Build and install

To build and install testprog exactly as it is, run the following commands:

```
[james@selinux-dev2 testprog]$ cd ~/selinux-testprog/testprog
[james@selinux-dev2 testprog]$ sudo yum -y install gcc make
...
<output truncated>
...
[james@selinux-dev2 testprog]$ make
gcc -o testprog testprog.c
[james@selinux-dev2 testprog]$ sudo make install
mkdir -p /usr/bin
cp testprog /usr/bin/testprog
cp testprog.conf /etc/testprog.conf
mkdir -p /var/testprog
cp testprog.service /etc/systemd/system/testprog.service
```

And optionally:

```
[james@selinux-dev2 testprog]$ sudo systemctl enable testprog
Created symlink from /etc/systemd/system/multi-user.target.wants/testprog.service to /etc/systemd/system/testprog.service.
```

If that completed without error, congratulations, you can move on to the next lab! If it doesn't please raise an Issue against this project and I'll help you out.

# Uninstall

As you can see from the above output, testprog doesn't install much. If you want to tidy it up, simply remove the items you see resulting from the `make install` stage. You can also try:

```
[james@selinux-dev2 testprog]$ sudo make uninstall
rm -f /usr/bin/testprog
rm -f /etc/testprog.conf
rm -f /etc/systemd/system/testprog.service
rm -rf /var/testprog
```

