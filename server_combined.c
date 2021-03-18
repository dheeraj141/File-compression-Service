#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "shared_mem.h"
#include <unistd.h>
#include "snappy.h"
#include "map.h"
#include "util.h"

#define DEBUG 0

#define roundup(x,y) (((x) + (y) - 1) & ~((y) - 1))
#define SERVER_QUEUE_NAME "/project2"

#define QUEUE_PERMISSIONS 0660

#define MAX_MESSAGES 10
#define MESSAGE_SIZE 128

#define MAX_BUF_SIZE 138

struct Segment
{
    char name[64];
    int owner;
    int fd;
};

struct Client
{
    char *data;
    char queue_name[64];
    int done, segment_id, offset;
    long long data_size, compressed_length;
    long int id;
};
struct Client  client_data[10];




mqd_t sqt, cqt;


int create_segments( int num_segments, long segment_size, struct Segment *ptr)
{
    char names_buffer[64];

    int i =0;
    
    //creating segments
    
    for( i = 0;  i<num_segments; i++)
    {
    
        strcpy(names_buffer, NAME);
        int fd_shm;
    sprintf(names_buffer+strlen(names_buffer), "%d", i+1);

    #if DEBUG
        printf(" segment name  is %s\n", names_buffer);
    #endif
        if( ( fd_shm = shm_open(names_buffer, O_RDWR|O_CREAT|O_EXCL , 0660 )) == -1)
        {
            printf("shared memory creating error\n");
            return -1;
        }

        //shared memory object created now increase its size

        if( ftruncate( fd_shm, segment_size) == -1)
        {
            printf(" error in increasing the size of the shared_memory\n");
            return -1;

        
        }
        strcpy((ptr+i)->name, names_buffer);
        (ptr+i)->owner = 0;
        (ptr+i)->fd = fd_shm;
    strcpy(names_buffer, "");
    }
    return 1;
    
}

int allocate_segment(  int num_segments, struct Segment *seg_occupy, long int client_id)
{
    // allocation of segments
    int i;
    for( i = 0; i<num_segments; i++)
    {
        if( (seg_occupy+i)->owner == 0)
        {
            // segment number i allocated to the client

            (seg_occupy+i)->owner =client_id;
            return i;
            
        }
            
    }

   

    printf("no free segment\n");
    return -1;
}

void open_client_queue( char *queue_name)
{
    while(1)
    {
        if( ( cqt = mq_open(queue_name, O_WRONLY)) == -1)
        {
            //printf("queue name is %s\n", queue_name);
            perror("error in opening the client queue\n");
            continue;
        }
        else
        {
            //printf("client queue opened\n");
            break;
        }
    }


}
void send_message( char *msg, char *str )
{
    while(1)
    {


        if( mq_send( cqt, msg, strlen( msg)+1, 0) == -1)
        {
             perror("client: not able to send message to server\n");
             continue;
                               
        }
        else
        {
    #if DEBUG
           printf("the response to client  is %s\n", msg);
    #endif
            break;
        }
    }

    //printf("%s\n", str);
                
}

void receive_message(char *msg)
{
    char buffer1[MAX_BUF_SIZE];

    while(1)
    {
        if( mq_receive( sqt, buffer1, MAX_BUF_SIZE,NULL) == -1)
        {
            perror("no message in the queue or some other error in recieving\n");
            continue;
        }
        else
            break;

    }

   
    #if DEBUG
    printf("request received from client is %s \n", buffer1);
    #endif
    strcpy( msg, buffer1);


}
void initialise_client( long int client_id, long long file_size, struct Segment *ptr, int num_segments, char *queue_name, long seg_size)
{
    
    char buffer2[ MAX_BUF_SIZE];


    char seg[5] ="SEG ";
    int i = 0;
    for( i = 0; i<10; i++)
    {
        if( client_data[i].id ==-1)
            break;
    }


    //printf("the client place is %d\n", i);
    
    client_data[i].id = client_id;
    
    client_data[i].data_size =file_size;
    client_data[i].segment_id = allocate_segment( num_segments, ptr, client_id);
    client_data[i].done =0;
    client_data[i].offset = 0;
    client_data[i].compressed_length = 0;

    if( file_size > seg_size)
    client_data[i].data =(char*)malloc( sizeof( char)*file_size);
    
    open_client_queue( queue_name  );

    strcpy(buffer2, seg);



    
    strcat( buffer2, (ptr+client_data[i].segment_id)->name);
    strcat(buffer2, " ");
    sprintf(buffer2 +strlen(buffer2), "%ld", seg_size);
    
    //printf("segment name sent to client is %s\n", buffer2);
    
    send_message(buffer2, "segment name sent to the client");
    
    
    
}





int compress_data(struct Segment *ptr, int offset, long segment_size)
{



    //printf("entered the compress function\n");
    int failed = 0;
    struct snappy_env env;
    snappy_init_env(&env);

    int fd_shm;
     // shared memory
    if( ( fd_shm = shm_open((ptr+offset)->name, O_RDWR , 0660 )) == -1)
    {
        printf("server side :error opening the shared memory\n");
    }



    size_t ps = sysconf(_SC_PAGE_SIZE);

    size_t size =  roundup(segment_size, ps);

    char *map = mmap(NULL, size, PROT_READ|PROT_WRITE,MAP_SHARED, fd_shm, 0);

    size_t outlen;

    int err;
    char *out = xmalloc(snappy_max_compressed_length(size));
    char *buf2 = xmalloc(size);

    err = snappy_compress(&env, map, size, out, &outlen);
    if (err) {
        failed = 1;
        printf("compression of segment failed: %d\n",err);
        goto next;
    }
    
    //printf("compressed the file and compressed file is  \n");

    //printf("%s\n", out);
    
    err = snappy_uncompress(out, outlen, buf2);

    //printf("uncompressing file\n");

    //printf("%s\n", buf2);

    if (err) {
        failed = 1;
        printf("uncompression of segment failed: %d\n", err);
        goto next;
    }
    if (memcmp(buf2, map, size)) {
        int o = compare(buf2, map, size);
        if (o >= 0) {
            failed = 1;
            printf("final comparision of segment failed at %d of %lu\n", o, (unsigned long)size);
        }
    }
    //printf("compression successful , copying the data to the user\n");
         
    memset( map, 0, size);


    memcpy( map, out, outlen);
    //char buf[64];
    //send_message("DONE ", "done compression");

    //sprintf(buf, "%lu",outlen );
    //send_message( buf, "send the compressed length");





    next:
        unmap_file(map, size);
        free(out);
        free(buf2);
        close(fd_shm);

    
    return failed;
        
}

int compress_data1(int index)
{



    //printf("entered the compress1 function\n");
    int failed = 0;
    struct snappy_env env;

    snappy_init_env(&env);
    
    
    size_t outlen;
    
    long long size = client_data[index].data_size;
    char *map = client_data[index].data;

    int err;
    char *out = xmalloc(snappy_max_compressed_length(size));
    char *buf2 = xmalloc(size);

    err = snappy_compress(&env, map, size, out, &outlen);
    if (err) {
        failed = 1;
        printf("compression of segment failed: %d\n",err);
        goto next;
    }
    
    //printf("compressed the file and compressed file is  \n");

    //printf("%s\n", out);
    
    err = snappy_uncompress(out, outlen, buf2);

    //printf("uncompressing file\n");

    //printf("%s\n", buf2);

    if (err) {
        failed = 1;
        printf("uncompression of segment failed: %d\n", err);
        goto next;
    }
    if (memcmp(buf2, map, size)) {
        int o = compare(buf2, map, size);
        if (o >= 0) {
            failed = 1;
            printf("final comparision of segment failed at %d of %lu\n", o, (unsigned long)size);
        }
    }
    //printf("compression successful , copying the data to the user\n");
    client_data[index].compressed_length =(long long)outlen;
    
    // not neccessary performance can be improved
    memset( map, 0, size);


    memcpy( map, out, outlen);
   




    next:
        //unmap_file(map, size);
        free(out);
        free(buf2);
        //close(fd_shm);

    
    return failed;
        
}

void print_data(int size, int offset, struct Segment *ptr, long segment_size)
{
    int fd_shm;
     // shared memory
    if( ( fd_shm = shm_open((ptr+offset)->name, O_RDWR , 0660 )) == -1)
    {
        printf("server side :error opening the shared memory\n");
    }
    char *data;
    if(( data =(char*)mmap( 0, segment_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd_shm, 0)) == MAP_FAILED)
    {
        printf("server: error in mmap\n");
    }
    int i ;
    for(  i =0; i<size; i++)
        printf("%c", data[i]);
    printf("\n");

    munmap( data, segment_size);

    close(fd_shm);
}


int  get_client( long int client_id)
{

    //printf("entered the get client function\n");
    
    int i ;
    for(  i = 0; i<10; i++)
    {
        if( client_data[i].id == client_id)
        {
            return i;
        }
    }
    
    return -1;
}

void copy_data(int index, struct Segment *ptr, int dir, long int segment_size)
{
    int fd_shm;
    char buffer1[MAX_BUF_SIZE];

    if( dir == 0)
    strcpy(buffer1,"ACK ");
    else
    strcpy(buffer1, "REC ");
    
    int offset = client_data[index].segment_id;
     // shared memory
    if( ( fd_shm = shm_open((ptr+offset)->name, O_RDWR , 0660 )) == -1)
    {
        printf("server side :error opening the shared memory\n");
    }
    
    
    char *map = mmap(NULL, segment_size, PROT_READ|PROT_WRITE,MAP_SHARED, fd_shm, 0);
    
    if( dir == 0)
    {
        long long file_size = client_data[index].data_size;
        long long current_size = client_data[index].offset;
         size_t len = 0;
        if( (file_size -current_size) > segment_size)
            len = segment_size;
        else
            len = (file_size -current_size);


            memcpy( client_data[index].data+client_data[index].offset, map, len);
            send_message(buffer1, "ACk signal sent to client \n");
    }
    else
    {
        long long file_size = client_data[index].compressed_length;
        long long current_size = client_data[index].offset;
         size_t len = 0;
        if( (file_size -current_size) > segment_size)
            len = segment_size;
        else
            len = (file_size -current_size);
        memset( map, '\0', segment_size);

            memcpy( map, client_data[index].data+client_data[index].offset, len);
            send_message(buffer1, "REC signal sent to client \n");
        }
    client_data[index].offset+=segment_size;
    
    munmap( map, segment_size);
    close(fd_shm);
    
}

void cleanup_client( int client_id, int segment_size, struct Segment *ptr)
{
    int i ;
    for( i = 0;i<10; i++)
    {
        if( client_data[i].id == client_id)
        {
            client_data[i].id =-1;
            if( client_data[i].data_size > segment_size)
                free( client_data[i].data);

            client_data[i].data_size = 0;
            int offset = client_data[i].segment_id;
            (ptr+offset)->owner = 0;
            client_data[i].segment_id =-1;
            client_data[i].compressed_length = 0;
            client_data[i].offset = 0;
            strcpy(client_data[i].queue_name, "");
            
            
            break;
        }
    }
}
int client_present( int client_id)
{
    int i ;
    for( i = 0; i<10; i++)
    {
        if( client_data[i].id == client_id)
            return 1;
    }
    return -1;
}

    
void process_message( char *msg, int size, char *queue_name, struct Segment *ptr, int num_segments, long segment_size)
{
    
    char msg_type[10];


    int temp_id = 0, i = 0, index = 0;
    while( msg[i] !=' ')
    {
        queue_name[i] = msg[i];
        if(msg[i]>='0' && msg[i]<='9')
        {
        
            temp_id = temp_id*10+(msg[i]-'0');
        }
        i++;
    }
    queue_name[i] ='\0';
    
    i++;

   
    while( msg[i]!= ' ')
    {
        msg_type[index] = msg[i];
        i++; index++;
        
    }
    msg_type[index] = '\0';
    #if DEBUG
    printf("the msg   type is %s\n" , msg_type);
    #endif

    
    
    if(strcmp( msg_type,"INIT") == 0)
    {
        // new client is there
    #if DEBUG
        printf("printing the queue name %s\n", queue_name );
    #endif
        i++;
        long long  temp = 0;


        while( msg[i]!= '\0' )
        {
            temp = temp*10 + msg[i] -'0';
            i++;
        }
    #if DEBUG
        printf("the file size is %lld\n", temp );
    #endif

        
        initialise_client( temp_id, temp, ptr, num_segments, queue_name, segment_size);
        
    }
    else if( strcmp( msg_type, "SEND") == 0)
    {
    //printf("entered the send type\n");
        int place = get_client( temp_id);
        if( client_data[place].data_size < segment_size)
        {
            compress_data(ptr, client_data[place].segment_id, segment_size);
        client_data[place].done =1;
            send_message("ACK ", " take the data");
            send_message("REC ", " take the data");

            
        }
           
        else
        {
            copy_data( place, ptr, 0, segment_size);
            
            if( client_data[place].offset>=client_data[place].data_size)
            {
                compress_data1(place);
                client_data[place].offset = 0;
                copy_data( place,ptr,  1,segment_size);
                
            if( client_data[place].offset >=client_data[place].compressed_length)
                {
            client_data[place].done=1;

            
            }
               

                
            }
                
   
        }
        
    }
    else if( strcmp(msg_type, "ACK") == 0)
    {
        int place = get_client(temp_id );
        if( client_data[place].done ==1)
    {
            send_message("DONE ", "done compression");
        cleanup_client( temp_id, segment_size, ptr);
    }
    else
    {
     
            copy_data(place, ptr, 1, segment_size);
        
            if( client_data[place].offset >=client_data[place].compressed_length)
                {
            client_data[place].done=1;

            
            }
    }
            
            
    }
    
       
    

    



}



int parse_args(int argc, char* argv[], int *num_segments, long  *segment_size)
{

    if( argc <5)
        return -1;
    *num_segments = (int)strtol( argv[2], NULL, 10);

    printf("number of segmments is %d\n", *num_segments);


    *segment_size = strtol( argv[4],NULL, 10 );
    
    printf("segment size is %ld\n", *segment_size);
    return 1;
}










int main( int argc, char *argv[])
{


    // variable declaration

    int num_segments = 0;
    long segment_size;
    struct Segment *segment_info;
    
    
    char buffer1[MAX_BUF_SIZE], queue_name[MAX_BUF_SIZE];

// default values
int i;
for( i =0; i<10; i++)
{
    client_data[i].id = -1;
}

    // taking arguments
    if( parse_args(argc, argv, &num_segments, &segment_size) == -1)
    {
        printf("insufficient number of arguments\n");
        return 0;
    }
    segment_info = (struct Segment*) malloc( num_segments*sizeof(struct Segment));
    create_segments( num_segments, segment_size, segment_info);

    

    printf("hello from server\n");

    // server queue attributes

    struct mq_attr attributes;


    attributes.mq_flags = 0;
    attributes.mq_maxmsg = MAX_MESSAGES;
    attributes.mq_msgsize = MESSAGE_SIZE;
    attributes.mq_curmsgs = 0;

    // opening the queuue

    if( (sqt = mq_open( SERVER_QUEUE_NAME, O_RDONLY| O_CREAT, QUEUE_PERMISSIONS, &attributes)) == -1)
    {
        perror(" server queue can't be created\n");
        exit(1);
    }
    
    while(1)
    {
        receive_message( buffer1);
        
        //printf("message recieved is %s\n", buffer1);
        
        // process the message

        process_message( buffer1, strlen( buffer1),queue_name, segment_info, num_segments, segment_size );
        
        
    }
    

    return EXIT_SUCCESS;















}
