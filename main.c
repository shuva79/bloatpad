/*** include ***/
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

/*** define ***/
#define CTRL_KEY(k) ((k) & 0x1f) // this macro stimulates what the usual CTRL does in the terminal
// the 0x1f in binary is 00011111 so this macro strips the upper 3 bits of the character
#define ABUF_INIT {NULL, 0} 	// it is a constant which represents an epmty buffer {buffer, length}. Acts as a constructor for our abuf struct


/*** structure data ***/
// this global struct will contain our editor state from which we can store the width and height of the terminal
struct editorConfiguration{
	int screenrows;
	int screencols;
	struct termios original_state; // this stores the initial state of the terminal
};

struct editorConfiguration E;

/*** initialising functions ***/
void RefreshScreen();

/*** terminal functions ***/
void onerror(const char* err)
{
	RefreshScreen();
	perror(err);
	exit(1);
}

void disableRawMode()
{
	if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_state) == -1)
		onerror("tcsetattr");
}

void enableRawMode()
{
		
	// the tcgetattr() function will get all the terminal attributes and store it in the raw struct
	if( tcgetattr(STDIN_FILENO, &E.original_state) == -1)
		onerror("tcgetattr");
	
	// atexit is a library function from stdlib.h where the disableRawMode function gets called automatically when the program exits by either returning to main function or by calling the exit() function 
	atexit(disableRawMode);	
	
	struct termios raw = E.original_state;
	/*
		The echo feature enables us to see the characters that we type on the terminal
		We can see the echo feature turned off such as when we enter our passwords 

		c_lflag field is for local flags, or miscellaneous flags; flags other than input, output or control flag

		Info about modes can be found in this documentation https://www.man7.org/linux/man-pages/man0/termios.h.0p.html
		
		ECHO is defined as 00000000000000000000000000001000 in binary, which when passed through a bitwise NOT gives us 11111111111111111111111111110111. We then bitwise AND this value to make sure that the 4th bit of every flag is 0
		
		There is an ICANON flag that allows us to turn off canonical/cooked mode which means that we will read the input byte-by-byte instead of line by line. ICANON comes from termios.h and although it started with an I which is typical for input flags, it is a local flag. ICANON = Canonical Input	
	*/
	
	// ISIG flag is used for enabling signals, disabling it would effectly render CTRL-C and CTRL-Z useless.
	// IEXTEN is used to enable extended input character procerssing aka CTRL-V where the following character is inserted literally without performing any associated action
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	// IXON enables the start/stop output control and IXOFF enables start/stop input control aka CTRL-S and CTRL-Q
	// c_iflag because IXON is an input flag	
	// CTRL-M is being read as 10, which is the same value as Enter, which is the same value as CTRL-J. This means that the terminal is translating carriage returns (CR) into newline (NL). We can use the ICRNL feature to toggle this.
	// carriage return is used to reset the device's position to the beginning of a line
	// BRKINT causes a SIGINT during a break condition	
	// INPCK enables parity checking
	// ISTRIP causes the 8th bit of every input byte to be stripped (set to 0)

	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

	// OPOST flag is used to post-process output, or output processing features such as the \n translation into \r\n. This will cause the newline to start directly below and not to the left of the terminal. We can fix this by simply hard coding \r\n into our print statement in the main function. 
	raw.c_oflag &= ~(OPOST);

	// we also need to turn off a control bitmask CS8 or character size 8, setting the character size to 8 bits per byte.
	// it has to be set instead of being turned off by using the bitwise or | operator
	raw.c_cflag |= (CS8);	
	
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 100;

	// we can then set the modified raw struct value back to the terminal attribute with the tcsetattr() function

	if ( tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw) == -1) 
		onerror("tcsetattr");
	
	// one thing to note is that this function permanently changes the terminal's attributes for the current session so we will create a function to restore the terminal to its original state	
}

int getWindowSize(int *rows, int *columns) 	// this helps us to get the screen size
{
	struct winsize ws;
	
	// We can get the size of the terminal using the ioctl() function with the TIOCGWINSZ or Terminal IOCtl (Input Output Control) Get WINdow SiZe which is a part of the <sys/ioctl.h> library

	// ioctl will place the number of columns and rows of the terminal to the given winsize struct. On failure, it returns -1. 	
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ,&ws) == -1 || ws.ws_col==0)
	{
		return -1;
	}
	else
	{
		*columns = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** character input ***/
char ReadKey()
{
	ssize_t read_num;
	char c;
	
	while (( read_num = read(STDIN_FILENO, &c, 1 )) != 1 ) // this waits for an input from the user
	{ 
		if ( read_num == -1 && errno != EAGAIN ) onerror("read");
	}
	return c;
}

void ProcessKeypress()				// we'll use this for our character input
{
	char c = ReadKey();
	
	switch (c)
	{
		case CTRL_KEY('q'):
			RefreshScreen();
			exit(0);
			break;
	}
}

/*** append buffer ***/
// instead of writing a number of small write()s on the screen for the ~, we can instead print one big write instead
struct abuf 			
{
	char *b;
	int len;
};

// define a abAppend() function and a abFree() destructor.
// for dynamic memory allocation, we have to allocate enough memory to hold the new string, which can be done using realloc
void abAppend(struct abuf *ab, const char* s, int len)
{
	char *mem_alloc = realloc(ab->b, ab->len + len);

	if (mem_alloc == NULL) return;
	//memcpy copies len bytes from the source string s to the position new + ab->len (i.e., appending s to the end of the existing buffer).
	memcpy(&mem_alloc[ab->len],s,len);
	ab->b = mem_alloc;
	ab->len += len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}


/*** character output ***/
void DrawRows(struct abuf *ab)
{
	int x;
	for (x = 0; x <= E.screenrows; x++)
	{
		abAppend(ab "~", 1);

		if (x < E.screenrows - 1) 
		{
			abAppend(ab, "\r\n", 2);		// this ensures that the last line of the editor isnt empty and has a tilda ~
		}
	}
}

void RefreshScreen()
{
	// \x1b is the escape character or 27 in decimal and each escape sequence starts with an escape character followed by a { character.
	// escape sequences instruct the terminal to do various text formatting tasks such as colouring texts, moving the cursor, clearing parts of the screen, etc.
	// J is used as the erase in display to clear the screen http://vt100.net/docs/vt100-ug/chapter3.html#ED
	// The 2 before the J is the argument which tells to clear the screen. There are similar arguments 0 (<esc>[0J) which clears the screen from the cursor up to the end of the screen and 1 (<esc>[1J), which would clear the screen upto where the cursor is.
	// 
	// the 4 indicates that 4 byts of data is being written
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[2J", 4);

	// This escape sequence is 3 bytes long and makes use of the H command (cursor position) to position the cursor. It takes two arguments row and column number. So, if you have a 100x50 size terminal, you can use the command <esc>[25;50H (we separate multiple commands using ;). 
	abAppend(&ab, "\x1b[H", 3);
	DrawRows(&ab);
	abAppend(&ab, "\x1b[H", 3);
	
	// instead of a number of small writes, what we have done is appended all the writes that we might have to do into a single buffer and then written it. We also have to free the allocated memory space.
	write(STDOUT_FILENO, ab.b, ab.len);

	// freeing the memory
	abFree(&ab);
}


/*** init ***/
void initializeEditor()
{
	if (getWindowSize(&E.screenrows, &E.screencols) == -1)			// We have passed the addresses of the struct members which will be used to set int references in the getWindowSize function. 
		onerror("getWindowsSize");
}

int main()
{
	enableRawMode();
	initializeEditor();	
	// the iscntrl() function is used to check whether a character is a control character or not
	// control characters are non printable characters which controls the behaviour of the device or interpret text such as Enter, Backspace
	// ASCII codes from 0 - 31 and 127 are control characters
	
	while(1)
	{	
		RefreshScreen();
		ProcessKeypress();
	}

	printf("Ballz\r\n");
	return 0;
}

