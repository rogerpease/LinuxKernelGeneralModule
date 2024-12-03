#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h> 

/* This is a "hodgepodge" module meant to keep notes on how various things work. */ 

#ifndef TIMER_INTERRUPT
   Define timer interrupt. Use 19 for VM ubuntu, 117 for khadas board. 
#endif

/* Interrupts */ 
#include <linux/interrupt.h>

// dentry_path_raw 
#include <linux/proc_fs.h>

// File ops  
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// Proc entries 
#define GENERALMODULE_PROCESSLIST "generalmodule_processlist"
#define GENERALMODULE_INTCOUNT    "generalmodule_intcount"

// Kernel Options 
static char *option1 = "default";

module_param(option1, charp, 0000);
MODULE_PARM_DESC(str, "Option 1");


//
// Make sure the .proc_open function properly directs v here as I do below. 
//
static int printprocesslist(struct seq_file *m,void *v) {  

    struct task_struct *p;

    for_each_process(p) {
       seq_printf(m,"%s %d \n",p->comm,p->pid);
    }

    /* Presuming this writes to a buffer seq_read will read.  */
    seq_printf(m,"PID %d %s",current->pid,current->comm);

    return 0; 

}
        
static int processlist_open(struct inode *inode, struct file *filp) {
    return single_open(filp, printprocesslist, NULL);
}

static const struct proc_ops processlist_ops = {
    .proc_open      = processlist_open,
    .proc_read      = seq_read,
};

////////////////////////////////////////////////////////////////////////////
//
// Functions to print the number of times an interrupt fires. 
// In a "normal" isr you'd also read a peripheral, to see if there were 
// anything worth fetching. 
// 

typedef struct _my_fancy_interrupt_data_t {
    int interrupt_count; 
    spinlock_t interrupt_lock;
} my_fancy_interrupt_data_t;


static my_fancy_interrupt_data_t * my_fancy_interrupt_data_p;

static int printintcount(struct seq_file *m,void *v) {  

    // Generally a good practice to do only what is absolutely necessary in interrupts. 
    int count; 

    spin_lock  (&my_fancy_interrupt_data_p->interrupt_lock);
    count = my_fancy_interrupt_data_p->interrupt_count;
    spin_unlock(&my_fancy_interrupt_data_p->interrupt_lock);

    seq_printf (m,"Interrupt count %d\n",count);
    return 0; 

}

#define INTCOUNT_BUFLEN (80)

static int intcount_open(struct inode *inode, struct file *filp) {

    char buf[INTCOUNT_BUFLEN]; 

    pr_info("Opened %s",filp->f_path.dentry->d_iname);

    dentry_path_raw(filp->f_path.dentry,buf,INTCOUNT_BUFLEN);

    pr_info("Full path %s", buf);

    return single_open(filp, printintcount, NULL);

}

static const struct proc_ops intcount_ops = {
    .proc_open      = intcount_open,
    .proc_read      = seq_read,
};

irqreturn_t my_int_handler(int i, void * p);

irqreturn_t my_int_handler(int i, void * p)
{
   my_fancy_interrupt_data_t * mfid = (my_fancy_interrupt_data_t *) p; 

   /*
    *  i lists the interrupt number. Multiple interrupts can use the same handler. 
    *
    *  Obviously for more complicated operations you'd need to make sure you always unlocked before leaving. 
    *  That being said interrupts are usually best when they are quick and minimal. 
    *  Get in, do what you need to, and get out as fast as you can. 
    *
    */ 

   spin_lock(&mfid->interrupt_lock);
   mfid->interrupt_count += 1; 
   spin_unlock(&mfid->interrupt_lock);

   return IRQ_HANDLED;
}


static int __init my_module_init(void)
{

     int result; 
     struct proc_dir_entry *file;

     pr_info("Option 1 %s\n", option1);
     pr_info("Change with: insmod GeneralModule option1='something_brilliant'\n");

     /* Create /proc/GENERALMODULE_PROCESSLIST */
     /* When you open to read it, you get a list of processes. */ 
     /* When you open to write it, nothing. */ 
      

     file = proc_create(GENERALMODULE_PROCESSLIST, 0, NULL, &processlist_ops);
     if (file == NULL) { 
         pr_err("Processlist proc allocation failed"); 
         return -ENOMEM; 
     }

   
     if ((my_fancy_interrupt_data_p =  kmalloc(sizeof(my_fancy_interrupt_data_t),GFP_KERNEL)) == NULL) 
     { 
       pr_err("Fancy Interrupt data allocation failed"); 
       goto cleanup1; 
     }

     /* Create /proc/GENERALMODULE_INTCOUNT */
     /* When you open to read it, you get a list of processes. */ 
     /* When you open to write it, nothing. */ 
      
     file = proc_create(GENERALMODULE_INTCOUNT, 0, NULL, &intcount_ops);
     if (file == NULL) { goto cleanup1; }

     memset(my_fancy_interrupt_data_p,0,sizeof(my_fancy_interrupt_data_t));
     

     result = request_irq(TIMER_INTERRUPT, my_int_handler, IRQF_SHARED, 
                 "MY_SHARED_INT", my_fancy_interrupt_data_p);
  
     pr_info("Request IRQ result %d\n",result);
     if (result < 0) 
     { 
        goto cleanup2;
     } 

     spin_lock_init (&(my_fancy_interrupt_data_p->interrupt_lock));

     return 0;

cleanup2:
     pr_info("CLEANUP- Removing intcount");
     remove_proc_entry(GENERALMODULE_INTCOUNT, NULL);
  
cleanup1: 
     pr_info("CLEANUP- Removing processlist");
     remove_proc_entry(GENERALMODULE_PROCESSLIST, NULL);
     return ENOMEM; 

}

static void __exit my_module_exit(void)
{
    remove_proc_entry(GENERALMODULE_PROCESSLIST, NULL);
    remove_proc_entry(GENERALMODULE_INTCOUNT, NULL);
    free_irq(TIMER_INTERRUPT,my_fancy_interrupt_data_p);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roger Pease");
MODULE_DESCRIPTION("General Kernel Module.");


