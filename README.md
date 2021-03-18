

#to build the code run all_build.sh ( this should be in the same folder as that of snappy c and other files, i have already place all that in one)



these two script should be run on two different terminals 
to run the server run  

./runscript 2 64 ( this means run 2 segments of 64 size) 

to run thhe client run 

./client_script --file ./test1.txt --state SYNC 

( the file should be in same directory and please don't provide absolute path)




these two scripts requires two terminals to run as they are communicating. 



I have tried running them on a single terminal using 

Bash run_scipt.sh & Bash client_script.sh 

but this command does not allow inputs to the script so inputs are hard coded in that. so if you want to run the program in a single terminal then Bash run_script_test.sh & Bash client_script.sh



The main files are client_combined and server_combined.

this client script run 5 instances of the same client to the server and mesaures timem ( 5 were taken and then value was averaged to calcullate CST). 


for QOS separate files are added which sends multiple request to the server from the same client and corressponding server file is also there. 

building mechanism is same as that above.
