# LAB 1 - Up and Running

## Introduction

Welcome! Let's get you up and running so that you can start to experience SELinux development! This lab will ensure you have all pre-requisites in place to run the other labs.

## Pre-requisites

These labs are designed to be run on a Red Hat Enterprise Linux or CentOS 7 system. They have been developed and tested on RHEL 7.4 x86_64 but should work well on other EL7 systems. The basic premises and even some of the commands should work on other SELinux enabled systems but I know some commands have changed since RHEL 6 so YMMV.

It is recommended you run these labs on a system that is not mission critical, as you will be playing with SELinux policies and worst case scenario you will break something you didn't intend to! Not a good idea on a production system!

All labs were created and tested on a system resulting from a **Minimal Install** of RHEL 7 so depending on your installation you may find you already have packages installed that you are asked to install in these labs. That is fine. Just ignore the **yum install** steps in those cases.

Your test system must be capable of running in SELinux enforcing mode. The labs at the time of writing are designed to demonstrate the **targeted** SELinux policy that comes as default on most EL7 systems. To check this, run the following command and check that the output matches that shown below:

```
[james@selinux-dev ~]$ sestatus
SELinux status:                 enabled
SELinuxfs mount:                /sys/fs/selinux
SELinux root directory:         /etc/selinux
Loaded policy name:             targeted
Current mode:                   enforcing
Mode from config file:          enforcing
Policy MLS status:              enabled
Policy deny_unknown status:     allowed
Max kernel policy version:      28
```
Of the output shown above, the following pieces are of interest to us:

* The **SELinux status** which is **enabled**
* The **Loaded policy name** which is **targeted**
* The **Current mode** which is **enforcing**

## Additional packages

As we go through the labs, we will install additional packages as required so you will want to make sure you have a working yum repository for your system to hand, or another source for the packages. Rather than install these all up front, we will install them as they become necessary so that you know which package performs which function.

## Get the code

Finally, if you're reading this on github.com, clone the repository to ensure you have all the code locally:

```
[james@selinux-dev2 ~]$ sudo yum -y install git
...
<output truncated>
...
[james@selinux-dev2 ~]$ git clone https://github.com/jamesfreeman959/selinux-testprog.git
Cloning into 'selinux-testprog'...
...
<output truncated>
...
```
## Now proceed to lab 2!

Congratulations - you have completed the first lab! That's all that's required here so please move on to lab 2.

