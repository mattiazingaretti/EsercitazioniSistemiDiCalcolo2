#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

#define N 10;

bool passed[N] = {false,false,false,false,false,false,false,false,false,false,false};
bool interested[N] = {false,false,false,false,false,false,false,false,false,false,false};
int k = 0; //Può essere uno tra gli N 


void dijkstra(){
    int i ; //id processo i-esimo
    interested[i] = true;
    E:while(k != i) {
        passed[i] = false;
        if (!interested[k]) k = i;
    }
    passed[i] = true;
    for(int j = 0; j < N && j != i ; j++){
        if (passed[j]) goto E;
    }
    //CS
    passed[i] = false;
    interested[i] = false;
}



int main(){

    dijkstra();

    exit(0);
}