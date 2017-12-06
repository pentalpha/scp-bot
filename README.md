scp-bot
=========
Application to synchronize files and folders across computer systems using scp (previous name: octo-sync).

scp-bot is a computer program capable of automatically synchronizing a folder in different computers, connected through a network. Once the user changes or creates files/folders in the folder being synchronized, scp-bot will automatically transfer the changes to the other computer.

## Usage Overview ##
--------------------
- Install it;
- Open it on the folder you want to synchronize, to start hosting it;
- Open it on the other computer, which will synchronize with the host;
- The user can do what he wants inside the folder, scp-bot will take care of the rest;

## Packages required ##
-----------------------
- scp: Command to transfer files over a network using ssh encryption. scp-bot uses this command to do the actual file transfers;
- sshpass: Used to pass user password directly to scp, without prompting the user to write it;
- openssh-server: To receive files with scp from other machine, a machine needs to be running a ssh server, this package installs the openssh server;
- To build: GCC (>= 6.3 recommended) and make

## Installation ##
------------------
```sh
$ git clone https://github.com/pentalpha/scp-bot.git
$ cd scp-bot
$ make
```
## Usage ##
-----------
```sh
$ ./scp-bot host hostAddress=[local-ip-address] hostPasswd=[a password]
```

The host will wait for a connection from other computer on the network:

```sh
$ ./scp-bot sync hostAddress=[host-ip-address] hostPasswd=[same password from above]
```

More detail on the arguments can be accessed with the -h argument, the output is the following:

```sh
$ ./scp-bot -h
sync or host
	The operation to perform: either host or sync;
	The host must be the first instance to be executed, it will wait for a connection;
	The sync connects to a host that is waiting for a new connection;
Use [arg-name]=[value] to pass arguments:
* = obrigatory
syncDir
	 The directory to synchronize. Default = ./;
hostAddress
	* Hosting machine ip;
hostPort
	 Hosting machine port. Default = 50002;
localPasswd
	 Local machine (sync or host) user password. The remote will use this password on the scp command;
hostPasswd
	* Server password of the host. Used to login;
scpPort
	Specify a port for the scp command. If not specified, the default will be used;
logLevel
	Minimum level of log messages. A lower value means more output. Default = 7;
```

## Future features ##
---------------------

- Trash: Once a file is deleted with scp-bot, there is no going back. A trash folder would make the program more safe.
- Stop using user password: sending the password of the user over the network is not safe at all. Future versions of scp-bot will use ssh keys.
- Multiple machines connected to host.


