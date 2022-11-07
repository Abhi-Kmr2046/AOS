
#include "a2_18CS10069.h"

/* Return a process with a specific pic */
static struct hashtable* get_htable_elem(int key)
{
	struct hashtable* temp = process_pq_htable->next;
	while (temp != NULL){
		if (temp->key == key){
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

/* Add a new process with pid to the linked list */
static void add_to_htable(struct hashtable* htable_elem)
{
	htable_elem->next = process_pq_htable->next;
	process_pq_htable->next = htable_elem;
}

/* Destroy hashtable of processes */
static void free_process_pq_htable(void)
{
	struct hashtable *temp, *temp2;
	temp = process_pq_htable->next;
	process_pq_htable->next = NULL;
	while (temp != NULL){
		temp2 = temp;
		printk(KERN_INFO DEVICE ": (free_process_pq_htable) Free key = %d", temp->key);
		temp = temp->next;
		kfree(temp2);
	}
	kfree(process_pq_htable);
}

/* Deletes a process from hashtable of processes */
static void remove_process(int key)
{
	struct hashtable *prev, *temp;
	prev = temp = process_pq_htable;
	temp = temp->next;

	while (temp != NULL){
		if (temp->key == key){
			prev->next = temp->next;
			temp->global_pq = destroy_pq(temp->global_pq);
			temp->next = NULL;
			printk(KERN_INFO DEVICE ": (remove_process) (PID %d) Free key = %d", current->pid, temp->key);
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}

/* Prints all the process pid */
static void print_all_processes(void)
{
	struct hashtable *temp;
	temp = process_pq_htable->next;
	printk(KERN_INFO DEVICE ": (print_all_processes) Total %d processes", open_processes);
	while (temp != NULL){
		printk(KERN_INFO DEVICE ": (print_all_processes) PID = %d", temp->key);
		temp = temp->next;
	}
}

/* Create PriorityQueue */
static PriorityQueue *create_pq(int32_t capacity)
{
	PriorityQueue *pq = (PriorityQueue *) kmalloc(sizeof(PriorityQueue), GFP_KERNEL);

	if (pq == NULL){
		printk(KERN_ALERT DEVICE ": (create_pq) (PID %d) Memory Error in allocating pq for the process", current->pid);
		return NULL;
	}
	pq->count = 0;
	pq->capacity = capacity;
	pq->arr = (Elem *) kmalloc_array(capacity, sizeof(Elem), GFP_KERNEL); 

	if (pq->arr == NULL){
		printk(KERN_ALERT DEVICE ": (create_pq) (PID %d) Memory Error while allocating pq->arr", current->pid);
		return NULL;
	}
	return pq;
}

/* Destroy PriorityQueue */
static PriorityQueue* destroy_pq(PriorityQueue* pq)
{
	if (pq == NULL)
		/* sanity check to check the existence of pq */
		return NULL; 
	printk(KERN_INFO DEVICE ": (destroy_pq) (PID %d) %ld bytes freed.\n", current->pid, sizeof(pq->arr));
	kfree_const(pq->arr); /* free pq->arr */
	kfree_const(pq); /* free pq */
	return NULL;
}


/* Heapify while inserting */
static void heapify_bottom_top(PriorityQueue *h, int32_t index) {
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



// /* Insert a number into pq */
// static int32_t insert(PriorityQueue *pq, int32_t val, int32_t prio)
// {
// 	if (pq->count < pq->capacity){
// 		/* if number is even; insert at the end */
// 		if (lr == 1){
// 			pq->arr[pq->count] = key;
// 		}
// 		else{
// 			/* if number is odd; insert at the front */
// 			int32_t i;
// 			for(i = pq->count; i > 0; i--){
// 				pq->arr[i] = pq->arr[i-1];
// 			}
// 			pq->arr[0] = key;
// 		}
// 		pq->count++;
// 	}
// 	else{
// 		/* number of elements in pq exceeded its capacity */
// 		return -EACCES;  
// 	}
// 	return 0;
// }
/* Insert a number into heap */
static int32_t insert(PriorityQueue *h, Elem key) {
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



/* Extract the first element of a pq */
static int32_t remove(PriorityQueue *pq, int32_t lr)
{
	int32_t pop, i;
	if (pq->count == 0){
		/* if pq is empty; return -INF */
		return -INF;
	}
	if (lr == 0){
		/* extract from left end */
		pop = pq->arr[0].val; /* read from the front */
		pq->count--; /* reduce the number of elements in pq */
		
		/* shift pq by one place to left */
		for(i = 0; i < pq->count; i++){
			pq->arr[i] = pq->arr[i+1];
		}
	}
	else if(lr == 1){
		/* extract from right end */
		pop = pq->arr[pq->count-1].val; /* read from the end */
		pq->count--; /* reduce the number of elements in pq */
	}
	return pop;
}

/* handle ioctl commands for device */
static long dev_ioctl(struct file *file, unsigned int command, unsigned long arg) 
{
	struct hashtable *htable_elem;
	struct obj_info deque_info;
	int32_t pq_initialized = 0;
	int32_t num;
	int32_t ret;
	int32_t pq_size;
	int32_t pq_max;

	static int32_t nval;
	static int32_t has_nval;


	switch (command) {
	case PB2_SET_CAPACITY:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}

		ret = copy_from_user(&pq_size, (int *)arg, sizeof(int32_t));
		if (ret)
			return -EINVAL;
	
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) PriorityQueue size received: %d", current->pid, pq_size);

		/* check pq size */
		if (pq_size <= 0 || pq_size > 100){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) PriorityQueue size must be an integer in [0,100]", current->pid);
			return -EINVAL;
		}

		htable_elem->global_pq = destroy_pq(htable_elem->global_pq);	/* reset the pq */
		htable_elem->global_pq = create_pq(pq_size);	/* allocate space for new pq */

		break;

	case PB2_INSERT_PRIO:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_PRIO) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		pq_initialized = (htable_elem->global_pq) ? 1 : 0;
		
		/* cannot read from non-initialized pq */
		if (pq_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_PRIO) (PID %d) pq not initialized", current->pid);
			return -EACCES;
		}

		/* obtain the number to be written in left of pq; simultaneously checked for invalid value */
		ret = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(ret)
			return -EINVAL;

		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_INSERT_PRIO) (PID %d) Writing %d to pq\n", current->pid, num);

		/* if pq is initialized and number has been obtained */
		Elem  new_elem;
		new_elem.prio = ret;
		new_elem.val = nval;
		ret = insert(htable_elem->global_pq, new_elem); /* insert into pq */
		has_nval = 0;

		if (ret < 0){ 
			/* pq filled to capacity */ 
			return -EACCES;
		}

		break;

	case PB2_INSERT_INT:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_INT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		pq_initialized = (htable_elem->global_pq) ? 1 : 0;
		
		/* cannot read from non-initialized pq */
		if (pq_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_INT) (PID %d) pq not initialized", current->pid);
			return -EACCES;
		}

		/* obtain the number to be written in right of pq; simultaneously checked for invalid value */
		ret = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(ret)
			return -EINVAL;

		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_INSERT_INT) (PID %d) Writing %d to pq\n", current->pid, num);

		/* if pq is initialized and number has been obtained */
		if(has_nval==0) {
			has_nval = 1;
			nval = ret;
		}
		// ret = insert(htable_elem->global_pq, num, 1); /* insert into pq */
		// if (ret < 0){ 
		// 	/* pq filled to capacity */ 
		// 	return -EACCES;
		// }

		break;

	case PB2_GET_INFO:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_INT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		pq_initialized = (htable_elem->global_pq) ? 1 : 0;
		
		/* cannot read from non-initialized pq */
		if (pq_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_INT) (PID %d) pq not initialized", current->pid);
			return -EACCES;
		}

		/* create obj_info struct and return to user process */
		deque_info.pq_size = htable_elem->global_pq->count;
		deque_info.capacity = htable_elem->global_pq->capacity;

		ret = copy_to_user((struct obj_info *)arg, &deque_info, sizeof(struct obj_info));
		if (ret != 0)
			return -EACCES;
		break;

	case PB2_GET_MIN:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_EXTRACT_LEFT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		pq_initialized = (htable_elem->global_pq) ? 1 : 0;
		
		/* cannot read from non-initialized pq */
		if (pq_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_EXTRACT_LEFT) (PID %d) pq not initialized", current->pid);
			return -EACCES;
		}
		
		/* return error if pq is empty */
		if(htable_elem->global_pq->count == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MIN) (PID %d) pq is empty", current->pid);
			return -EACCES;
		}

		/* obtain leftmost element to send to userspace */
		// pq_max = remove(htable_elem->global_pq, 0);
		pq_max = htable_elem->global_pq->arr[0].val;

		ret = copy_to_user((int32_t*)arg, (int32_t*)&pq_max, sizeof(int32_t));

		if (ret != 0){   
			/* unable to send data to user process */ 
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MIN) (PID %d) pq_max is %d; failed to send to user process", current->pid, pq_max);
			return -EACCES;      
		}
		
		/* obtained the first elem of pq */
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_GET_MIN) (PID %d) Sending data of %ld bytes with value %d to the user process", current->pid, sizeof(pq_max), pq_max);
		break;
	
	case PB2_GET_MAX:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MAX) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		pq_initialized = (htable_elem->global_pq) ? 1 : 0;
		
		/* cannot read from non-initialized pq */
		if (pq_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MAX) (PID %d) pq not initialized", current->pid);
			return -EACCES;
		}
		
		/* return error if pq is empty */
		if(htable_elem->global_pq->count == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MAX) (PID %d) pq is empty", current->pid);
			return -EACCES;
		}

		/* obtain max element to send to userspace */
		// pq_max = remove(htable_elem->global_pq, 1);
		pq_max = htable_elem->global_pq->arr[0].val;

		ret = copy_to_user((int32_t*)arg, (int32_t*)&pq_max, sizeof(int32_t));

		if (ret != 0){   
			/* unable to send data to user process */ 
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_GET_MAX) (PID %d) pq_max is %d; failed to send to user process", current->pid, pq_max);
			return -EACCES;      
		}
		
		/* obtained the first elem of pq */
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_GET_MAX) (PID %d) Sending data of %ld bytes with value %d to the user process", current->pid, sizeof(pq_max), pq_max);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}


static int dev_open(struct inode *inodep, struct file *filep)
{
	struct hashtable *htable_elem;

	/* a process cannot open the file more than once */
	if (get_htable_elem(current->pid) != NULL){
		/* successful to find the process; process already has opened the file */
		printk(KERN_ALERT DEVICE ": (dev_open) (PID %d) Process tried to open the file twice\n", current->pid);
		return -EACCES;
	}

	/* new process; create a corresponding htable_elem in process_pq_htable */ 
	htable_elem = kmalloc(sizeof(struct hashtable), GFP_KERNEL);
	*htable_elem = (struct hashtable){current->pid, NULL, NULL};

	/* obtain the lock before adding the current process to process_pq_htable */
	spin_lock(&pq_spin_lock);
	printk(DEVICE ": (dev_open) (PID %d) Adding %d to hashtable", current->pid, htable_elem->key);
	add_to_htable(htable_elem); /* add the process into hashtable */

	open_processes++;
	printk(KERN_INFO DEVICE ": (dev_open) (PID %d) Device has been opened by %d processes", current->pid, open_processes);
	print_all_processes();
	spin_unlock(&pq_spin_lock);
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	/*
		obtain the spin_lock 
		ensure no other process accesses the process linked list when current process is releasing the device
	*/
	spin_lock(&pq_spin_lock);
	remove_process(current->pid);	/* remove the process from linked list */
	open_processes--;
	printk(KERN_INFO DEVICE ": (dev_release) (PID %d) Device successfully closed\n", current->pid);
	print_all_processes();
	spin_unlock(&pq_spin_lock); /* release the spin_lock */
	return 0;
}

static int LKM_pq_init(void)
{
	struct proc_dir_entry *htable_elem = proc_create(DEVICE, 0, NULL, &file_ops); /* create device (LKM) */
	if (!htable_elem)
		return -ENOENT;
	process_pq_htable = kmalloc(sizeof(struct hashtable), GFP_KERNEL); /* allocate memory to hashtable for process ID (key) -> process pq*/
	if(process_pq_htable == NULL){
		printk(KERN_ALERT DEVICE ": (LKM_pq_init) Cannot allocate memory to process -> pq hash table.");
		return -ENOMEM;
	}
	*process_pq_htable = (struct hashtable){ -1, NULL, NULL};
	printk(KERN_ALERT DEVICE ": (LKM_pq_init) Init PriorityQueue LKM\n");
	spin_lock_init(&pq_spin_lock); /* initialize spin_lock */
	return 0;
}

static void LKM_pq_exit(void)
{
	free_process_pq_htable(); /* free the hash table memory */
	remove_proc_entry(DEVICE, NULL); /* remove the device (LKM) */

	printk(KERN_ALERT DEVICE ": Exiting PriorityQueue LKM\n");
}

module_init(LKM_pq_init);
module_exit(LKM_pq_exit);