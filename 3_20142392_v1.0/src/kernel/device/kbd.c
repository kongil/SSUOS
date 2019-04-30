#include <device/kbd.h>
#include <type.h>
#include <device/console.h>
#include <interrupt.h>
#include <device/io.h>
#include <ssulib.h>

#include <proc/proc.h>
static Key_Status KStat;

static char kbd_buf[BUFSIZ];
int buf_head, buf_tail;
//위 전역변수를 사용하는 코드를 cur_foreground_process를 사용하는 코드로 변경
extern struct process *cur_foreground_process;
Kbd_buffer kbd_buffer[MAX_KBD_BUFFER];

static BYTE Kbd_Map[4][KBDMAPSIZE] = {
	{ /* default */
		0x00, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x00, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x00, '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* capslock */
		0x00, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', 0x00, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0x00, '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* Shift */
		0x00, 0x00, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0x00,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x00, 0x00, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0x00, '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* Capslock & Shift  */
		0x00, 0x00, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0x00,
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 0x00, 0x00, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~', 0x00, '|', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', '<', '>', '?', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};

static bool kbd_remove_char();

/*change : initialize all the kbd_buffer array*/
void init_kbd(void)
{
	KStat.ShiftFlag = 0;
	KStat.CapslockFlag = 0;
	KStat.NumlockFLag = 0;
	KStat.ScrolllockFlag = 0;
	KStat.ExtentedFlag = 0;
	KStat.PauseFlag = 0;

	/*
	buf_head = 0;
	buf_tail = 0;
	*/
	int i, j;
	for (i = 0; i < MAX_KBD_BUFFER; i++) {			//initialize kbd_buffer array rather tha local varialbe
		for (j = 0; j < KBD_BUFFER_SIZE; j++)
			kbd_buffer[i].buf[j] = kbd_buf[j];	
		kbd_buffer[i].head = 0;	
		kbd_buffer[i].tail = 0;	
		kbd_buffer[i].used = FALSE;	
	}
	cur_process->kbd_buffer = get_kbd_buffer();

	reg_handler(33, kbd_handler);
}

/*change : add CtrlFlag*/
void UpdateKeyStat(BYTE Scancode)
{
	if(Scancode & 0x80)	// break code
	{
		if(Scancode == 0xB6 || Scancode == 0xAA) //Lshift, Rshift + 0x80
		{
			KStat.ShiftFlag = FALSE;
		}
		else if(Scancode == 0x9d) { //Ctrl+0x80 Ctrl unpressed
			KStat.CtrlFlag = FALSE;
		}
	}
	else
	{
		if(Scancode == 0x3A && KStat.CapslockFlag)
		{
			KStat.CapslockFlag = FALSE;
		}
		else if(Scancode == 0x3A)
			KStat.CapslockFlag = TRUE;
		else if(Scancode == 0x36 || Scancode == 0x2A)
		{
			KStat.ShiftFlag = TRUE;
		}
		//
		else if(Scancode == 0x1d) {
				KStat.CtrlFlag = TRUE;
		}
	}
	if(Scancode == 0xE0) // function+ ....
	{
		KStat.ExtentedFlag = TRUE;
	}
	else if(KStat.ExtentedFlag == TRUE && Scancode != 0xE0)
	{
		KStat.ExtentedFlag = FALSE;
	}
}

BOOL ConvertScancodeToASCII(BYTE Scancode, BYTE *Asciicode)
{
	if(KStat.PauseFlag > 0)
	{
		KStat.PauseFlag--;
		return FALSE;
	}

	if(KStat.ExtentedFlag == TRUE)
	{
		if(Scancode & 0x80)	
			return FALSE;
		*Asciicode = Scancode;
		return TRUE;
	}

	if(Scancode == 0xE1)
	{
		*Asciicode = 0x00;
		KStat.PauseFlag = 2;
		return FALSE;
	}
	else if(Scancode == 0xE0)
	{
		*Asciicode = 0x00;
		return FALSE;
	}

	if(!(Scancode & 0x80))
	{
		if(KStat.ShiftFlag & KStat.CapslockFlag)
		{
			*Asciicode = Kbd_Map[3][Scancode & 0x7F];
		}
		else if(KStat.ShiftFlag)
		{
			*Asciicode = Kbd_Map[2][Scancode & 0x7F];
		}
		else if(KStat.CapslockFlag)
		{
			*Asciicode = Kbd_Map[1][Scancode & 0x7F];
		}
		else
		{
			*Asciicode = Kbd_Map[0][Scancode];
		}

		return TRUE;
	}
	return FALSE;
}

/*change : local variable ro cur_foreground_process->kbd_buffer->local_varialbe*/
bool isFull()
{
	return (cur_foreground_process->kbd_buffer->head-1) % BUFSIZ == cur_foreground_process->kbd_buffer->tail;
}

/*change : local variable ro cur_process->kbd_buffer->local_varialbe*/
bool isEmpty()
{
	return cur_process->kbd_buffer->head == cur_process->kbd_buffer->tail;
	//return cur_foreground_process->kbd_buffer->head == cur_foreground_process->kbd_buffer->tail;
}

/*change : add function when Tab, l is pressed with CtrlFlag && local variable to cur_foreground_process->kbd_buffer->varable*/
void kbd_handler(struct intr_frame *iframe)
{
	BYTE asciicode;
	BYTE data = inb(0x60);

	if(ConvertScancodeToASCII(data, &asciicode))
	{

#ifdef SCREEN_SCROLL
		if( KStat.ExtentedFlag == TRUE)
		{
			switch(asciicode)
			{
				case 72://UP
					break;
				case 80 ://DOWN
					break;
				case 75 ://LEFT
					break;
				case 77 ://RIGHT
					break;
				case 73 :// PageUp
					scroll_screen(-1);
					break;
				case 81 :// PageDown
					scroll_screen(+1);
					//clearScreen();
					break;
				case 79 ://End
					set_fallow();
					break;
				case 29 :/*kongil:1d->Rctrl*/
					KStat.CtrlFlag = TRUE;
					break;
				default:
					break;
			}
		}
		else if( KStat.CtrlFlag == TRUE) {	 		//After ctrl
			if(data == 0x0f && KStat.CtrlFlag) { 	//Tab
				next_foreground_proc();
			}
			else if(data == 0x26 && KStat.CtrlFlag) { //l
				clearScreen();
			}
		}
		else if( !isFull() && asciicode != 0)
		{
			cur_foreground_process->kbd_buffer->buf[cur_foreground_process->kbd_buffer->tail] = asciicode;
			cur_foreground_process->kbd_buffer->tail = (cur_foreground_process->kbd_buffer->tail + 1) % BUFSIZ;
		}

#endif
		
	}
	UpdateKeyStat(data);
}

/*change : local variable ro cur_process->kbd_buffer->local_varialbe*/
char kbd_read_char()
{
	if( isEmpty())
		return -1;
	char ret;
	ret = cur_process->kbd_buffer->buf[cur_process->kbd_buffer->head];
	cur_process->kbd_buffer->head = (cur_process->kbd_buffer->head + 1)%BUFSIZ;
	return ret;
}

//입력 버퍼 할당
struct Kbd_buffer *get_kbd_buffer(){
	int i;

	for(i = 0; i < MAX_KBD_BUFFER; i++){
		if(kbd_buffer[i].used == false){
			kbd_buffer[i].used = true;
			return &kbd_buffer[i];
		}
	}

	return NULL;
}
