#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <filesys/file.h>

pid_t do_fork(proc_func func, void* aux1, void* aux2)
{
	pid_t pid;
	struct proc_option opt;

	opt.priority = cur_process-> priority;
	pid = proc_create(func, &opt, aux1, aux2);

	return pid;
}

void do_exit(int status)
{
	cur_process->exit_status = status; 	//종료 상태 저장
	proc_free();						//프로세스 자원 해제
	do_sched_on_return();				//인터럽트 종료시 스케줄링
}

pid_t do_wait(int *status)
{
	while(cur_process->child_pid != -1)
		schedule();
	//SSUMUST : schedule 제거.

	int pid = cur_process->child_pid;
	cur_process->child_pid = -1;

	extern struct process procs[];
	procs[pid].state = PROC_UNUSED;

	if(!status)
		*status = procs[pid].exit_status;

	return pid;
}

void do_shutdown(void)
{
	dev_shutdown();
	return;
}

int do_ssuread(void)
{
	return kbd_read_char();
}

int do_open(const char *pathname, int flags)
{
	struct inode *inode;
	struct ssufile **file_cursor = cur_process->file;
	int fd;

	for(fd = 0; fd < NR_FILEDES; fd++)
		if(file_cursor[fd] == NULL) break;

	if (fd == NR_FILEDES)
		return -1;

	if ( (inode = inode_open(pathname)) == NULL)
		return -1;
	
	if (inode->sn_type == SSU_TYPE_DIR)
		return -1;

	fd = file_open(inode,flags,0);
	
	return fd;
}

int do_read(int fd, char *buf, int len)
{
	return generic_read(fd, (void *)buf, len);
}
int do_write(int fd, const char *buf, int len)
{
	return generic_write(fd, (void *)buf, len);
}
/*change : add*/
int do_lseek(int fd, size_t offset, int whence)
{
	struct inode *inode;
	struct ssufile **file_cursor = cur_process->file;
	size_t cur_pos;

	inode = file_cursor[fd]->inode;

	/*when inode type is not file(directory)*/
	if (inode->sn_type == SSU_TYPE_DIR)
		return -1;

	/*change position if option is NULL*/
	if (whence % SEEK_END == 0) {
		cur_pos = file_cursor[fd]->inode->sn_size + offset;
	}
	else if(whence % SEEK_CUR == 0) {
		cur_pos = file_cursor[fd]->pos + offset;
	}
	else if(whence % SEEK_SET == 0) {
		cur_pos = offset;
	}

	/*options*/
	if (whence / C_OPT) {
		/*if position is bigger than file size, set cur_pos to cur_pos % file size*/
		if (cur_pos > file_cursor[fd]->inode->sn_size) {
			cur_pos %= file_cursor[fd]->inode->sn_size;
		}
	}
	else if (whence / RE_OPT) {
		int i;
		char tmp_block[BUFSIZ];

		/*re option exec only if position is less than 0*/
		if ((int)cur_pos < 0) {
			int plus_len = -1 * cur_pos;

			/*copy inode to tmp_block*/
			file_read(inode, 0, tmp_block, file_cursor[fd]->inode->sn_size);
			tmp_block[file_cursor[fd]->inode->sn_size] = NULL;
			/*extend file size*/
			file_cursor[fd]->inode->sn_size = plus_len;
			
			/*write copied tmp_block to inode  after plus_len position*/
			file_write(inode, (size_t)(plus_len), tmp_block, file_cursor[fd]->inode->sn_size);

			/*fill "000..." to inode before plus_len position*/
			for (i = 0; i < plus_len; i++) {
				tmp_block[i] = '0';
			}
			file_write(inode, 0, tmp_block, plus_len);
		}
	}
	else if (whence / E_OPT) {
		int i;
		unsigned int plus_len = cur_pos - file_cursor[fd]->inode->sn_size;
		char tmp_block[BUFSIZ];

		memcpy(tmp_block, 0, BUFSIZ);

		for (i = 0; i < plus_len; i++) {
			tmp_block[i] = '0';
		}

		/*fill "000..." to inode after filesize position*/
		file_write(inode, file_cursor[fd]->inode->sn_size, tmp_block, plus_len);
	}
	else if (whence / A_OPT) {
		int i;
		unsigned int plus_len = offset;
		unsigned int mid_len = file_cursor[fd]->inode->sn_size - file_cursor[fd]->pos;
		char tmp_block[BUFSIZ];

		memcpy(tmp_block, 0, BUFSIZ);

		/*move inode after pos to inode after pos + plus_len*/
		file_read(inode, file_cursor[fd]->pos, tmp_block, mid_len);
		file_write(inode, file_cursor[fd]->pos + plus_len, tmp_block, mid_len);

		/*fill "000..." to inode from plus_len position to cur_pos + plus_len*/
		for (i = 0; i < plus_len; i++) {
			tmp_block[i] = '0';
		}
		file_write(inode, file_cursor[fd]->pos, tmp_block, plus_len);


	}

	/*if position is less than a, set to 0*/
	if ((int)cur_pos < 0) {
		cur_pos = 0;
	}

	file_cursor[fd]->pos = cur_pos;

	return file_cursor[fd]->pos;
}
