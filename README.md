
# Description 

* This is a **file compression service** code which starts a server process which is configured via number of segments and segment size and provides compression      service to other processes. The compression is done via the snappyC developed by google ( C version) in the background.

* The communication between the server and the client takes place via the posix message queues.

* Main thing in the code is data sharing via shared memory management to speed up the data transfer between server and client and also the implementattion of the Qos mechanism when you have many clients. 

* The code has a server and a library ( which supports two methods of calling server sync and async and sync is done via msg_receive in posix message queues and async is done via the signal handler ( this is more productive as the client is not blocked and can do its work in the meantime).

## Building code
* to build the code run all_build.sh ( this should be in the same folder as that of snappy c and other files, i have already place all that in one)




* to run the server run   ./runscript 2 64 ( this means run 2 segments of 64 size) 

* to run thhe client run  ./client_script --file ./test1.txt --state SYNC 
* these two script should be run on two different terminals  

( the file should be in same directory and please don't provide absolute path)




* these two scripts requires two terminals to run as they are communicating. 

## Code description

* The main files are client_combined and server_combined.
* Comments are added in the code for understanding. 
* if you are reading the code read the sync one , easier to understand , check this system architecture diagram. 
* ![Overall Architecture](/images/fig1.jpeg)
* Check this diagram which shows the communication between server and the libraray ( client).
* ![Server and Client](/images/fig3.png)
