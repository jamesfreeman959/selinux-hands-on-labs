# Targeting testprog

By this stage we've established that we can run testprog on our lab system, either from the shell or through systemd. This is all well and good but remember the binary is running in the **unconfined** space which is largely unrestricted - we don't actually want that - we want to know that our application is secured by SELinux and that even if an attacker compromised it, SELinux would contain the attack from further compromise of the system.

Now you might thing, "yes but we have the source code so we can fix it or make more secure". Remember that in many cases software vendors only provide pre-compiled binaries and as such you won't have this privilege. Hence we want to target testprog and confine it.

To do this we need to write a simple SELinux policy. I will leave reading the documentation about how this policy is created as an exercise for the reader (hint: https://selinuxproject.org/page/TypeRules) - however the files you need are provided in this lab subdirectory. To build and test the policy do the following:

```
[james@selinux-dev2 selinux-testprog]$ sudo yum -y install selinux-policy-devel
...

```
