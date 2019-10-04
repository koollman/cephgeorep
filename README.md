# cephgeorep
Ceph File System Remote Sync Daemon  
For use with a distributed Ceph File System cluster to georeplicate files to a remote backup server.  
This daemon takes advantage of Ceph's `rctime` directory attribute, which is the value of the highest `mtime` of all the files below a given directory tree. Using this attribute, it selectively recurses only into directory tree branches with modified files - instead of wasting time accessing every branch.

## Prerequisites
You must have a Ceph file system. `rsync` must be installed on both the local system and the remote backup. You must also set up passwordless SSH from your sender (local) to your receiver (remote backup) with a public/private key pair to allow rsync to send your files without prompting for a password. For compilation, boost development libraries are needed. The binary provided is statically linked, so the server does not need boost to run the daemon.

## Runtime Dependencies
Since the binary is statically linked, no boost runtime libraries are needed on the system. It only requires that `rsync` is installed.

## Installation Instructions
Download the provided binary and move it to /usr/bin/ as cephfssyncd. Place cephfssyncd.service in /etc/systemd/system/. Run `systemctl enable cephfssyncd` to enable daemon startup at boot. A default configuration file will be created by the daemon at /etc/ceph/cephfssyncd.conf, which must be edited to add the sender sync directory, receiver host, and receiver directory.

## Build Dependencies
* `g++`, the Gnu C++ compiler
* boost development libraries for C++

## Build Instructions
First clone the repository, then run `make` and move the build output to /usr/bin/ as cephfssyncd. Place cephfssyncd.service in /etc/systemd/system/.

## Configuration
Default config file generated by daemon: (/etc/ceph/cephfssyncd.conf)

```
# local backup settings
SND_SYNC_DIR=               # full path to directory to backup
IGNORE_HIDDEN=false         # ignore files beginning with "."
IGNORE_WIN_LOCK=true        # ignore files beginning with "~$"

# remote settings
REMOTE_USER=                # user on remote backup machine (optional)
RECV_SYNC_HOST=             # remote backup machine address/host
RECV_SYNC_DIR=              # directory in remote backup

# daemon settings
LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/
SYNC_FREQ=10                # time in seconds between checks for changes
RCTIME_PROP_DELAY=100       # time in milliseconds between snapshot and sync
COMPRESSION=false           # rsync compression
LOG_LEVEL=1
# 0 = minimum logging
# 1 = basic logging
# 2 = debug logging
# If not remote user is specified, the daemon will sync remotely as root user.
# Propagation delay is to account for the limit that Ceph can
# propagate the modification time of a file all the way back to
# the root of the sync directory.
# Only use compression if your network connection to your
# backup server is slow.
```

\* The Ceph file system has a propagation delay for recursive ctime to make its way from the changed file to the
top level directory it's contained in. To account for this delay in deep directory trees, there is a user-defined
delay to ensure no files are missed. 

~~`Small directory tree    (0-50 total dirs):        RCTIME_PROP_DELAY=1000`~~  
~~`Medium directory tree   (51-500 total dirs):      RCTIME_PROP_DELAY=2000`~~  
~~`Large directory tree    (500-10,000 total dirs):  RCTIME_PROP_DELAY=5000`~~  
~~`Massive directory tree  (10,000+ total dirs):     RCTIME_PROP_DELAY=10000`~~  

This delay was greatly reduced in the Ceph Nautilus release, so a delay of 100ms is the new default. This was able to sync 1000 files, 1MB each, randomly placed within 3905 directories without missing one. If you find that some files are being missed, try increasing this delay.

## Usage
Launch the daemon by running `systemctl start cephfssyncd`, and run `systemctl enable cephfssyncd` to enable launch at startup. To monitor output of daemon, run `watch -n 1 systemctl status cephfssyncd`.

## Notes
* If your backup server is down, cephfssyncd will try to launch rsync and fail, however it will retry the sync at 25 second
intervals. All new files in the server created while cephfssyncd is waiting for rsync to succeed will be synced on the next cycle.  
* Windows does not update the `mtime` attribute when drag/dropping or copying a file, so files that are moved into a shared folder will not sync if their Last Modified time is earlier than the most recent sync. 
* When the daemon is killed with SIGINT, SIGTERM, or SIGQUIT, it saves the last sync timestamp to disk in the directory specified in the configuration file to pick up where it left off on the next launch. If the daemon is killed with SIGKILL or if power is lost to the system causing an abrupt shutdown, the daemon will resync all files modified since the previously saved timestamp.
* If the REMOTE_USER is specified as a user that does not exist on the remote backup server, `rsync` will prompt for the user's password. Since it doesn't exist, when SSH fails the daemon will act as if the remote server is down and retry `rsync` every 25 seconds.

[![45Drives Logo](https://www.45drives.com/img/45-drives-brand.png)](https://www.45drives.com)
