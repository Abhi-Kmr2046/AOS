/*
	Roll No: 18CS10069
	Name: Siba Smarak Panigrahi
	CS60038 : Assignment 2 
	Header LKM For PriorityQueue
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>

#define DEVICE "cs60038_a2_18CS10069"
#define INF 100000000 /* 32 bit integers */
#define current get_current()
/* ioctl commands */
#define PB2_SET_CAPACITY _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_INT _IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_PRIO _IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x34, int32_t*)
#define PB2_GET_MIN _IOR(0x10, 0x35, int32_t*)
#define PB2_GET_MAX _IOR(0x10, 0x36, int32_t*)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siba Smarak Panigrahi (18CS10069)");


/* define spinlock to be used while updating an entry in hashtable */
/* in this program hashtable is representad as a linkedlist of hashtable struct (defined below)*/
static DEFINE_SPINLOCK(pq_spin_lock); 

struct obj_info{
	int32_t pq_size;// current number of elementsin pq
	int32_t capacity;// maximum capacity of the pq
};

struct Elem {
    int32_t prio;
    int32_t val;
};
typedef struct Elem Elem;

struct PriorityQueue{
	Elem *arr; /* pq */
	int32_t count; /* total existing elements in pq */
	int32_t capacity; /* total capacity of pq (N) */
	Elem last_inserted;
};
typedef struct PriorityQueue PriorityQueue;

struct hashtable{
	int key; /* process ID */
	PriorityQueue* global_pq; /* corresponding pq */
	struct hashtable* next; 
};
struct hashtable *process_pq_htable, *htable_elem;

/* methods to interact with hashtable */
static struct hashtable* get_htable_elem(int key);
static void add_to_htable(struct hashtable*);
static void free_process_pq_htable(void);
static void remove_process(int key);
static void print_all_processes(void);

/* PriorityQueue methods */
static PriorityQueue *create_pq(int32_t capacity);
static PriorityQueue* destroy_pq(PriorityQueue* pq);
// static int32_t insert(PriorityQueue *pq, int32_t val, int32_t prio);
static int32_t insert(PriorityQueue *pq, Elem Key);

static int32_t remove(PriorityQueue *pq, int32_t lr);

static long dev_ioctl(struct file *, unsigned int, unsigned long);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
// static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
// static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int open_processes = 0;

static struct proc_ops file_ops =
{
	.proc_open = dev_open,
	.proc_release = dev_release,
	.proc_ioctl = dev_ioctl,
};