////////////////////////////////////////////////////////////////////////////////
//Base code
//Purpose:simulate the data read and write synchronization mechanism between the
//        emulated PCIe40 card, FPGA accelerator, Buffer, and the EB.
//Author:Yara Al-Quorashy  
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
////////////////////////////////////////////////////////////////////////////////
#define NUM_THREADS 3
#define BUFFER_SIZE 750
////////////////////////////////////////////////////////////////////////////////
int data_write[BUFFER_SIZE];
int buffer_data[BUFFER_SIZE];
int empty[BUFFER_SIZE];
int empty1[BUFFER_SIZE];
int* write_p;
int* read_p;
int* write_temp;
bool write_d = true;
int* read_temp;
pthread_mutex_t lock;
int r_fpga =0;
bool done = false;
bool fpga_done = true;
int r_eb = 0;
void write_func(void *argument);
void read_func(void *argument);
int fd;
////////////////////////////////////////////////////////////////////////////////
typedef struct thdata_type
{
int thread_no;
int* data_storage;
} thdata;
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

pthread_mutex_init(&lock, NULL);
fd = open("mysync.out",O_RDWR|O_CREAT, S_IRWXU);  /// description of mysync.out; what does mysync.out contains?
//If you want read/write/execute permission for the current user/owner use: S_IRWXU
if (fd<0) {perror("Open"); exit(2);}

for(int x = 0; x < sizeof(data_write)/4; ++x){
data_write[x] = x;
}

pthread_t threads[NUM_THREADS];
thdata pci = {0, &data_write[0]}; //ask
thdata fpga = {1, &empty[0]};
thdata eb = {2, &empty1[0]};
int thread_args[3];

for (int r=0; r<NUM_THREADS; ++r){
thread_args[r] = r;
printf("In main: creating thread %d\n", r);
}

//create all threads
pthread_create(&threads[0], NULL, (void *) &write_func,(void *) &pci);
pthread_create(&threads[1], NULL, (void *) &read_func, (void *) &fpga);
pthread_create(&threads[2], NULL, (void *) &read_func, (void *) &eb);

// wait for all threads to complete
pthread_join(threads[0], NULL);
pthread_join(threads[1], NULL);
pthread_join(threads[2], NULL);
pthread_mutex_destroy(&lock);
exit(EXIT_SUCCESS);

}

////////////////////////////////////////////////////////////////////////////////

void write_func(void *argument)
{

int* temp_r;
int* temp_w;
int i_write = 0;
int diff;
int* first_el = &buffer_data[0];
int* last_el = first_el + BUFFER_SIZE;

pthread_mutex_lock(&lock);
write_p = &buffer_data[0];   //write_p es puntero
read_p = &buffer_data[0];
pthread_mutex_unlock(&lock);

int buff_size = sizeof(buffer_data)/4;  //sizeof(buffer_data) = 3000; int size = 4
thdata *data = (thdata*) argument;

////////////////////////////////////
while(1){

pthread_mutex_lock(&lock);
temp_r = read_p;
temp_w = write_p;
printf("Write: temp_r %d temp_w %d\n", temp_r, temp_w);
pthread_mutex_unlock(&lock);

diff = 0;
/////////////////
if(temp_r < temp_w || i_write<=1){

diff = (int)(((last_el - temp_w) + (temp_r - first_el)));
printf("diff_1 %d  \n", diff);

}else if(temp_r > temp_w){

diff = (int)((temp_r - temp_w));
printf("diff_2 %d \n", diff );

}else if(temp_r == temp_w && done){

diff = (int)(((last_el - temp_w) + (temp_r - first_el)));
printf("diff_3 %d \n", diff);

}
///////////////////
int slots_to_be_written = diff/4;
printf("Writing %d slots\n",slots_to_be_written);
pthread_mutex_lock(&lock);

for(int slots = 0; slots < slots_to_be_written; ++slots){
buffer_data[i_write%BUFFER_SIZE] = data->data_storage[i_write%BUFFER_SIZE];
printf("i_write is %d buffer_data[i_write%BUFFER_SIZE] %d\n", i_write,buffer_data[i_write%BUFFER_SIZE]);
i_write++;
}

write_p = &buffer_data[(i_write)%(buff_size)];
write_d = false;
printf("i_write%buff_size %d\n",i_write%(buff_size));
pthread_mutex_unlock(&lock);
usleep(400);

}
////////////////////////////////////
}


////////////////////////////////////////////////////////////////////////////////

void read_func(void *argument)
{
int* temp_r1;
int* temp_w1;
int diff1;
int* first_el1 = &buffer_data[0];
int* last_el1 = first_el1 + BUFFER_SIZE;

while(1){

thdata *temp = (thdata*) argument;
int th_no = temp->thread_no;

pthread_mutex_lock(&lock);
temp_r1 = read_p;
temp_w1 = write_p;
printf("Thr %d Read:  temp_r %d temp_w %d\n", th_no,temp_r1, temp_w1);
printf("Thr %d done is %d fpga_done is %d\n", th_no,done,fpga_done);
pthread_mutex_unlock(&lock);

if(th_no == 1 && fpga_done && !write_d){//FPGA starts reading

pthread_mutex_lock(&lock);
write_temp = temp_w1;
pthread_mutex_unlock(&lock);

diff1=0;

if(temp_r1 < write_temp){

diff1 = (int)((write_temp - temp_r1));
printf("Thr %d 1st diff1 %d \n", th_no,diff1);

}else if(temp_r1 >= write_temp){

diff1 = (int)((last_el1 - temp_r1) + (write_temp - first_el1));
printf("Thr %d 2nd diff1 %d %d %d %d\n", th_no,diff1,last_el1, first_el1,sizeof(buffer_data));

}

int slots_to_be_read = (int) diff1;

pthread_mutex_lock(&lock);
printf ("r_fpga %d\n",r_fpga);

for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
temp->data_storage[r_fpga%BUFFER_SIZE] = buffer_data[r_fpga%BUFFER_SIZE];
r_fpga++;
}

fpga_done = false;
done = false;
pthread_mutex_unlock(&lock);

printf("Thr %d Done reading FPGA \n",th_no);
printf("r_fpga is %d\n", r_fpga%BUFFER_SIZE);

}else if(th_no == 2 && !fpga_done){ //EB starts reading

diff1=0;
if(temp_r1 < write_temp){

diff1 = (int)(write_temp - temp_r1);
printf("Thr %d 1st diff1 %d \n", th_no,diff1);

}else if(temp_r1 >= write_temp){

diff1 = (int)((last_el1 - temp_r1) + (write_temp - first_el1));
printf("Thr %d 2nd diff1 %d \n", th_no,diff1);

}

int slots_to_be_read = (int) diff1;
printf("Thr %d Reading by EB %d slots \n", th_no,slots_to_be_read);

pthread_mutex_lock(&lock);
int *wr_pnt = &(temp->data_storage[r_eb%BUFFER_SIZE]);
for(int slots1 = 0; slots1 < slots_to_be_read; ++slots1){
temp->data_storage[r_eb%BUFFER_SIZE] = buffer_data[r_eb%BUFFER_SIZE];

if (slots1==0) printf("temp_storage %d buffer_data %d\n",temp->data_storage[r_eb%BUFFER_SIZE], buffer_data[r_eb%BUFFER_SIZE]);
r_eb++;
}

printf("Thr %d Done reading by EB \n",th_no);
read_p = write_temp;
printf("Thr %d read_p after assignment %d \n", th_no,read_p);
done = true;
fpga_done = true;
write_d = true;
int end = (int) (&temp->data_storage[BUFFER_SIZE] - wr_pnt)*4;

if (slots_to_be_read*4 <= end) end = slots_to_be_read*4;

printf ("start end len %d %d %d\n", &temp->data_storage[BUFFER_SIZE], wr_pnt, end);
int ret = write(fd, wr_pnt,end);
int dum = -1;
ret = write(fd, &dum,4);

if (slots_to_be_read*4 != end) ret = write(fd, &temp->data_storage[0],slots_to_be_read*4 - end);

dum = -2;
ret = write(fd, &dum,4);

if (ret<0) {perror("Write");}

pthread_mutex_unlock(&lock);

printf("r_eb is %d\n", r_eb%BUFFER_SIZE);
}
usleep(400);
}

}
