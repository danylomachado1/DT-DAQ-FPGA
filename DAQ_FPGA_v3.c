////////////////////////////////////////////////////////////////////////////////
//DAQ_FPGA_v3
//Purpose:we must ensure that the two data streams are copied independently to
//        the EB, where a consumer mechanism must also be implemented. So that
//        later these streams, in parallel, are copied to a common output.
//Author:Franz Machado
////////////////////////////////////////////////////////////////////////////////
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
////////////////////////////////////////////////////////////////////////////////
#define NUM_THREADS 6    // 5 insted 3
#define BUFFER_SIZE 750 //1000000
////////////////////////////////////////////////////////////////////////////////
int data_write[BUFFER_SIZE];
int data_write_stream_1[BUFFER_SIZE];  // change
int data_write_stream_2[BUFFER_SIZE];  // change

int buffer_data[BUFFER_SIZE];
int buffer_data_stream_1[BUFFER_SIZE];  // change
int buffer_data_stream_2[BUFFER_SIZE];  // change
int empty_sacc_1[BUFFER_SIZE];
int empty_stream_1[BUFFER_SIZE];  //data must be copied here
int empty_sacc_2[BUFFER_SIZE];
int empty_stream_2[BUFFER_SIZE];  //and here, then it must be copied to joint_data_empty
int joint_data_empty[2*BUFFER_SIZE];   //something like this maybe empty_stream_1 and ..._2

int header_stream_1 = 4026531840;
int header_stream_2 = 4043309056;

int* write_p_1;          //to stream 1, write pointers
int* read_p_1;           //read pointers

int* write_p_2;          //to stream 2, write pointers
int* read_p_2;           //read pointers

int* write_temp_1;      //to stream 1
int* write_temp_2;      //to stream 2

bool write_d_1 = true;  //to stream 1
bool write_d_2 = true;  //to stream 2

pthread_mutex_t lock;
int r_sacc_1=0;
int r_sacc_2=0;
int r_stream_1=0;   /////
int r_stream_2=0;   /////
bool done_1 = false;    /////
bool done_2 = false;    /////
bool stream_1_done = true;   //////
bool stream_2_done = true;   //////
void write_func_stream_1(void *argument);    //function to introduce data to stream 1
void write_func_stream_2(void *argument);    //function to introduce data to stream 2

void read_func_stream_1(void *argument);     //function to read the stream 1
void read_func_stream_2(void *argument);     //function to read the stream 2

int fd;
int fd1;
int fd2;

int z_1 = 0;
int z_2 = 0;
int xd = 3;
////////////////////////////////////////////////////////////////////////////////
typedef struct thdata_type
{
int thread_no;
int* data_storage;
} thdata;
////////////////////////////////////////////////////////////////////////////////
void array_input(int array1[], int num2);
/////////////////////////////////////////////////////////////////////////////7//
int main(int argc, char *argv[])
{

srand(time(NULL));

pthread_mutex_init(&lock, NULL);
fd = open("joint.out",O_RDWR|O_CREAT, S_IRWXU);  /// description of mysync.out; what does mysync.out contains?
//If you want read/write/execute permission for the current user/owner use: S_IRWXU
if (fd<0) {perror("Open"); exit(2);} //use this

fd1 = open("stream1.out",O_RDWR|O_CREAT, S_IRWXU);  /// description of mysync.out; what does mysync.out contains?
//If you want read/write/execute permission for the current user/owner use: S_IRWXU
if (fd1<0) {perror("Open"); exit(2);}

fd2 = open("stream2.out",O_RDWR|O_CREAT, S_IRWXU);  /// description of mysync.out; what does mysync.out contains?
//If you want read/write/execute permission for the current user/owner use: S_IRWXU
if (fd2<0) {perror("Open"); exit(2);}
/////////////////////////////////////////

/////////////////////////////////////////


array_input(data_write_stream_1, 1);
array_input(data_write_stream_2, 2);

pthread_t threads[NUM_THREADS];
thdata stream_1 = {0, &data_write_stream_1[0]};  //change
thdata stream_2 = {1, &data_write_stream_2[0]};  //change
thdata stream_sacc_1 = {2, &empty_sacc_1[0]};
thdata saved_data_1 = {3, &empty_stream_1[0]};
thdata stream_sacc_2 = {4, &empty_sacc_1[0]};
thdata saved_data_2 = {5, &empty_stream_2[0]};


int thread_args[6];

for (int r=0; r<NUM_THREADS; ++r){
thread_args[r] = r;
printf("In main: creating thread %d\n", r);
}

//create all threads
pthread_create(&threads[0], NULL, (void *) &write_func_stream_1,(void *) &stream_1);
pthread_create(&threads[1], NULL, (void *) &write_func_stream_2,(void *) &stream_2);
pthread_create(&threads[2], NULL, (void *) &read_func_stream_1, (void *) &stream_sacc_1);
pthread_create(&threads[3], NULL, (void *) &read_func_stream_1, (void *) &saved_data_1);
pthread_create(&threads[4], NULL, (void *) &read_func_stream_2, (void *) &stream_sacc_2);
pthread_create(&threads[5], NULL, (void *) &read_func_stream_2, (void *) &saved_data_2);

// wait for all threads to complete
pthread_join(threads[0], NULL);
pthread_join(threads[1], NULL);
pthread_join(threads[2], NULL);
pthread_join(threads[3], NULL);
pthread_join(threads[4], NULL);
pthread_join(threads[5], NULL);
//pthread_join(threads[4], NULL);
pthread_mutex_destroy(&lock);
exit(EXIT_SUCCESS);

}

////////////////////////////////////////////////////////////////////////////////

void write_func_stream_1(void *argument){


int* temp_r_s1;   //temporal read pointer
int* temp_w_s1;   //temporal write pointer
int i_write_s1 = 0;
int diff_s1;
int* first_el_s1 = &buffer_data_stream_1[0];
int* last_el_s1 = first_el_s1 + BUFFER_SIZE;

pthread_mutex_lock(&lock);
write_p_1 = &buffer_data_stream_1[0];   //write_p es puntero
read_p_1 = &buffer_data_stream_1[0];
pthread_mutex_unlock(&lock);

int buff_size = sizeof(buffer_data)/4;  //sizeof(buffer_data) = 3000; int size = 4
thdata *data_s1 = (thdata*) argument;

////////////////////////////////////
while(1){
//while(z_1 <= xd){
pthread_mutex_lock(&lock);
temp_r_s1 = read_p_1;
temp_w_s1 = write_p_1;
printf("Write (stream 1): temp_r_s1 %x temp_w_s1 %x\n", temp_r_s1, temp_w_s1);
pthread_mutex_unlock(&lock);

//Por aca debos implementar algo que restinga a la diferencia que al inicio es 750/4 = 187

diff_s1 = 0;
/////////////////
if(temp_r_s1 < temp_w_s1 || i_write_s1<=1){

diff_s1 = (int)(((last_el_s1 - temp_w_s1) + (temp_r_s1 - first_el_s1)));
printf("diff_s1_1 %d  \n", diff_s1);

}else if(temp_r_s1 > temp_w_s1){

diff_s1 = (int)((temp_r_s1 - temp_w_s1));
printf("diff_s1_2 %d \n", diff_s1 );

}else if(temp_r_s1 == temp_w_s1 && done_1){

diff_s1 = (int)(((last_el_s1 - temp_w_s1) + (temp_r_s1 - first_el_s1)));
printf("diff_s1_3 %d \n", diff_s1);

}
///////////////////
int slots_to_be_written = diff_s1/4;
printf("Writing %d slots\n",slots_to_be_written); //check how much space there is on the main buffer (data_write_stream_)

pthread_mutex_lock(&lock);


//here is the deal
int temp_length_s1 = 0;
//f0000000 = 4026531840
for(int slots = 0; slots < slots_to_be_written; ++slots){
  buffer_data_stream_1[i_write_s1%BUFFER_SIZE] = data_s1->data_storage[i_write_s1%BUFFER_SIZE];
  if(buffer_data_stream_1[i_write_s1%BUFFER_SIZE]>0xf0000000){
    printf("%d - %d\n", buffer_data_stream_1[i_write_s1%BUFFER_SIZE], buffer_data_stream_1[i_write_s1%BUFFER_SIZE] - 4026531840);
    int m_temp = buffer_data_stream_1[i_write_s1%BUFFER_SIZE] - header_stream_1;
    temp_length_s1 = temp_length_s1 + m_temp + 1;
  }

  //printf("%d - %d\n", buffer_data_stream_1[i_write_s1%BUFFER_SIZE], buffer_data_stream_1[i_write_s1%BUFFER_SIZE] - 4026531840);
  if(temp_length_s1 >= slots_to_be_written){
    break;
  }else{
    printf("i_write_s1 (stream 1) is %d buffer_data_stream_1[i_write_s1%BUFFER_SIZE] 0x%08X\n", i_write_s1,buffer_data_stream_1[i_write_s1%BUFFER_SIZE]);
    //printf("i_write_s1 (stream 1) is %d buffer_data_stream_1[i_write_s1%BUFFER_SIZE] %d\n", i_write_s1,buffer_data_stream_1[i_write_s1%BUFFER_SIZE]);
    i_write_s1++;   //this could be importatn to read complete packets, yo have to star from here.
  }

}

write_p_1 = &buffer_data_stream_1[(i_write_s1)%(buff_size)];  //write pointers are updated here
write_d_1 = false;
printf("i_write_s1%buff_size %d\n",i_write_s1%(buff_size));
pthread_mutex_unlock(&lock);
usleep(400);

//z_1 = z_1 + 1;
}
////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////

void write_func_stream_2(void *argument)
{

int* temp_r_s2;
int* temp_w_s2;
int i_write_s2 = 0;
int diff_s2;
int* first_el_s2 = &buffer_data_stream_2[0];
int* last_el_s2 = first_el_s2 + BUFFER_SIZE;

pthread_mutex_lock(&lock);
write_p_2 = &buffer_data_stream_2[0];   //write_p es puntero
read_p_2 = &buffer_data_stream_2[0];
pthread_mutex_unlock(&lock);

int buff_size = sizeof(buffer_data)/4;  //sizeof(buffer_data) = 3000; int size = 4
thdata *data_s2 = (thdata*) argument;

////////////////////////////////////
while(1){
//while(z_2 <= xd){
pthread_mutex_lock(&lock);
temp_r_s2 = read_p_2;
temp_w_s2 = write_p_2;
printf("Write (stream 2): temp_r_s2 %x temp_w_s2 %x\n", temp_r_s2, temp_w_s2);
pthread_mutex_unlock(&lock);

diff_s2 = 0;
/////////////////
if(temp_r_s2 < temp_w_s2 || i_write_s2<=1){

diff_s2 = (int)(((last_el_s2 - temp_w_s2) + (temp_r_s2 - first_el_s2)));
printf("diff_s2_1 %d  \n", diff_s2);

}else if(temp_r_s2 > temp_w_s2){

diff_s2 = (int)((temp_r_s2 - temp_w_s2));
printf("diff_s2_2 %d \n", diff_s2 );

}else if(temp_r_s2 == temp_w_s2 && done_2){

diff_s2 = (int)(((last_el_s2 - temp_w_s2) + (temp_r_s2 - first_el_s2)));
printf("diff_s2_3 %d \n", diff_s2);

}
///////////////////
int slots_to_be_written = diff_s2/4;
printf("Writing %d slots\n",slots_to_be_written);

pthread_mutex_lock(&lock);

//here is the deal
int temp_length_s2 = 0;
//f0000000 = 4026531840
for(int slots = 0; slots < slots_to_be_written; ++slots){
  buffer_data_stream_2[i_write_s2%BUFFER_SIZE] = data_s2->data_storage[i_write_s2%BUFFER_SIZE];
  if(buffer_data_stream_2[i_write_s2%BUFFER_SIZE]>0xf1000000){
    printf("%d - %d\n", buffer_data_stream_2[i_write_s2%BUFFER_SIZE], buffer_data_stream_2[i_write_s2%BUFFER_SIZE] - header_stream_2);
    int m_temp = buffer_data_stream_2[i_write_s2%BUFFER_SIZE] - header_stream_2;
    temp_length_s2 = temp_length_s2 + m_temp + 1;
  }

  if(temp_length_s2 >= slots_to_be_written){
    break;
  }else{
    printf("i_write_s2 (stream 2) is %d buffer_data_stream_2[i_write_s2%BUFFER_SIZE] 0x%08X\n", i_write_s2,buffer_data_stream_2[i_write_s2%BUFFER_SIZE]);
    i_write_s2++;   //this could be importatn to read complete packets, yo have to star from here.
  }

}

write_p_2 = &buffer_data_stream_2[(i_write_s2)%(buff_size)]; //write pointers are updated here
write_d_2 = false;
printf("i_write_s2%buff_size %d\n",i_write_s2%(buff_size));
pthread_mutex_unlock(&lock);
usleep(400);
//z_2 = z_2+1;
}
////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////

void read_func_stream_1(void *argument)
{
int* temp_r1_s1;
int* temp_w1_s1;
int diff1_s1;
int* first_el1_s1 = &buffer_data_stream_1[0];
int* last_el1_s1 = first_el1_s1 + BUFFER_SIZE;

while(1){
//while(z_1 <= xd +1){
thdata *temp_s1 = (thdata*) argument;
int th_no_s1 = temp_s1->thread_no;

pthread_mutex_lock(&lock);
temp_r1_s1 = read_p_1;
temp_w1_s1 = write_p_1;
printf("Thr %d Read:  temp_r %x temp_w %x\n", th_no_s1,temp_r1_s1, temp_w1_s1);
printf("copying data from stream 1 (consumer)\n");
pthread_mutex_unlock(&lock);

if(th_no_s1 == 2 && stream_1_done && !write_d_1){
//safe copy has to be implemented.

pthread_mutex_lock(&lock);
write_temp_1 = temp_w1_s1; //updates the write pointer for stream_sacc
pthread_mutex_unlock(&lock);

diff1_s1=0;

if(temp_r1_s1 < write_temp_1){

diff1_s1 = (int)((write_temp_1 - temp_r1_s1));
printf("Thr %d (stream 1) 1st diff1 %d \n", th_no_s1,diff1_s1);

}else if(temp_r1_s1 >= write_temp_1){

diff1_s1 = (int)((last_el1_s1 - temp_r1_s1) + (write_temp_1 - first_el1_s1));
printf("Thr %d (stream 1) 2nd diff1 %d %d %d %d\n", th_no_s1,diff1_s1,last_el1_s1, first_el1_s1,sizeof(buffer_data));

}

int slots_to_be_read = (int) diff1_s1;

pthread_mutex_lock(&lock);
printf ("r_sacc_1 %d\n",r_sacc_1); //r_sacc :  not sure about it

for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
temp_s1->data_storage[r_sacc_1%BUFFER_SIZE] = buffer_data_stream_1[r_sacc_1%BUFFER_SIZE];
r_sacc_1++;
}

stream_1_done = false;
done_1 = false;
pthread_mutex_unlock(&lock);

printf("Thr %d Done reading sacc_1 \n",th_no_s1);
printf("r_sacc_1 is %d\n", r_sacc_1%BUFFER_SIZE);

}else if(th_no_s1 == 3 && !stream_1_done){  // in this thread the join of buffers is performed

diff1_s1=0;
if(temp_r1_s1 < write_temp_1){

diff1_s1 = (int)(write_temp_1 - temp_r1_s1);
printf("Thr %d (stream 1) 1st diff1 %d \n", th_no_s1,diff1_s1);

}else if(temp_r1_s1 >= write_temp_1){

diff1_s1 = (int)((last_el1_s1 - temp_r1_s1) + (write_temp_1 - first_el1_s1));
printf("Thr %d (stream 1) 2nd diff1_s1 %d \n", th_no_s1,diff1_s1);

}

int slots_to_be_read = (int) diff1_s1;
printf("Thr %d (stream 1) Reading by st_1 %d slots \n", th_no_s1,slots_to_be_read);  //??

pthread_mutex_lock(&lock);
int *wr_pnt = &(temp_s1->data_storage[r_stream_1%BUFFER_SIZE]);

for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
  temp_s1->data_storage[r_stream_1%BUFFER_SIZE] = buffer_data_stream_1[r_stream_1%BUFFER_SIZE];
  joint_data_empty[(r_stream_1+r_stream_2)%(2*BUFFER_SIZE)] = buffer_data_stream_1[r_stream_1%BUFFER_SIZE]; //use a similar function to write fd1
  int ret_1 = write(fd, &joint_data_empty[(r_stream_1+r_stream_2)%(2*BUFFER_SIZE)],4);
  if (slots1==0) printf("temp_storage %d buffer_data %d\n",temp_s1->data_storage[r_stream_1%BUFFER_SIZE], buffer_data_stream_1[r_stream_1%BUFFER_SIZE]);
  r_stream_1++;
}

printf("Thr %d Done reading by st_1 \n",th_no_s1);
read_p_1 = write_temp_1;
printf("Thr %d read_p after assignment %d \n", th_no_s1,read_p_1);
done_1 = true;
stream_1_done = true;
write_d_1 = true;
int end = (int) (&temp_s1->data_storage[BUFFER_SIZE] - wr_pnt)*4;

if (slots_to_be_read*4 <= end) end = slots_to_be_read*4;

printf ("(stream 1): start end len %d %x %x\n", &temp_s1->data_storage[BUFFER_SIZE], wr_pnt, end);
int ret = write(fd1, wr_pnt,end);

if (slots_to_be_read*4 != end) ret = write(fd1, &temp_s1->data_storage[0],slots_to_be_read*4 - end);

if (ret<0) {perror("Write");}

pthread_mutex_unlock(&lock);

printf("r_stream_1 is %d, %d\n", r_stream_1%BUFFER_SIZE, r_stream_1);
}

usleep(400);
}

}


////////////////////////////////////////////////////////////////////////////////

void read_func_stream_2(void *argument)
{
int* temp_r1_s2;
int* temp_w1_s2;
int diff1_s2;
int* first_el1_s2 = &buffer_data_stream_2[0];
int* last_el1_s2 = first_el1_s2 + BUFFER_SIZE;

while(1){
//while(z_2 <= xd +1){
thdata *temp_s2 = (thdata*) argument;
int th_no_s2 = temp_s2->thread_no;

pthread_mutex_lock(&lock);
temp_r1_s2 = read_p_2;
temp_w1_s2 = write_p_2;
printf("Thr %d Read:  temp_r %x temp_w %x\n", th_no_s2,temp_r1_s2, temp_w1_s2);
printf("copying data from stream 2 (consumer)");
pthread_mutex_unlock(&lock);


if(th_no_s2 == 4 && stream_2_done && !write_d_2){


pthread_mutex_lock(&lock);
write_temp_2 = temp_w1_s2;
pthread_mutex_unlock(&lock);

diff1_s2=0;

if(temp_r1_s2 < write_temp_2){

diff1_s2 = (int)((write_temp_2 - temp_r1_s2));
printf("Thr %d (stream 2) 1st diff1 %d \n", th_no_s2,diff1_s2);

}else if(temp_r1_s2 >= write_temp_2){

diff1_s2 = (int)((last_el1_s2 - temp_r1_s2) + (write_temp_2 - first_el1_s2));
printf("Thr %d (stream 2) 2nd diff1 %d %d %d %d\n", th_no_s2,diff1_s2,last_el1_s2, first_el1_s2,sizeof(buffer_data));

}

int slots_to_be_read = (int) diff1_s2;

pthread_mutex_lock(&lock);
printf ("r_sacc_2 %d\n",r_sacc_2); //r_sacc : stream accelerator

for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
temp_s2->data_storage[r_sacc_2%BUFFER_SIZE] = buffer_data_stream_2[r_sacc_2%BUFFER_SIZE];
r_sacc_2++;
}

stream_2_done = false;
done_2 = false;
pthread_mutex_unlock(&lock);

printf("Thr %d Done reading sacc_2 \n",th_no_s2);
printf("r_sacc_2 is %d\n", r_sacc_2%BUFFER_SIZE);

}else if(th_no_s2 == 5 && !stream_2_done){

diff1_s2=0;
if(temp_r1_s2 < write_temp_2){

diff1_s2 = (int)(write_temp_2 - temp_r1_s2);
printf("Thr %d (stream 2) 1st diff1 %d \n", th_no_s2,diff1_s2);

}else if(temp_r1_s2 >= write_temp_2){

diff1_s2 = (int)((last_el1_s2 - temp_r1_s2) + (write_temp_2 - first_el1_s2));
printf("Thr %d (stream 2) 2nd diff1 %d \n", th_no_s2,diff1_s2);

}

int slots_to_be_read = (int) diff1_s2;
printf("Thr %d Reading by st_2 %d slots \n", th_no_s2,slots_to_be_read);  //??

pthread_mutex_lock(&lock);
int *wr_pnt = &(temp_s2->data_storage[r_stream_2%BUFFER_SIZE]);

for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
  temp_s2->data_storage[r_stream_2%BUFFER_SIZE] = buffer_data_stream_2[r_stream_2%BUFFER_SIZE];
  joint_data_empty[(r_stream_1+r_stream_2)%(2*BUFFER_SIZE)] = buffer_data_stream_2[r_stream_2%BUFFER_SIZE];  //we store the data of both streams here
  int ret_1 = write(fd, &joint_data_empty[(r_stream_1+r_stream_2)%(2*BUFFER_SIZE)],4);
  if (slots1==0) printf("temp_storage %d buffer_data %d\n",temp_s2->data_storage[r_stream_2%BUFFER_SIZE], buffer_data_stream_2[r_stream_2%BUFFER_SIZE]);
  r_stream_2++;
}

printf("Thr %d Done reading by st_2 \n",th_no_s2);
read_p_2 = write_temp_2;
printf("Thr %d read_p after assignment %d \n", th_no_s2,read_p_2);
done_2 = true;
stream_2_done = true;
write_d_2 = true;
int end = (int) (&temp_s2->data_storage[BUFFER_SIZE] - wr_pnt)*4;

if (slots_to_be_read*4 <= end) end = slots_to_be_read*4;

printf ("(stream 2): start end len %d %d %d\n", &temp_s2->data_storage[BUFFER_SIZE], wr_pnt, end);
int ret = write(fd2, wr_pnt,end);

if (slots_to_be_read*4 != end) ret = write(fd2, &temp_s2->data_storage[0],slots_to_be_read*4 - end);

if (ret<0) {perror("Write");}

pthread_mutex_unlock(&lock);

printf("r_stream_2 is %d, %d\n", r_stream_2%BUFFER_SIZE, r_stream_2);
}

usleep(400);
}

}

//Functions extra
void array_input(int array1[], int num_a) {
    int a;
    if(num_a == 1 ){
        a = 4026531840;
    }

    if(num_a == 2 ){
        a = 4043309056;
    }

    int len; //= rand() % 31;

    int i = 0;
    int temp_len = 0;
    float w = sizeof(data_write)/4;
    //printf("w = %f\n",w);
    while(i < w){

        len = 1+rand() % 30;

        if(i + len + 1 >= w){
            len = w - i;
        }

        array1[i] = a + len;
        i = i + 1;


        for(int x = 1 + temp_len; x < len + temp_len + 1; ++x){
            array1[i] = x;
            i = i + 1;
        }

        temp_len = temp_len + len + 0;
    }

    //for(int x = 0; x<=w;++x){
    //    printf("data_write_stream_%d[%d] = 0x%08X / %d\n",num_a,x, array1[x], array1[x]);
    //}

    return 0;
}
