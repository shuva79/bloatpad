#include <stdio.h>
#include <unistd.h>
#include <termios.h>

void enableRawMode()
{
		
	struct termios raw;
	// the tcgetattr() function will get all the terminal attributes and store it in the raw struct
	tcgetattr(STDIN_FILENO, &raw);
	
	/*
		The echo feature enables us to see the characters that we type on the terminal
		We can see the echo feature turned off such as when we enter our passwords 

		c_lflag field is for local flags, or miscellaneous flags; flags other than input, output or control flag
		
		ECHO is defined as 00000000000000000000000000001000 in binary, which when passed through a bitwise NOT gives us 11111111111111111111111111110111. We then bitwise AND this value to make sure that the 4th bit of every flag is 0
		
	*/
	raw.c_lflag &= ~(ECHO);

	// we can then set the modified raw struct value back to the terminal attribute with the tcsetattr() function
	tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw);
	
}
int main()
{
	enableRawMode();
	char c;
		
	while(read(STDIN_FILENO,&c,1)==1 && c !='q');
	printf("Ballz\n");
	return 0;
}

