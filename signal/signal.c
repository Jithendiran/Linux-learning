#include<stdio.h>

/*
signal
void ( *signal(int sig, void (*handler)(int)) ) (int);

How to read
1.void (*handler)(int) - This is a function pointer :
   * Parameter: int
   * Return : void

2. signal(int sig, void (*handler)(int)) - The signal function takes:
    * Parameter: int ,the signal number
    * Parameter: function pointer ,void (*handler)(int) - a function pointer (the signal handler)
    * Return type: Pointer to a function void (*)(int)

3. void (*signal(...))(int) or void (*)(int) - The signal function returns:
    * A function pointer that takes an int and returns void
*/

int main(){
    
}