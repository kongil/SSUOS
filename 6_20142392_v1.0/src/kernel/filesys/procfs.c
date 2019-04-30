#include <filesys/procfs.h>
#include <filesys/vnode.h>
#include <proc/proc.h>
#include <list.h>
#include <string.h>
#include <ssulib.h>

extern struct list p_list;
extern struct process *cur_process;

struct vnode *init_procfs(struct vnode *mnt_vnode)
{
	mnt_vnode->v_op.ls = proc_process_ls;
	mnt_vnode->v_op.mkdir = NULL;
	mnt_vnode->v_op.cd = proc_process_cd;

	return mnt_vnode;
}

int proc_process_ls()
{
	int result = 0;
	struct list_elem *e;

	printk(". .. ");
	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_all);

		printk("%d ", p->pid);
	}
	printk("\n");

	return result;
}

int atoi(char const *c){

	int value = 0;
	int positive = 1;

	if(*c == '\0')
		return 0;

	if(*c == '-')
		positive = -1;

	while(*c) {
		if(*c > '0' && *c < '9')
			value = value * 10 + *c - '0';
		c++;
	}

	return value*positive;
}

int proc_process_cd(char *dirname)
{
	struct vnode *new_vnode;
	struct list_elem *e;

	/*check if dirname is not in p_list*/
	int check = 0;
	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
	    struct process* p = list_entry(e, struct process, elem_all);
		if (p->pid == atoi(dirname)) {
			check++;
		}
	}
	if (check == 0)
		return;

	/*create new vnode*/
	new_vnode = vnode_alloc();
	new_vnode->type = DIR_TYPE;
	new_vnode->v_parent = cur_process->cwd;
	new_vnode->v_op.ls = proc_process_info_ls;
	new_vnode->v_op.cat = proc_process_info_cat;
	new_vnode->v_op.cd = proc_process_info_cd;
	new_vnode->v_op.mkdir = NULL;
	memcpy(new_vnode->v_name, dirname, strlen(dirname)+1);

	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
	    struct process* p = list_entry(e, struct process, elem_all);
		if (p->pid == atoi(dirname)) {
			new_vnode->info = (void *)p;
			break;
		}
	}

	/*create new vnode childlist*/
	list_push_back(&cur_process->cwd->childlist, &new_vnode->elem);

	/*change cur_process->cwd to created vnode*/
	cur_process->cwd = new_vnode;

}

int proc_process_info_ls()
{
	int result = 0;

	printk(". .. ");
	printk("cwd root time stack");

	printk("\n");

	return result;
}

int proc_process_info_cd(char *dirname)
{
	struct vnode *new_vnode;

	/*check if dirname is not in cwd||root*/
	if (strcmp(dirname, "cwd") != 0 && strcmp(dirname, "root") != 0) {
		return 0;
	}

	/*create new vnode*/
	new_vnode = vnode_alloc();
	new_vnode->type = DIR_TYPE;
	new_vnode->v_parent = cur_process->cwd;
	new_vnode->v_op.ls = proc_link_ls;
	new_vnode->v_op.cat = NULL; 
	new_vnode->v_op.cd = proc_process_info_cd;
	new_vnode->v_op.mkdir = NULL;
	memcpy(new_vnode->v_name, dirname, strlen(dirname)+1);
	new_vnode->info = (void *)cur_process->cwd->info;

	/*create new vnode childlist*/
	//list_push_back(&cur_process->cwd->childlist, &new_vnode->elem);

	/*change cur_process->cwd to created vnode*/
	cur_process->cwd = new_vnode;
}

int proc_process_info_cat(char *filename)
{
	struct process *tmp_process = (struct process *)(cur_process->cwd->info);

	/*print if cat stack*/
	if (strcmp(filename, "stack") == 0) {
		printk("stack : %x\n", tmp_process->stack); 
	}
	/*print if cat time*/
	else if (strcmp(filename, "time") == 0) {
		printk("time_used : %lu\n", tmp_process->time_used);
	}
}

int proc_link_ls()
{
	struct list_elem *e;
	int i;

	struct process *tmp;
	tmp = cur_process -> cwd -> info;

	printk(". .. "); 
	
	/*if cur_process's current working directory v_node = cwd*/
	if (strcmp(cur_process->cwd->v_name, "cwd") == 0) {
		for(e = list_begin (&tmp->cwd->v_parent->childlist); e != list_end (&tmp->cwd->v_parent->childlist); e = list_next (e))
	    {
			struct vnode *child;
	        child = list_entry(e, struct vnode, elem);
	        printk("%s ", child->v_name);
	    }

	}
	/*if cur_process's current working directory v_node = root*/
	else if (strcmp(cur_process->cwd->v_name, "root") == 0) {
		for(e = list_begin (&tmp->rootdir->v_parent->childlist); e != list_end (&tmp->rootdir->v_parent->childlist); e = list_next (e))
		{
			struct vnode *child;
	        child = list_entry(e, struct vnode, elem);
	        printk("%s ", child->v_name);
		}
	}

	printk("\n"); 
}
