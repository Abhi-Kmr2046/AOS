#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define DEVICE_NAME "partb_19CS10004"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define RESET "\x1B[0m"

int main(int argc, char *argv[]) {
    char procfile[] = "/proc/";
    strcat(procfile, DEVICE_NAME);

    int pid;
    pid = fork();

    int32_t tmp;
    int32_t val[5], prio[5], exprio[5];
    int32_t array[5];

    val[0] = 9;
    val[1] = 3;
    val[2] = 6;
    val[3] = 4;
    val[4] = 2;

    prio[0] = 1;
    prio[1] = 7;
    prio[2] = 4;
    prio[3] = 6;
    prio[4] = 8;

    array[0] = 9;
    array[1] = 6;
    array[2] = 4;
    array[3] = 3;
    array[4] = 2;

    exprio[0] = 1;
    exprio[1] = 4;
    exprio[2] = 6;
    exprio[3] = 7;
    exprio[4] = 8;

    int fd = open(procfile, O_RDWR);

    if (fd < 0) {
        printf("PID %d :: ", pid);
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    // Initialize min heap =============================================
    printf("PID %d ==== Initializing Min Heap ====\n", pid);
    char buf[2];
    // min heap
    buf[0] = 0xFF;
    // size of the heap
    buf[1] = 20;

    int result;
    result = write(fd, buf, 2);
    if (result < 0) {
        printf("PID %d :: ", pid);
        perror("Write failed");
        close(fd);
        return 0;
    }
    printf("PID %d Written %d bytes\n", pid, result);

    // Test  Priority Queue ====================================================
    printf("PID %d ======= Test Min Heap =========\n", pid);

    // // Insert --------------------------------------------------------
    for (int i = 0; i < 5; i++) {
        printf("PID %d Inserting %d\n", pid, val[i]);
        char arr[8];
        memcpy(arr, &prio[i], sizeof(int32_t));
        memcpy(arr + 4, &val[i], sizeof(int32_t));

        result = write(fd, arr, 8);
        if (result < 0) {
            printf("PID %d :: error is %d\n", pid, result);
            perror("ERROR! Write failed\n");
            close(fd);
            return 0;
        }
        printf("PID %d Written %d bytes\n", pid, result);
    }

    for (int i = 0; i < 5; i++) {
        printf("PID %d Extracting..\n", pid);
        char arr[8];
        result = read(fd, arr, sizeof(arr));
        if (result < 0) {
            printf("PID %d :: \n", pid);
            perror(RED "ERROR! Read failed\n" RESET);
            close(fd);
            return 0;
        }
        int priority, num;
        memcpy(&priority, arr, sizeof(priority));
        memcpy(&num, arr + 4, sizeof(num));

        printf("PID %d Extracted: value %d, priority %d\n", pid, num, priority);
        if (num == array[i] && priority == exprio[i]) {
            printf(GRN "PID %d Results Matched\n" RESET, pid);
        } else {
            printf(RED
                   "PID %d ERROR! Results Do Not Match. Expected %d, Found "
                   "value %d prio %d\n" RESET,
                   pid, (int)array[i], num, priority);
        }
    }

    close(fd);

    return 0;
}