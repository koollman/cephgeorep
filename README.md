# cephgeorep
Ceph File System Remote Sync Daemon  
For use with a distributed Ceph File System cluster to georeplicate files to a remote backup server.  
This daemon takes advantage of Ceph's `rctime` directory attribute, which is the value of the highest `mtime` of all the files below a given directory tree. Using this attribute, it selectively recurses only into directory tree branches with modified files - instead of wasting time accessing every branch.

## Prerequisites
You must have a Ceph file system. `rsync` must be installed on both the local system and the remote backup. You must also set up passwordless SSH from your sender (local) to your receiver (remote backup) with a public/private key pair to allow rsync to send your files without prompting for a password. For compilation, boost development libraries are needed. The binary provided is statically linked, so the server does not need boost to run the daemon. 

## Installation Instructions
Download the provided binary and move it to /usr/bin/ as cephfssyncd. Place cephfssyncd.service in /etc/systemd/system/. Run `systemctl enable cephfssyncd` to enable daemon startup at boot. A default configuration file will be created by the daemon at /etc/ceph/cephfssyncd.conf, which must be edited to add the sender sync directory, receiver host, and receiver directory.

## Build Instructions
First clone the repository, then run `make` and move the build output to /usr/bin/ as cephfssyncd. Place cephfssyncd.service in /etc/systemd/system/. 

## Configuration
Default config file generated by daemon: (/etc/ceph/cephfssyncd.conf)

```
SND_SYNC_DIR=                               (path to directory you want backed up)
REMOTE_USER=                                (optional - if included, launches rsync with REMOTE_USER@RECV_SYNC_HOST)
RECV_SYNC_HOST=                             (ip address/hostname of your backup server)
RECV_SYNC_DIR=                              (path to backup directory)
LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/   (path to where the last modification time is stored)
SYNC_FREQ=25                                (frequency to check directory for changes, in seconds)
IGNORE_HIDDEN=false                         (change to true to ignore files and folders starting with '.')
IGNORE_WIN_LOCK=true                        (ignore files beginning with "~$")
RCTIME_PROP_DELAY=100                       (delay between taking snapshot and searching for new files*)
COPMRESSION=false                           (compresses files before sending for slow network)
LOG_LEVEL=1                                 (log output verbosity)
# 0 = minimum logging
# 1 = basic logging
# 2 = debug logging
```

\* The Ceph file system has a propagation delay for recursive ctime to make its way from the changed file to the
top level directory it's contained in. To account for this delay in deep directory trees, there is a user-defined
delay to ensure no files are missed. 

~~`Small directory tree    (0-50 total dirs):        RCTIME_PROP_DELAY=1000`~~  
~~`Medium directory tree   (51-500 total dirs):      RCTIME_PROP_DELAY=2000`~~  
~~`Large directory tree    (500-10,000 total dirs):  RCTIME_PROP_DELAY=5000`~~  
~~`Massive directory tree  (10,000+ total dirs):     RCTIME_PROP_DELAY=10000`~~  

This delay was greatly reduced in the Ceph Nautilus release, so a delay of 100ms is the new default. This was able to sync 1000 files, 1MB each, randomly placed within 3905 directories without missing one. With very large directory trees this delay may need to be increased, but only up to a few seconds rather than up to 10.

## Usage
Launch the daemon by running `systemctl start cephfssyncd`, and run `systemctl enable cephfssyncd` to enable launch at startup. To monitor output of daemon, run `watch -n 1 systemctl status cephfssyncd`.

## Notes
* If your backup server is down, cephfssyncd will try to launch rsync and fail, however it will retry the sync at 25 second
intervals. All new files in the server created while cephfssyncd is waiting for rsync to succeed will be synced on the next cycle.  
* Windows does not update the `mtime` attribute when drag/dropping or copying a file, so files that are moved into a shared folder will not sync if their Last Modified time is earlier than the most recent sync. 
* When the daemon is killed with SIGINT, SIGTERM, or SIGQUIT, it saves the last sync timestamp to disk in the directory specified in the configuration file to pick up where it left off on the next launch. If the daemon is killed with SIGKILL or if power is lost to the system causing an abrupt shutdown, the daemon will resync all files modified since the previously saved timestamp.

[![45Drives Logo](https://www.45drives.com/img/45-drives-brand.png)](https://www.45drives.com)
