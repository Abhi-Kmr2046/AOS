#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "partb_19CS10004"
#define INF (0xffffffff)
#define current get_current()

MODULE_LICENSE("GPL");
static DEFINE_MUTEX(heap_mutex);

struct Elem {
    int32_t prio;
    int32_t val;
};
typedef struct Elem Elem;

bool equal(struct Elem e1, struct Elem e2) {
    return (e1.prio == e2.prio && e1.val == e2.val);
}

struct Heap {
    Elem *arr;
    int32_t count;
    int32_t capacity;
    Elem last_inserted;
};
typedef struct Heap Heap;

struct h_struct {
    int32_t key;
    Heap *global_heap;
    struct h_struct *next;
};
struct h_struct *htable, *entry;

/* Function Prototypes */
/* Heap methods */
static Heap *CreateHeap(int32_t capacity);
static Heap *DestroyHeap(Heap *heap);
static int32_t insert(Heap *h, Elem key);
static void heapify_bottom_top(Heap *h, int32_t index);
static void heapify_top_bottom(Heap *h, int32_t parent_node);
Elem PopMin(Heap *h);

/* Concurrency Control Methods */
static struct h_struct *get_entry_from_key(int32_t key);
static void key_add(struct h_struct *);
static void DestroyHashTable(void);
static void key_del(int32_t key);
static void print_key(void);

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int numberOpens = 0;
static char buffer[256] = {0};
static int buffer_len = 0;
static int args_set = 0;
static int retval = -1;

static int32_t num;
static int32_t prio;

static int32_t ret;
static int32_t heap_size;
static Elem topnode;

static struct proc_ops file_ops = {.proc_open = dev_open,
                                   .proc_release = dev_release,
                                   .proc_read = dev_read,
                                   .proc_write = dev_write};

/* Add a new process with pid to the linked list */
static void key_add(struct h_struct *entry) {
    entry->next = htable->next;
    htable->next = entry;
}

/* Return a process with a specific pid */
static struct h_struct *get_entry_from_key(int32_t key) {
    struct h_struct *temp = htable->next;
    while (temp != NULL) {
        if (temp->key == key) {
            return temp;
        }
        // if(equal(temp->key, key)) {
        //     return temp;
        // }
        temp = temp->next;
    }
    return NULL;
}

/* Deletes a process entry */
static void key_del(int32_t key) {
    struct h_struct *prev, *temp;
    prev = temp = htable;
    temp = temp->next;

    while (temp != NULL) {
        if (temp->key == key) {
            // if(equal(temp->key, key)){
            prev->next = temp->next;
            temp->global_heap = DestroyHeap(temp->global_heap);
            temp->next = NULL;
            printk("PID %d <key_del> Kfree key = %d", current->pid, temp->key);
            kfree(temp);
            break;
        }
        prev = temp;
        temp = temp->next;
    }
}

/* Prints all the process pid */
static void print_key(void) {
    struct h_struct *temp;
    temp = htable->next;
    while (temp != NULL) {
        printk("<print_key> key = %d", temp->key);
        temp = temp->next;
    }
}

/* Destroy Linked List of porcesses */
static void DestroyHashTable(void) {
    struct h_struct *temp, *temp2;
    temp = htable->next;
    htable->next = NULL;
    while (temp != NULL) {
        temp2 = temp;
        printk("<DestroyHashTable> Kfree key = %d", temp->key);
        temp = temp->next;
        kfree(temp2);
    }
    kfree(htable);
}

/* Create Heap */
static Heap *CreateHeap(int32_t capacity) {
    Heap *h = (Heap *)kmalloc(sizeof(Heap), GFP_KERNEL);

    // check if memory allocation is fails
    if (h == NULL) {
        printk(KERN_ALERT DEVICE_NAME
               ": PID %d Memory Error in allocating heap!",
               current->pid);
        return NULL;
    }
    h->count = 0;
    h->capacity = capacity;
    h->arr = (Elem *)kmalloc_array(capacity, sizeof(Elem),
                                   GFP_KERNEL);  // size in bytes

    // check if allocation succeed
    if (h->arr == NULL) {
        printk(KERN_ALERT DEVICE_NAME
               ": PID %d Memory Error while allocating heap->arr!",
               current->pid);
        return NULL;
    }
    return h;
}

/* Destroy Heap */
static Heap *DestroyHeap(Heap *heap) {
    if (heap == NULL) return NULL;  // heap is not allocated
    printk(KERN_INFO DEVICE_NAME
           ": PID %d, %ld bytes of heap->arr Space freed.\n",
           current->pid, sizeof(heap->arr));
    kfree_const(heap->arr);
    kfree_const(heap);
    return NULL;
}

/* Insert a number into heap */
static int32_t insert(Heap *h, Elem key) {
    if (h->count < h->capacity) {
        h->arr[h->count] = key;
        heapify_bottom_top(h, h->count);
        h->count++;
        h->last_inserted = key;
    } else {
        return -EACCES;  // Number of elements exceeded the capacity
    }
    return 0;
}

/* Heapify while inserting */
static void heapify_bottom_top(Heap *h, int32_t index) {
    Elem temp;
    int32_t parent_node = (index - 1) / 2;

    // MIN Heap
    if (h->arr[parent_node].prio > h->arr[index].prio) {
        // swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        heapify_bottom_top(h, parent_node);
    }
}

/* Heapify while deleting */
static void heapify_top_bottom(Heap *h, int32_t parent_node) {
    int32_t left = parent_node * 2 + 1;
    int32_t right = parent_node * 2 + 2;
    Elem temp;
    int32_t max;

    if (left >= h->count || left < 0) left = -1;
    if (right >= h->count || right < 0) right = -1;

    // Max heap
    if (left != -1 && h->arr[left].prio < h->arr[parent_node].prio)
        max = left;
    else
        max = parent_node;
    if (right != -1 && h->arr[right].prio < h->arr[max].prio) max = right;

    if (max != parent_node) {
        temp = h->arr[max];
        h->arr[max] = h->arr[parent_node];
        h->arr[parent_node] = temp;

        // recursive  call
        heapify_top_bottom(h, max);
    }
}

/* Extract the top node of a heap */
Elem PopMin(Heap *h) {
    Elem pop;
    if (h->count == 0) {
        pop.val = -INF;
        pop.prio = 0;
        return pop;
    }
    // replace first node by last and delete last
    pop = h->arr[0];
    h->arr[0] = h->arr[h->count - 1];
    h->count--;
    heapify_top_bottom(h, 0);
    return pop;
}

static ssize_t dev_write(struct file *file, const char *buf, size_t count,
                         loff_t *pos) {
    if (!buf || !count) return -EINVAL;
    if (copy_from_user(buffer, buf, count < 256 ? count : 256)) return -ENOBUFS;

    // Get the process corresponing heap
    entry = get_entry_from_key(current->pid);
    if (entry == NULL) {
        printk(KERN_ALERT DEVICE_NAME
               ": PID %d RAISED ERROR in dev_write entry is non-existent",
               current->pid);
        return -EACCES;
    }
    // Args Set = 1 if the heap is initialized(not NULL)
    args_set = (entry->global_heap) ? 1 : 0;
    buffer_len = count < 256 ? count : 256;

    // If heap is initialized
    if (args_set) {
        char arr[8];
        Elem e;
        if (count != 8) {
            printk(KERN_ALERT DEVICE_NAME
                   ": PID %d WRONG DATA SENT. %d bytes %ld count",
                   current->pid, buffer_len, count);
            return -EINVAL;
        }
        // Check for unexpected type
        memset(arr, 0, sizeof(arr));
        memcpy(arr, buf, 8 * sizeof(char));
        memcpy(&prio, arr, sizeof(num));
        memcpy(&num, arr + 4, sizeof(prio));
        printk(DEVICE_NAME ": PID %d writing %d to heap with prio %d\n",
               current->pid, num, prio);
        e.prio = prio;
        e.val = num;

        ret = insert(entry->global_heap, e);
        if (ret < 0) {  // Heap is filled to capacity
            return -EACCES;
        }
        return sizeof(num);
    }

    // Any other call before the heap has been initialized
    if (buffer_len != 2) {
        return -EACCES;
    }

    // Initlize Heap
    // heap_type = buf[0];
    heap_size = buf[1];
    printk(DEVICE_NAME ": PID %d ", current->pid);
    printk(DEVICE_NAME ": PID %d HEAP SIZE: %d", current->pid, heap_size);
    printk(DEVICE_NAME ": PID %d RECIEVED:  %ld bytes", current->pid, count);

    if (heap_size <= 0 || heap_size > 100) {
        printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of heap %d!!\n",
               current->pid, heap_size);
        return -EINVAL;
    }

    entry->global_heap =
        DestroyHeap(entry->global_heap);  // destroy any existing heap before
                                          // creating a new one
    entry->global_heap =
        CreateHeap(heap_size);  // allocating space for new heap

    return buffer_len;
}

static ssize_t dev_read(struct file *file, char *buf, size_t count,
                        loff_t *pos) {
    if (!buf || !count) return -EINVAL;

    // Get the process corresponing heap
    entry = get_entry_from_key(current->pid);
    if (entry == NULL) {
        printk(KERN_ALERT DEVICE_NAME
               "PID %d RAISED ERROR in dev_read entry is non-existent",
               current->pid);
        return -EACCES;
    }
    // Args Set = 1 if the heap is initialized(not NULL)
    args_set = (entry->global_heap) ? 1 : 0;

    // If heap is not initialized
    if (args_set == 0) {
        printk(KERN_ALERT DEVICE_NAME ": PID %d Heap not initialized",
               current->pid);
        return -EACCES;
    }

    // Extract the topmost node of heap
    topnode = PopMin(entry->global_heap);
    printk(DEVICE_NAME ": PID %d asking for %ld bytes\n", current->pid, count);
    char arr[8];
    memset(arr, 0, sizeof(arr));
    memcpy(arr, &(topnode.prio), sizeof(topnode.prio));
    memcpy(arr+4, &(topnode.val), sizeof(topnode.val));
    retval = copy_to_user(buf, arr,
                          sizeof(arr));

    if (retval == 0) {  // success!
        printk(KERN_INFO DEVICE_NAME
               ": PID %d Sent %ld chars with value %d prio %dto the user\n",
               current->pid, sizeof(topnode), topnode.val, topnode.prio);
        return sizeof(topnode);
    } else {
        printk(KERN_INFO DEVICE_NAME
               ": PID %d Failed to send retval : %d, topnode value is %d prio "
               "is %d\n",
               current->pid, retval, topnode.val, topnode.prio);
        return -EACCES;  // Failed -- return a bad address message (i.e. -14)
    }
}

static int dev_open(struct inode *inodep, struct file *filep) {
    // If same process has already opened the file
    if (get_entry_from_key(current->pid) != NULL) {
        printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n",
               current->pid);
        return -EACCES;
    }

    // Create a new entry to the process linked list
    entry = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
    *entry = (struct h_struct){current->pid, NULL, NULL};
    if (!mutex_trylock(&heap_mutex)) {
        printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_open>",
               current->pid);
        return -EBUSY;
    }
    printk(DEVICE_NAME ": PID %d !!!! Adding %d to HashTable\n", current->pid,
           entry->key);
    key_add(entry);

    numberOpens++;
    printk(KERN_INFO DEVICE_NAME ": PID %d Device has been opened %d time(s)\n",
           current->pid, numberOpens);
    print_key();
    mutex_unlock(&heap_mutex);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    if (!mutex_trylock(&heap_mutex)) {
        printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_release>.",
               current->pid);
        return -EBUSY;
    }
    // Delete the process entry from the process linked list
    key_del(current->pid);
    printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully closed\n",
           current->pid);
    print_key();
    mutex_unlock(&heap_mutex);
    return 0;
}

static int hello_init(void) {
    struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0, NULL, &file_ops);
    if (!entry) return -ENOENT;
    htable = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
    *htable = (struct h_struct){-1, NULL, NULL};
    printk(KERN_ALERT DEVICE_NAME ": Hello world\n");
    mutex_init(&heap_mutex);
    return 0;
}

static void hello_exit(void) {
    mutex_destroy(&heap_mutex);
    DestroyHashTable();
    remove_proc_entry(DEVICE_NAME, NULL);

    printk(KERN_ALERT DEVICE_NAME ": Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);