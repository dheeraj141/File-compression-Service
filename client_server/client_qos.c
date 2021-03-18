#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/mman.h>
#include "shared_mem.h"
#include <signal.h>
#include <errno.h>
#include<sys/time.h>
#define SERVER_QUEUE_NAME "/project2"
#define DEBUG 1 
#define QUEUE_PERMISSIONS 0660

#define MAX_MESSAGES 10
#define MESSAGE_SIZE 256
#define MAX_BUF_SIZE 300
int segment_size = 0;
 mqd_t cqt, sqt;
char queue_name[MAX_BUF_SIZE];

    char segment_name[64];
struct sigevent sev;
long long int data_sent = 0;
FILE *file_pointer, *new_fp;
long long total_data_size = 0;
int get_file_size( char *file_name,long long *file_size)
{
        int fd;


        int result =1;

        fd = open( file_name, O_RDONLY);
        struct stat st;

        if (fstat(fd, &st) >= 0)
        {
                *file_size =( long long )st.st_size;
             //   printf(" file size is %lld\n",  *file_size);
        total_data_size = (*file_size);

        }
        else
                result=-1;

        close( fd);

        return result;

}


// function for writing the data to the segment
void write_segment( FILE *fp, char *segment_name )
{




     

    // shared memory
    int fd_shm;



    if( ( fd_shm = shm_open(segment_name, O_RDWR , 0660 )) == -1)
    {
        printf("client side :error opening the shared memory\n");
    }

    //printf("shared memory opened\n");

    char *data;

    if(( data =(char*)mmap( 0, segment_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd_shm, 0)) == MAP_FAILED)
    {
        printf("server: error in mmap\n");
    }

    //printf("mmap done\n");
    memset( data, 0, segment_size);
    char c ;
    long count = 0;

    //long size  = min( segment_size, file_size);

    while( count< segment_size &&  (c = getc(fp) ) != EOF )
    {
           //printf("%c", c);
            data[count++] = c;
    }

    data_sent+=segment_size;


    munmap( data, segment_size);

    close(fd_shm);
   
    //printf("write segment done\n");




}

int send_message( char *msg )
{
    while(1)
    {
        if( mq_send( sqt, msg, strlen( msg)+1, 0) == -1)
        {
             perror("client: not able to send message to server\n");
             continue;
                               
        }
        else
        {
            //printf("sent the request to server\n");
            break;
        }
                
             
                
    }

    return 1;




}
int receive_message(char *msg)
{
     char buffer1[MAX_BUF_SIZE];

    while( 1)
    {
        // requesting the server
        if( mq_receive( cqt, buffer1, MESSAGE_SIZE, NULL) == -1)
        {
            perror("client: error in receiving message");
            continue;
        }
        //printf("response received from server is %s\n\n",  buffer1);
        //printf("exiting\n");
        break;
    }
    strcpy( msg, buffer1);
    return 1;


}

void copy_data_to_file( FILE *fp,  char *segment_name)
{
    // shared memory
    int fd_shm;



    if( ( fd_shm = shm_open(segment_name, O_RDWR , 0660 )) == -1)
    {
        printf("client side :error opening the shared memory\n");
    }

    //printf("shared memory opened\n");

    char *data;

    if(( data =(char*)mmap( 0, segment_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd_shm, 0)) == MAP_FAILED)
    {
        printf("server: error in mmap\n");
    }

    
  
     long int i = 0;
    while( i<segment_size && data[i]!='\0')
    {

            fputc(data[i], fp);
        i++;
    }
    
    munmap( data, segment_size);
    close(fd_shm);
    
}


void cleanup_resources()
{
    fclose(file_pointer);
    fclose(new_fp);
}


static void data_copier( int sig)
{

        printf("entered the signal handler\n");
    
    char buffer[MAX_BUF_SIZE];
    
            char msg[ MAX_BUF_SIZE];
    char send[6] ="SEND ";
    if (mq_notify(cqt, &sev) == -1)
    {
        printf("error in registering mq_notify\n");
        return ;
        
    }


    //printf("reached checkpoint 2\n");
    int numRead = 0;
    
   while (1)
   {

    

    


    numRead = mq_receive(cqt, buffer, MESSAGE_SIZE, NULL);
    //printf("numRead is %d\n", numRead);
        if(numRead <0)
        break;

    


    #if DEBUG
    printf("msg received is %s\n", buffer);

    #endif
        
        char msg_type[16];
        int count = 0;
        while( buffer[count] != ' ')
        {
            msg_type[count] = buffer[count];
            count++;
        }
        
        msg_type[count] = '\0';
        count++;
        if( strcmp( msg_type, "SEG")== 0)
        {
        int k = 0;

        while( buffer[count]!= ' ')
        {
            segment_name[k] = buffer[count];
            k++; count++;
        }
        count++;
        segment_name[k]='\0';
        while( buffer[count] !='\0')
        {
            segment_size = segment_size*10 + ( buffer[count] -'0');
            count++;
        }



           // printf("extracted the message\n");
        //strcpy( segment_name, buffer+count);
        //printf("segment name is %s\n", segment_name);
        write_segment( file_pointer, segment_name);
        
        strcpy(msg, queue_name);

            strcat(msg," ");
        strcat(msg, send);
            if(send_message( msg) == 1)
        {
            
        #if DEBUG
            printf("sent message to the server\n");
    
        #endif
        }

            // segment id sent so now open the memory and copy the data and send the SEND signal to the server
        }
    else if( strcmp(msg_type, "ACK") == 0)
    {
    
        char msg1[MAX_BUF_SIZE];
    

        //printf("entered the ACK if condition\n");
    
        if( data_sent >= total_data_size  )
        {
            printf("data already sent");
            continue;
        }
        write_segment(file_pointer, segment_name);
        
               int i = 0;
        while(queue_name[i]!='\0')
        {
            msg1[i] = queue_name[i];
            i++;
        }

        
        #if DEBUG
            printf("the message passed to server is %s\n", msg1);
    
        #endif

        
        msg1[i++] = ' ';
        msg1[i++] ='S'; msg1[i++] ='E'; msg1[i++] ='N'; msg1[i++]='D';
        msg1[i++] =' ';
        msg1[i++] = '\0';

            if(send_message( msg1) == 1)
        {
        #if DEBUG
            printf("the message passed to server is %s\n", msg1);
    
        #endif

        }
        
    }
    else if(strcmp(msg_type, "REC") == 0)
    {
        printf("entered the REC if condition\n");
            copy_data_to_file( new_fp, segment_name);
        strcpy(msg, queue_name);

            strcat(msg," ");
        strcat(msg, "ACK ");
            if(send_message( msg) == 1)
        {
        #if DEBUG
            printf("sent message to the server\n");
    
        #endif

        }
    }
    else if(strcmp(msg_type, "DONE") == 0)
    {
        printf("compression done\n");
        cleanup_resources();
    }
    }


    printf("reached the end of signal handler\n");
    if (errno != EAGAIN)
        printf("mq_receive error\n");
}


void install_signal_handler(int signo, void(*handler)(int))
{
    sigset_t set;
    struct sigaction act;

    /* Setup the handler */
    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;
    sigaction(signo, &act,0);
    
    /* Unblock the signal */
    sigemptyset(&set);
    sigaddset(&set, signo);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    
    return;
            
}



void ASYNC_call(int argc, char *argv[])
{
    
    
    
    
  
    char *file_name, msg[256], segment_name[256];

    char ack[5] ="ACK ";

    if( argc ==5)
            file_name = argv[2];
    else
    {
            printf("provide the file name\n");
            return ;
    }

    sprintf(queue_name, "/client-%d", getpid());
   printf("queue name is %s\n", queue_name);

    strcpy( msg, queue_name);

    strcat(msg," ");
    strcat( msg, "INIT ");
    long long int file_size = 0;
    if( get_file_size(file_name, &file_size) == -1)
    {
             printf("error in finding the file size\n");
            return ;
    }

    sprintf(msg+strlen(msg), "%lld", file_size);

    printf("message sent is %s\n", msg);
    struct mq_attr attributes;

    attributes.mq_flags = O_NONBLOCK;  // non  blocking
    attributes.mq_maxmsg = MAX_MESSAGES;
    attributes.mq_msgsize = MESSAGE_SIZE;
    attributes.mq_curmsgs = 0;
    
    


    // opening the client queue
    if(( cqt = mq_open( queue_name, O_RDONLY|O_CREAT|O_NONBLOCK, QUEUE_PERMISSIONS, &attributes)) == -1)
    {
            perror(" client side: error in opening the client queue\n");
            exit(1);
    }
    // opening the server queue

    if(( sqt = mq_open( SERVER_QUEUE_NAME, O_WRONLY)) == -1)
    {
            perror("client side:  error in opening the server queue\n");
            exit(1);
    }
    
        file_pointer = fopen( "./test1.txt","r");
    char new_file[64];
    strcpy(new_file, "./compresses_");
    strcat(new_file, file_name+2);
    new_fp = fopen(new_file, "a" );
    
    
    install_signal_handler( SIGUSR1, data_copier);
    
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    if (mq_notify(cqt, &sev) == -1)
    {
        printf("error in registering the mq_notify\n");
        return ;
        
    }
    
        

     char buffer1[MAX_BUF_SIZE];


   

    printf("requesting to server\n");

    if(send_message( msg) == 1)
    printf("sent message to the server\n");
       int i = 0;
    for(  i = 0 ; i<100; i++)
    {
        printf("%d\n", i);
        sleep(10);
    }


    
    // close the queue
    if( mq_close( cqt) == -1)
    {
        perror(" Client : error in closing the client queue\n ");
        exit(1);
    }

    if( mq_unlink( queue_name) == -1)
    {
        perror(" client: error in removing the client queue\n");
        exit(1);
    }

    //printf("client id %d with file size %lld  :completed in",getpid(),  file_size );

    exit(0);

    
    
    
   
    
    
    
    
    
    
    
}

void sync_call( int argc, char *argv[])
{
    char queue_name[256], *file_name, msg[256], segment_name[256];

char ack[5] ="ACK ";
char send[6] ="SEND ";

    if( argc ==5)
            file_name = argv[2];
    else
    {
            printf("provide the file name\n");
            return ;
    }

    sprintf(queue_name, "/client-%d", getpid());
   //printf("queue name is %s\n", queue_name);

    strcpy( msg, queue_name);

    strcat(msg," ");
strcat( msg, "INIT ");
    long long int file_size = 0;
    if( get_file_size(file_name, &file_size) == -1)
    {
             printf("error in finding the file size\n");
            return ;
    }

    sprintf(msg+strlen(msg), "%lld", file_size);

    //printf("message sent is %s\n", msg);
    struct mq_attr attributes;

    attributes.mq_flags = 0;  // blocking
    attributes.mq_maxmsg = MAX_MESSAGES;
    attributes.mq_msgsize = MESSAGE_SIZE;
    attributes.mq_curmsgs = 0;


    // opening the client queue
    if(( cqt = mq_open( queue_name, O_RDONLY|O_CREAT, QUEUE_PERMISSIONS, &attributes)) == -1)
    {
            perror(" client side: error in opening the client queue\n");
            exit(1);
    }
    // opening the server queue

    if(( sqt = mq_open( SERVER_QUEUE_NAME, O_WRONLY)) == -1)
    {
            perror("client side:  error in opening the server queue\n");
            exit(1);
    }
     char buffer1[MAX_BUF_SIZE];


   

    //printf("requesting to server\n");
    int num_times = 0; 
    for( num_times = 0;num_times <2;num_times++)
	{ 
    		if(send_message( msg) == 1)
    		{
    		#if DEBUG
    		    printf("sent message to the server is %s\n", msg);
		
		    #endif


	   	 }
	}
    for(num_times= 0;num_times <2;num_times++)
	{
    //printf( "waiting for response from server\n");

    if(receive_message( buffer1) == 1)
     {
    #if DEBUG
        printf("recieved message from server is %s\n" , buffer1);

    #endif

     }
    int i =0;
    while( buffer1[i]!= ' ')
    i++;
    i++;

    

    int k = 0;

    while( buffer1[i]!= ' ')
    {
        segment_name[k] = buffer1[i];
        k++; i++;
    }
    i++;
    segment_name[k]='\0';
    while( buffer1[i] !='\0')
    {
        segment_size = segment_size*10 + ( buffer1[i] -'0');
        i++;
    }


    //printf("segment size %d\n", segment_size);
    #if DEBUG
    printf("the segment size is %d\n", segment_size);

    #endif
    
        

    

//strcpy( segment_name, buffer1+i);
//printf("segment name is %s\n", segment_name);
i=0;
    // initial acknowledge done segment size is known and file size is also known
   
int num_times =0;
    if( file_size< segment_size)
    {
        num_times =1;
    }
    else
    {
        num_times = ( file_size%segment_size == 0) ? file_size/segment_size : file_size/segment_size+1;
    }
//printf("num times is %d\n", num_times);


strcpy(msg, queue_name);

    strcat(msg," ");
strcat(msg, send);

FILE *fp;
fp = fopen( file_name,"r");
//printf("entered the write segment function\n");



for( i = 0; i<num_times; i++ )
{
    write_segment( fp, segment_name);
    #if DEBUG
    printf("message sent is %s\n", msg);

    #endif
    send_message( msg);
    
    while(1)
    {
        if( receive_message( buffer1) ==1)
        {
            if( strcmp( buffer1, "ACK ") == 0)
            {
    #if DEBUG
                printf("ACK received\n");

    #endif
                break;
                
            }
        }
        
        
    }
}
fclose( fp);




char new_file[64];
strcpy(new_file, "./compresses_");
strcat(new_file, file_name+2);
fp = fopen(new_file, "a" );

strcpy(msg, queue_name);

    strcat(msg," ");

strcat(msg, ack);


while(1)
{
    
    // wait for the message from the server

    receive_message( buffer1);
    if( strcmp( buffer1, "REC ") == 0 )
    {
        copy_data_to_file( fp, segment_name);
    #if DEBUG
        printf("message sent is %s\n", msg);

    #endif
        send_message( msg);
        
    }
    else if( strcmp( buffer1, "DONE ") == 0)
    {
    #if DEBUG
    printf("message received is DONE , compression finished\n");

    #endif

        
        copy_data_to_file( fp, segment_name);
        break;
        
    }



    
}

fclose( fp);

}


// close the queue
if( mq_close( cqt) == -1)
{
    perror(" Client : error in closing the client queue\n ");
    exit(1);
}

if( mq_unlink( queue_name) == -1)
{
    perror(" client: error in removing the client queue\n");
    exit(1);
}

    //printf("client id %d with file size %lld  :completed in",getpid(),  file_size );
//printf("client :completed\n");

//exit(0);

    
}



int main( int argc, char *argv[])
{


    struct timeval tv1, tv2, tv;

    gettimeofday(&tv1, NULL);
        if( strcmp( argv[4], "SYNC") == 0)
    {




        sync_call( argc, argv);
    }
    else
         ASYNC_call(argc, argv);
       gettimeofday( &tv2, NULL);
    timersub( &tv2, &tv1, &tv);
    unsigned long total_time_us = tv.tv_sec*1000000+ tv.tv_usec;
    printf("  %lu us \n", total_time_us);
    
    
    return 0;
    
    
    
    
       

      



        




}