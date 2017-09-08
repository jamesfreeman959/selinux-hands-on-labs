# About macros

Let's proceed with understanding how the ntp service, which runs in a confined domain, writes its own PID file into /var/run - we can hopefully make use of this to create our own policy.

Here's a disection of the ntp policy file, showing the parts that are relevant to the PID file. Once you get used to the nomenclature used in SELinux and the policies, this will get easier to find, and I recommend you stick to the same standards so that others can understand your policy files when you hand them over.

First off, is there a file context specifically for the PID file so that, per our previous desire, we can isolate this file and allow our confined application to write this file, but not everything else in that directory (remember our .fc file from before?):

```
[james@selinux-dev2 serefpolicy-contrib-3.13.1]$ grep pid ntp.fc
/var/run/ntpd\.pid	--	gen_context(system_u:object_r:ntpd_var_run_t,s0)
```

Excellent! Now we know what to look for in the policy file:

```
[james@selinux-dev2 serefpolicy-contrib-3.13.1]$ cat ntp.te
policy_module(ntp, 1.11.0)

########################################
#
# Declarations
#
...

type ntpd_var_run_t;
files_pid_file(ntpd_var_run_t)
...

########################################
#
# Local policy
#
...
manage_files_pattern(ntpd_t, ntpd_var_run_t, ntpd_var_run_t)
files_pid_filetrans(ntpd_t, ntpd_var_run_t, file)
...
```

We found all the pieces that fit together, simply by matching up names. The type declaration we have seen before - this is simply declaring a new type, but what of the other lines? These look a bit different to the kind of allow rules we created when we built our testprog domain transition policy.

Where you see lines in a policy like this, these are macros, and are used to make the policy files more readable, and save developers from writing the same chunks of code over and over again. Essentially this follows the usual best practises concerning code reuse. If you're curious though - and you might rather than to simply take a macro someone else created without understanding what they do, how do you understand the macros? Thankfully we have the source code for these too - we just have to find the macros.

You can do this by hand, but to make your life easier I thoroughly recommend the SELinux functions developed by Sven Vermeulen. At the time of writing you can download these and use them as follows (URL correct at time of writing):

```
[james@selinux-dev2 selinux-testprog]$ curl -O http://dev.gentoo.org/~swift/blog/01/selinux-funcs.txt
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  2273  100  2273    0     0   4071      0 --:--:-- --:--:-- --:--:--  4073

[james@selinux-dev2 selinux-testprog]$ source selinux-funcs.txt
[james@selinux-dev2 selinux-testprog]$ export POLICY_LOCATION=$HOME/selinux-testprog/refpolicy/serefpolicy-3.13.1
[james@selinux-dev2 selinux-testprog]$ seshowif files_pid_filetrans
interface(`files_pid_filetrans',`
	gen_require(`
		type var_t, var_run_t;
	')

	allow $1 var_t:dir search_dir_perms;
	allow $1 var_run_t:lnk_file read_lnk_file_perms;
	filetrans_pattern($1, var_run_t, $2, $3, $4)
')
```

This macro saves developers from writing 4 lines of code every time they want to allow a confined application to write a PID file into /var/run by:

1. Declaring the types required
2. Allowing the first passed argument to access the **var_t** type (i.e. /var) to search directories (i.e. to find /var/run)
3. Allowing the first passed argument when the macro is called (ntp_t in our example) to access the **var_run_t** type (i.e. /var/run) with permission to read link files (on EL7 /var/run is a symbolic link to /run).
4. Another macro is then called allow access to write files into the **var_run_t** type (so that we can create our PID file in /var/run)

If we so chose we could expand this second macro, and so on until we have a complete picture of all the policies that make up a particular macro:

```
[james@selinux-dev2 selinux-testprog]$ seshowdef filetrans_pattern
define(`filetrans_pattern',`
	allow $1 $2:dir rw_dir_perms;
	type_transition $1 $2:$4 $3 $5;
')
```

Briefly, we can see this is allowing **ntp_t** ($1) access to **var_run_t** with permissions to read and write the directory. It also specifies a type transition to ensure we can write into this context. Note that even the permissions themselves are macros to save on coding:

```
[james@selinux-dev2 selinux-testprog]$ seshowdef rw_dir_perms
define(`rw_dir_perms', `{ open read getattr lock search ioctl add_name remove_name write }')
```

Once you are happy with what macros are, and how they work, you can start to use them to build your own policy. Note that you don't have to use macros - however you can see how much that one line we have explored here has expanded to as we have explored the macros, so it makes sense to use them if you feel they will achieve your aims.

We'll put all this together in the next lab and actually get testprog running from the shell in a confined domain!

