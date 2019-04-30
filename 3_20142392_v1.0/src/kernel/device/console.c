#include <interrupt.h>
#include <device/console.h>
#include <type.h>
#include <device/kbd.h>
#include <device/io.h>
#include <device/pit.h>
#include <stdarg.h>

#include <proc/proc.h>
char next_line[2]; //"\r\n";

#ifdef SCREEN_SCROLL

char buf_s[SIZE_SCROLL]; 
char *buf_w;	
char *buf_p;	

int sum_y;

bool a_s = TRUE;
#endif

//위 전역변수를 사용하는 코드를 아래 변수를 사용하는 코드로 변경
struct Console console[MAX_CONSOLE_NUM];
extern struct process *cur_process;
struct Console *cur_console;

/*change : variable to console array and initialize all the console array*/
void init_console(void)
{
	Glob_x = 0;
	Glob_y = 2;

	next_line[0] = '\r';
	next_line[1] = '\r';

#ifdef SCREEN_SCROLL
	/*
	buf_w = buf_s;
	buf_p = buf_s;
	a_s = TRUE;
	*/

	sum_y = 0;
	//
	
	int i, j;
	for (i = 0; i < MAX_CONSOLE_NUM; i++) {
		for (j = 0; j < SIZE_SCROLL; j++)
			console[i].buf_s[j] = 0;
		console[i].Glob_x = Glob_x;
		console[i].Glob_y = Glob_y;
		console[i].buf_w = console[i].buf_s;
		console[i].buf_p = console[i].buf_s;
		console[i].sum_y = sum_y;
		console[i].a_s = TRUE;
		console[i].used = FALSE;
	}

	cur_console = get_console();

	cur_process->console = cur_console;
#endif

}

/*change : variable to cur_console->variable*/
void set_cursor(void)
{
	/*
    outb(0x3D4, 0x0F);
    outb(0x3D5, (Glob_y*HSCREEN+Glob_x)&0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (((Glob_y*HSCREEN+Glob_x)>>8)&0xFF));
	*/
    outb(0x3D4, 0x0F);
    outb(0x3D5, (cur_console->Glob_y*HSCREEN+cur_console->Glob_x)&0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (((cur_console->Glob_y*HSCREEN+cur_console->Glob_x)>>8)&0xFF));
}

/*change : variable to cur_process->console->variable*/
void PrintCharToScreen(int x, int y, const char *pString) 
{
	/*
	Glob_x = x;
	Glob_y = y;
	int i = 0;
	while(pString[i] != 0)
	{
		PrintChar(Glob_x++, Glob_y, pString[i++]);
	}
	a_s = TRUE;
	*/
	/*
	cur_console->Glob_x = x;
	cur_console->Glob_y = y;
	int i = 0;
	while(pString[i] != 0)
	{
		PrintChar(cur_console->Glob_x++, cur_console->Glob_y, pString[i++]);
	}
	cur_console->a_s = TRUE;
	*/
	cur_process->console->Glob_x = x;
	cur_process->console->Glob_y = y;
	int i = 0;
	while(pString[i] != 0)
	{
		PrintChar(cur_process->console->Glob_x++, cur_process->console->Glob_y, pString[i++]);
	}
	cur_process->console->a_s = TRUE;
}

/*change : variable to cur_process->console->variable*/
void PrintChar(int x, int y, const char String) 
{
#ifdef SCREEN_SCROLL
	/*
	if (String == '\n') {
		if((y+1) > VSCREEN) {
			scroll();
			y--;
		}
		Glob_x = 0;
		Glob_y = y+1;
		sum_y++;
		return;
	}
	else if (String == '\b') {
		if(Glob_x == 0) return;
		Glob_x-=2;
		buf_w[y * HSCREEN + x - 1] = 0;
	}
	else {
		if ((y >= VSCREEN) && (x >= 0)) {
			scroll();
			x = 0;
			y--;
		}      	              	

		char* b = &buf_w[y * HSCREEN + x];
		if(b >= SCROLL_END)
			b-= SIZE_SCROLL;
		*b = String;

		if(Glob_x >= HSCREEN)
		{
			Glob_x = 0;
			Glob_y++;
			sum_y++;
		}    
	}
	*/
	if (String == '\n') {
		if((y+1) > VSCREEN) {
			scroll();
			y--;
		}
		cur_process->console->Glob_x = 0;
		cur_process->console->Glob_y = y+1;
		cur_process->console->sum_y++;
		return;
	}
	else if (String == '\b') {
		if(cur_process->console->Glob_x == 0) return;
		cur_process->console->Glob_x-=2;
		cur_process->console->buf_w[y * HSCREEN + x - 1] = 0;
	}
	else {
		if ((y >= VSCREEN) && (x >= 0)) {
			scroll();
			x = 0;
			y--;
		}      	              	

		char* b = &cur_process->console->buf_w[y * HSCREEN + x];
		if(b >= SCROLL_END)
			b-= SIZE_SCROLL;
		*b = String;

		if(cur_process->console->Glob_x >= HSCREEN)
		{
			cur_process->console->Glob_x = 0;
			cur_process->console->Glob_y++;
			cur_process->console->sum_y++;
		}    
	}
#else
	CHAR *pScreen = (CHAR *)VIDIO_MEMORY;

	if (String == '\n') {
		if((y+1) > 24) {
			scroll();
			y--;
		}
		pScreen += ((y+1) * 80);
		Glob_x = 0;
		Glob_y = y+1;
	return;
	}
	else {
		if ((y > 24) && (x >= 0)) {
			scroll();
			x = 0; y--;
		}                       

		pScreen += ( y * 80) + x;
		pScreen[0].bAtt = 0x07;
		pScreen[0].bCh = String;

		if(Glob_x > 79)
		{
			Glob_x = 0;
			Glob_y++;
		}    
	}
#endif
}

/*change : variable to cur_console->variable*/
void clrScreen(void) 
{
	CHAR *pScreen = (CHAR *) VIDIO_MEMORY;
	int i;

	for (i = 0; i < 80*25; i++) {
		(*pScreen).bAtt = 0x07;
		(*pScreen++).bCh = ' ';
	}   
	/*
	Glob_x = 0;
	Glob_y = 0;
	*/
	cur_console->Glob_x = 0;
	cur_console->Glob_y = 0;
}

//Ctrl+l 화면 클리어 구현
void clearScreen(void)
{
	set_fallow();						// goto end of the scroll
	refreshScreen();
	while (cur_console->Glob_y != 0) {	//modified scroll() function which us cur_console->varialbe
		cur_console->buf_w += HSCREEN;
		cur_console->buf_p += HSCREEN;
		
		while(cur_console->buf_w > SCROLL_END)
			cur_console->buf_w -= SIZE_SCROLL;
	
		
		/*
		//clear line
		int i;
		char *buf_ptr = cur_console->buf_w + SIZE_SCREEN;
		for(i = 0; i < HSCREEN; i++)
		{
			if(buf_ptr > SCROLL_END)
				buf_ptr -= SIZE_SCROLL;
			*(buf_ptr++) = 0;
		}
		*/
		cur_console->Glob_y--;
	}
}

/*change : variable to cur_process->console->variable*/
void scroll(void) 
{
#ifdef SCREEN_SCROLL
	/*
	buf_w += HSCREEN;
	buf_p += HSCREEN;
	
	while(buf_w > SCROLL_END)
		buf_w -= SIZE_SCROLL;

	
	//clear line
	int i;
	char *buf_ptr = buf_w + SIZE_SCREEN;
	for(i = 0; i < HSCREEN; i++)
	{
		if(buf_ptr > SCROLL_END)
			buf_ptr -= SIZE_SCROLL;
		*(buf_ptr++) = 0;
	}
	*/
	cur_process->console->buf_w += HSCREEN;
	cur_process->console->buf_p += HSCREEN;
	
	while(cur_process->console->buf_w > SCROLL_END)
		cur_process->console->buf_w -= SIZE_SCROLL;

	
	//clear line
	int i;
	char *buf_ptr = cur_process->console->buf_w + SIZE_SCREEN;
	for(i = 0; i < HSCREEN; i++)
	{
		if(buf_ptr > SCROLL_END)
			buf_ptr -= SIZE_SCROLL;
		*(buf_ptr++) = 0;
	}
		
//
#else
	CHAR *pScreen = (CHAR *) VIDIO_MEMORY;
	CHAR *pScrBuf = (CHAR *) (VIDIO_MEMORY + 2*80);
	int i;
	for (i = 0; i < 80*24; i++) {
		    (*pScreen).bAtt = (*pScrBuf).bAtt;
		        (*pScreen++).bCh = (*pScrBuf++).bCh;
	}   
	for (i = 0; i < 80; i++) {
		    (*pScreen).bAtt = 0x07;
		        (*pScreen++).bCh = ' ';
	} 
#endif
	/*
	   Glob_y--;
	*/
	cur_process->console->Glob_y--;

}

#ifdef SERIAL_STDOUT
void printCharToSerial(const char *pString)
{
	int i;
	enum intr_level old_level = intr_disable();
	for(;*pString != NULL; pString++)
	{
		if(*pString != '\n'){
			while((inb(LINE_STATUS) & THR_EMPTY) == 0)
				continue;
			outb(FIRST_SPORT, *pString);

		}
		else{
			for(i=0; i<2; i++){
				while((inb(LINE_STATUS) & THR_EMPTY) == 0)
					continue;
				outb(FIRST_SPORT, next_line[i]);
			}
		}
	}
	intr_set_level (old_level);
}
#endif


/*change : variable to cur_process->console->variable*/
int printk(const char *fmt, ...) {
	char buf[1024];
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsprintk(buf, fmt, args);
	va_end(args);
#ifdef SERIAL_STDOUT
	printCharToSerial(buf);
#endif
	/*
	PrintCharToScreen(Glob_x, Glob_y, buf);
	*/
	PrintCharToScreen(cur_process->console->Glob_x, cur_process->console->Glob_y, buf);

	return len;

}

/*change : variable to cur_console->variable*/
#ifdef SCREEN_SCROLL
void scroll_screen(int offset)
{
	/*
	char * tmp_buf_p;
	char * tmp_buf_w;
	if(a_s == TRUE && offset > 0 && buf_p == buf_w)
		return;

	a_s = FALSE;
	
	tmp_buf_p = (char*)((int)buf_p + (HSCREEN * offset));
	tmp_buf_w = buf_w + SIZE_SCREEN;
	if(tmp_buf_w > SCROLL_END)
		tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL);

	if(sum_y < NSCROLL && offset < 0 && tmp_buf_p <= buf_s && buf_p > buf_s) return;
	if(offset > 0 && tmp_buf_p > buf_w && buf_p <= buf_w) return;
	else if(offset < 0 && tmp_buf_p <= tmp_buf_w && buf_p > tmp_buf_w) return;

	buf_p = tmp_buf_p;
	
	if(buf_p >= SCROLL_END)
		buf_p = (char*)((int)buf_p - SIZE_SCROLL);
	else if(buf_p < buf_s)
		buf_p = (char*)((int)buf_p + SIZE_SCROLL);

	refreshScreen();
	*/
	char * tmp_buf_p;
	char * tmp_buf_w;
	if(cur_console->a_s == TRUE && offset > 0 && cur_console->buf_p == cur_console->buf_w)
		return;

	cur_console->a_s = FALSE;
	
	tmp_buf_p = (char*)((int)cur_console->buf_p + (HSCREEN * offset));
	tmp_buf_w = cur_console->buf_w + SIZE_SCREEN;
	if(tmp_buf_w > SCROLL_END)
		tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL);

	if(cur_console->sum_y < NSCROLL && offset < 0 && tmp_buf_p <= cur_console->buf_s && cur_console->buf_p > cur_console->buf_s) return;
	if(offset > 0 && tmp_buf_p > cur_console->buf_w && cur_console->buf_p <= cur_console->buf_w) return;
	else if(offset < 0 && tmp_buf_p <= tmp_buf_w && cur_console->buf_p > tmp_buf_w) return;

	//
	if (offset < 0 && tmp_buf_p < cur_console->buf_s) return;
	//if (offset < 0 && cur_console->buf_p > buf_s) return;

	cur_console->buf_p = tmp_buf_p;
	
	if(cur_console->buf_p >= SCROLL_END)
		cur_console->buf_p = (char*)((int)cur_console->buf_p - SIZE_SCROLL);
	else if(cur_console->buf_p < cur_console->buf_s)
		cur_console->buf_p = (char*)((int)cur_console->buf_p + SIZE_SCROLL);

	refreshScreen();
}

/*change : variable to cur_console->variable*/
void set_fallow(void)
{
	cur_console->a_s = TRUE;
}

/*change : variable to cur_console->variable*/
void refreshScreen(void)
{
	/*
	CHAR *p_s= (CHAR *) VIDIO_MEMORY;
	int i;

	if(a_s)
		buf_p = buf_w;

	char* b = buf_p;

	for(i = 0; i < SIZE_SCREEN; i++, b++, p_s++)
	{
		if(b >= SCROLL_END)
			b -= SIZE_SCROLL;
		p_s->bAtt = 0x07;
		p_s->bCh = *b;
	}
	*/
	CHAR *p_s= (CHAR *) VIDIO_MEMORY;
	int i;

	if(cur_console->a_s)
		cur_console->buf_p = cur_console->buf_w;

	char* b = cur_console->buf_p;

	for(i = 0; i < SIZE_SCREEN; i++, b++, p_s++)
	{
		if(b >= SCROLL_END)
			b -= SIZE_SCROLL;
		p_s->bAtt = 0x07;
		p_s->bCh = *b;
	}
}

//콘솔 할당 함수
struct Console *get_console(void){
	int i;

	for(i = 0; i < MAX_CONSOLE_NUM; i++){
		if(console[i].used == FALSE){
			console[i].used = TRUE;
			return &console[i];
		}
	}

	return NULL;
}
#endif

