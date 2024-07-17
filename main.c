#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

struct termios original_state; // this stores the initial state of the terminal

void disableRawMode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_state);
}

void enableRawMode()
{
		
	// the tcgetattr() function will get all the terminal attributes and store it in the raw struct
	tcgetattr(STDIN_FILENO, &original_state);
	
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
	raw.c_iflag &= ~(IXON);

	// we can then set the modified raw struct value back to the terminal attribute with the tcsetattr() function
	tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw);
	
	// one thing to note is that this function permanently changes the terminal's attributes for the current session so we will create a function to restore the terminal to its original state	
}
int main()
{
	enableRawMode();
	char c;
	
	// the iscntrl() function is used to check whether a character is a control character or not
	// control characters are non printable characters which controls the behaviour of the device or interpret text such as Enter, Backspace
	// ASCII codes from 0 - 31 and 127 are control characters
	
	while(read(STDIN_FILENO,&c,1)==1 && c !='q'){
		if (iscntrl(c)){
			printf("%d\n",c);
		}

		else {
			printf("%d ('%c')\n", c, c);
		}
	}
	printf("Ballz\n");
	return 0;
}

