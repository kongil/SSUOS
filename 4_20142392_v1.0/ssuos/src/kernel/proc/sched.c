#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>
#include <device/console.h>

extern struct list level_que[QUE_LV_MAX];
extern struct list plist;
extern struct list slist;
extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;

struct process *latest;

struct process* get_next_proc(struct list *rlist_target);
void proc_que_levelup(struct process *cur);
void proc_que_leveldown(struct process *cur);
struct process* sched_find_que(void);

int scheduling;

/*
	linux multilevelfeedback queue scheduler
	level 1 que policy is FCFS(First Come, First Served)
	level 2 que policy is RR(Round Robin).
*/

//sched_find_que find the next process from the highest queue that has proccesses.
struct process* sched_find_que(void) {
	int i,j;
	struct process * result = NULL;
	 
	proc_wake();

		/*TODO :check the queue whether it is empty or not  
		 and find each queue for the next process.
		*/
	/*change : get next process in level_que[1] or if level_que[1] is empty, find level_que[2]*/
	while (result == NULL) {
		if (!list_empty(&level_que[1])) {
			result = get_next_proc(&level_que[1]);
			break;
		}
		else if (!list_empty(&level_que[2])){
			result = get_next_proc(&level_que[2]);
			break;
		}
	}

		
	return result;
}

struct process* get_next_proc(struct list *rlist_target) {
	struct list_elem *e;

	for(e = list_begin (rlist_target); e != list_end (rlist_target);
		e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_stat);

		if(p->state == PROC_RUN)
			return p;
	}
	return NULL;
}

void schedule(void)
{
	struct process *cur;
	struct process *next;
	struct process *tmp;
	struct list_elem *ele;
	int i = 0, printed = 0;

	scheduling = 1;	
	cur = cur_process;
	/*TODO : if current process is idle_process(pid 0), schedule() choose the next process (not pid 0).
	when context switching, you can use switch_process().  
	if current process is not idle_process, schedule() choose the idle process(pid 0).
	complete the schedule() code below.
	*/
	if ((cur -> pid) != 0) {
		/*change : pid(not 0) to  pid0*/
		latest = cur;
		scheduling = 0;
		cur_process = idle_process;
		cur_process->time_slice = 0;
		intr_disable();
		switch_process(cur, cur_process);
		intr_enable();
		return;
	}

		switch (latest -> que_level) {
			/*change : add case*/
			case 0:
				break;
			case 1:	//FIFO
				if (latest->time_slice == 40) 	// time INT
				{
					proc_que_leveldown(latest);
				}
												//i/o INT
				break;
			case 2:	//ROUND ROBIN
				if (latest->time_slice >= 80) // time INT
				{
					proc_que_leveldown(latest);
				}
												//i/o INT
				break;
			default:
				break;
		}
		latest = NULL;
		
	proc_wake(); //wake up the processes 

	intr_disable();
	for (ele = list_begin(&plist); ele != list_end(&plist); ele = list_next(ele)) {
		tmp = list_entry (ele, struct process, elem_all);
		if ((tmp -> state == PROC_ZOMBIE) || 
			//(tmp -> state == PROC_BLOCK) || 
			//	(tmp -> state == PROC_STOP) ||
					(tmp -> pid == 0)) 	continue;
			if (!printed) {	
				printk("#=%2d t=%3d u=%3d ", tmp -> pid, tmp -> time_slice, tmp -> time_used);
				printk("q=%3d\n", tmp->que_level);
				printed = 1;			
			}
			else {
				printk(", #=%2d t=%3d u=%3d ", tmp -> pid, tmp -> time_slice, tmp->time_used);
				printk("q=%3d\n", tmp->que_level);
			}
			
	}

	if (printed)
		printk("\n");

	if ((next = sched_find_que()) != NULL) {
		printk("Selected process : %d\n", next -> pid);

		intr_enable();

		/*chagne : pid0 to next process (first -> 1 or if que[1] is empty, search que[2])*/
		scheduling = 0;
		cur_process = next;
		cur_process->time_slice = 0;
		intr_disable();
		switch_process(cur, next);
		intr_enable();

		return;
	}
	return;
}

void proc_que_levelup(struct process *cur)
{
	/*TODO : change the queue lv2 to queue lv1.*/
}

void proc_que_leveldown(struct process *cur)
{
	/*TODO : change the queue lv1 to queue lv2.*/
	if (cur->que_level == 1) {
		cur->que_level = 2;
		
		intr_disable();
		printk("Proc %d change the queue(%d->%d)\n", cur->pid, 1, 2);
		intr_enable();
		list_push_back(&level_que[2], &latest->elem_stat); //insert level_que[1] to level_que[2]'s back
	}
	else if (cur->que_level == 2) {
		list_push_back(&level_que[2], &latest->elem_stat); // insert level_que[2] to level_que[2]'s back
	}
}

