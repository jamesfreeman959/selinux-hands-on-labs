# Putting It All Together

Now it's time to put all this together and build a working policy to enable us to run testprog from the shell. First of all we shall decide what contexts to put the files and directories into as we shall build the policy around that. To follow the practise used for other applications on EL7 (including our reference policy **ntp**) our file contexts could look like this:

```
[root@selinux-dev lab9-putting-it-all-together]# cat testprog.fc
/usr/bin/testprog	-- system_u:object_r:testprog_exec_t:s0
/etc/testprog.conf	-- system_u:object_r:testprog_conf_t:s0
/var/run/testprog.pid	-- system_u:object_r:testprog_var_run_t:s0
/var/testprog(/.*)?	   system_u:object_r:testprog_data_t:s0
```

In this way we have defined a type for the executable, a configuration file type to ensure we can read the configuration file but in isolation from the test of the /etc directory, a type for the pid file as discussed in previous labs, and a type for the data directory. Note we have used a wildcard for the data directory to cover any file in there - this is an example here to show the alternative syntax and the regular expression style syntax that does in the fc file.

Now that we have this created we can build the policy. Build on our previous policy from earlier, and the reference code we have taken from the **ntp** policy, testprog's policy should look something like this:

```
[james@selinux-dev2 selinux-testprog]$ cat testprog.te
policy_module(testprog, 0.2)

# Require all the types, attributes and classes we reference in this policy
require {
	type unconfined_t;
	role unconfined_r;
	class file { ioctl getattr setattr create read write unlink open relabelto };
	class dir { search write add_name };
	class process transition;
	type fs_t;
	class filesystem getattr;
	type console_device_t;
	type user_devpts_t;
	class unix_dgram_socket { create connect sendto };
	class chr_file { append read write open getattr ioctl };
	type devlog_t;
	class sock_file write;
	type kernel_t;
	type var_run_t;
	class capability sys_tty_config;
}

# Define our new types that testprog will use, and ensure that we tell the policy that testprog_exec_t is a file
type testprog_t;
domain_type(testprog_t);
type testprog_exec_t;
files_type(testprog_exec_t);
type testprog_conf_t;
files_config_file(testprog_conf_t);
type testprog_var_run_t;
files_pid_file(testprog_var_run_t);
type testprog_data_t;
files_type(testprog_data_t);

# Allow the testprog_t type under the unconfined_r role
role unconfined_r types testprog_t;

# Tell SELinux that testprog_exec_t is an entrypoint to the tetprog_t domain
allow testprog_t testprog_exec_t : file { ioctl read getattr lock execute execute_no_trans entrypoint open } ;
# Make the type transition from unconfined_t (i.e. user shell) to testprog_t
type_transition unconfined_t testprog_exec_t : process testprog_t;
# Explicitly allow the type transition we have just created
allow unconfined_t testprog_t : process transition ;

# Allow testprog to read it's config file using Red Hat's code for ntpd as example
allow testprog_t testprog_conf_t:file read_file_perms;

# We know from sealert that these allow rules are required, largely for writing to the console
allow testprog_t console_device_t:chr_file { open write getattr ioctl };
allow testprog_t self:capability sys_tty_config;
allow testprog_t user_devpts_t:chr_file { append read write getattr };

# Allow testprog access to it's data directory
allow testprog_t testprog_data_t:dir { search write add_name };
allow testprog_t testprog_data_t:file { create open write append getattr };

# Allow syslog access based on Red Hat's code for ntpd
logging_send_syslog_msg(testprog_t)

# Allow testprog to create and write it's PID file - based on code from Red Hat for ntpd
manage_files_pattern(testprog_t, testprog_var_run_t, testprog_var_run_t)
files_pid_filetrans(testprog_t, testprog_var_run_t, file)
```

This as you'll see is an amalgamation of our previous context transition policy, rules from the **ntp.te** policy file, and a set of individually created rules that were derived from the audit log and sealert. In cases where rules were taken from the audit log the process of development was sometimes iterative, allowing one permission only to find a subsequent one was missing.

Take for example the **testprog_data_t:file** rule - the initial AVC denial was for the **open** operation on our data file. This results in a rule like:

```
allow testprog_t testprog_data_t:file open;
```

However once **open** was allowed and the new policy installed and testprog run again, it was found that testprog could not append to the file. Thus over several iterations of testing we ended up with:

```
allow testprog_t testprog_data_t:file { create open write append getattr };
```

This includes both the case where the data file does not exist so has to have the **create** and **write** permissions, as well as when it does and so needs the **open** and **append** permissions. The **getattr** permission is needed to look up the file when it exists before it is opened and written to.

Let's try building, installing and then testing the policy:

```

```
