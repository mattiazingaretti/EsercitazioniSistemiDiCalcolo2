#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

bool flag[2] = {false ,false};
int me = 0, turn = 0,other = 1 ; //Revert for P1

//simply pseudocode.
void dekker(){
    flag[me] = true;
    while(flag[other]){
        if (turn == other){
            flag[me] = false;
            while(turn == other) ; //busy waiting
            flag[me] = true;
        }
    }
    //CS
    turn = other;
    flag[me] = false;
}


int main(){

    dekker();

    exit(0);
}