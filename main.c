#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

struct termios original_state; // this stores the initial state of the terminal

void onerror(const char* err)
{
	perror(err);
	exit(1);
}

void disableRawMode()
{
	if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_state) == -1)
		onerror("tcsetattr");
}

void enableRawMode()
{
		
	// the tcgetattr() function will get all the terminal attributes and store it in the raw struct
	if( tcgetattr(STDIN_FILENO, &original_state) == -1)
		onerror("tcgetattr");
	
	// atexit is a library function from stdlib.h where the disableRawMode function gets called automatically when the program exits by either returning to main function or by calling the exit() function 
	atexit(disableRawMode);	
	
	struct termios raw = original_state;
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
int main()
{
	enableRawMode();
	
	// the iscntrl() function is used to check whether a character is a control character or not
	// control characters are non printable characters which controls the behaviour of the device or interpret text such as Enter, Backspace
	// ASCII codes from 0 - 31 and 127 are control characters
	
	while(1){
		char c = '\0';
		read(STDIN_FILENO,&c,1);		
	
		if (iscntrl(c)){
			printf("%d\r\n",c);
		}

		else {
			printf("%d ('%c')\r\n", c, c);
		}
		
		if (c == 'q') break;
	}

	printf("Ballz\r\n");
	return 0;
}

