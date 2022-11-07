/*** Credits for this test script: Somnath Jena (18CS30047) and Archisman Pathak (18CS30050) ***/
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
using namespace std;

#define PB2_SET_CAPACITY _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_INT _IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_PRIO _IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x34, int32_t*)
#define PB2_GET_MIN _IOR(0x10, 0x35, int32_t*)
#define PB2_GET_MAX _IOR(0x10, 0x36, int32_t*)

 
#define int int32_t
#define PROCNAME "/proc/cs60038_a2_19cs10004"
#define MAX_CAPACITY 100
#define NUM_OPERATIONS 5
#define NUM_FORKS 0

struct obj_info {
    int32_t pq_size;         //current number of elements in deque
    int32_t capacity;           //maximum capacity of the deque
};

int main(){

 
    priority_queue<int> dq;
 
    vector<int> op({5, 0,1,0,1,0,1, 2,3, 4});
    vector<int> input({100, 1,1,2,2,3,3,0,0, 0});
    vector<int> exp({0, 0,0,0,0,0,0,3,3, 0});



    


    pid_t curr_pid = getpid();
    char procname[] = PROCNAME;
 
    int fd = open(procname, O_RDWR);
    if(fd<0)
    {
        printf("PID %d : Error in opening PROC FILE\n", curr_pid);
        return 0;
    }
 
    printf("PID %d : PROC FILE successfully opened!\n", curr_pid);

    for(int i=0; i<op.size(); i++) {
        if(op[i]==5) {
            int inval = input[i];
            int ret = ioctl(fd, PB2_SET_CAPACITY, &inval);
            if(ret < 0)
            {
                printf("PID %d : Failed to (re)initialize with capacity%d!\n", curr_pid, input[i]);
                close(fd);
                return 0;
            }
        } else if(op[i]==0){
            int ret = ioctl(fd, PB2_INSERT_INT, &input[i]);
            if(ret < 0)
            {
                printf("PID %d : Failed to insert number!\n", curr_pid);
                close(fd);
                return 0;
            }
        } else if(op[i]==1){
            int ret = ioctl(fd, PB2_INSERT_PRIO, &input[i]);
            if(ret < 0)
            {
                printf("PID %d : Failed to insert prio!\n", curr_pid);
                close(fd);
                return 0;
            }
        }else if(op[i]==2){
            int val;
            int ret = ioctl(fd, PB2_GET_MAX, &val);
            if(ret < 0)
            {
                printf("PID %d : Failed PB2_GET_MAX\n", curr_pid);
                close(fd);
                return 0;
            }
            printf("MAX VALUE %d\n", val);
        } else if(op[i]==3){
            int val;
            int ret = ioctl(fd, PB2_GET_MIN, &val);
            if(ret < 0)
            {
                printf("PID %d : Failed PB2_GET_MIN\n", curr_pid);
                close(fd);
                return 0;
            }
            printf("MIN VALUE %d\n", val);

        } else if(op[i]==4){
            struct obj_info curr_info;
            int ret = ioctl(fd, PB2_GET_INFO, &curr_info);
            if(ret < 0)
            {
                printf("PID %d : Failed PB2_GET_INFO\n", curr_pid);
                close(fd);
                return 0;
            }
            printf("Curr Info %d %d\n", curr_info.capacity, curr_info.pq_size);
        }
    }

    



    printf("PID %d : All Outputs Matched!\n", curr_pid);
    close(fd);
    return 0;
}
