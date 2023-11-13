# CSDS-325-Networks
For CSDS 325 class projects

Logging into the remoteclass server:
chkm823@BigBoiL:~$ ssh cxk445@eecslab-12.case.edu
cd home/CSDS325/[project folder]

Moving all files in current folder using SCP:
scp [* or name of file to be moved] cxk445@eecslab-12.case.edu:home/CSDS325/[project name]

Extract from a tar folder:
tar -xf [name of tar folder]
Compress into a tar folder:
tar cvzf [tar_folder_output_name].tar.gz [included_files]

To run a test file, _bash [test_file]_
To run a Makefile, _make_

To use the debugger (dbg), 
gdb [project_name]
run [arguments]
(https://www.bitdegree.org/learn/gdb-debugger)

To open a file on linux terminal,
vi [file_name]
(https://www.redhat.com/sysadmin/introduction-vi-editor)

To cancel previous n pushes,
git reset HEAD^ 
for how many commits ahead the local branch is