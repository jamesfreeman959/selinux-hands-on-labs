# Interfaces

So far we have largely ignored the `testprog.if` file that was created as part of the template for our policy. We know that the `.if` file sets the default file contexts, and the `.te` fileis the actual policy. What about the `.if`? This file denotes the interfaces for the policy, and in short creates macros that other modules will use to access our resources. So far we have treated testprog as totally self contained, but what if we wanted another process to access it's data from a different domain.

To do this, we'll take a copy of the standard `cat` command and set up a special policy called `testcat` that will confine it to its own domain. In real life this would be another daemon or service, but making use of this existing command keeps our lives simple for the sake of these labs.

To get started, let's take a copy of the cat command, and then set up a special policy module for it that causes it to switch to its own confined context:

```
[root@selinux-dev lab11-interfaces]# cp /usr/bin/cat /usr/bin/testcat

```

