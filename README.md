# Introduction

This project started from one simple requirement - to get to grips with SELinux for a project at work. SELinux has been around for years and whilst it has been championed by many in the industry, in my career I have witnessed many others shun it. I have worked with many companies where the solution to SELinux issues was simply to turn it off, and indeed I know of several software vendors who state that SELinux should be turned off in their installation instructions.

With the advent of the GDPR this solution is no longer acceptable. Whilst this project does not intend to get into any debate on the meaning or implementation of the GDPR, it is based on the premise that in the event of an attack on a system (whether that attack was successful or not), it would have been better to have taken advantage of this additonal security layer than to have simply turned it off because it was deemed too complex or difficult to get working.

# Scope

At this stage, this project does not aim to be a comprehensive coverage of SELinux - it is a huge and powerful security layer and there are many excellent references texts on it. I have always learned better by doing than by reading or sitting in a lecture or webinar, so I decided to come up with a set of labs where you can safely learn some of the more common SELinux fundamentals and hopefully demystify it.

As such the scope of this project is a very common scenario that I have come up against many times in my career:

* The application to be secured is not SELinux aware and has no specific coding to work with or alongside SELinux
* The hosting machine is running Red Hat Enterprise Linux or a derivative (e.g. CentOS or OEL)
* The host machine has SELinux enabled and in enforcing mode
* The host machine is using the targeted policy

MLS is beyond the scope of this project at this stage but may be added if there is a requirement for it.

# Getting started

I have endeavoured to provide all the information you need to get started and run these labs, and more information can be found in Lab 1 which I recommend you proceed to straight away. I do recommend working through these labs on a VM set aside for this purpose as although the labs are designed to be self contained and not affect the other part of the host system, any tinkering or testing things outside the bounds of the lab (which is highly recommended if it helps you learn more!) could have an undesirable effect on the system.

# Credits

There have been many many sources that have helped me put this project together and I have tried to include them in the comments section of each file where relevant. Special mention is deserved to:

* Gentoo Linux for their excellent SELinux tutorials - found at: https://wiki.gentoo.org/wiki/SELinux/Tutorials
* Sven Vermeulen for his SELinux Cookbook

