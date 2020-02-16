#include<stdio.h>
#include<stdlib.h>
//#include<ncurses.h>
#include<string.h>

/*
int myScanf(char* buff){
    int count=0;
    while(1){
        char temp=getchar();
        if(temp=='\n')break;
        else buff[count]=temp;
        count++;
    }
    return count;
}
*/
int main(){
    /*
	char name[100];
	int a=myScanf(name);
	printf("%d\n",a);
    printf("%s\n",name);*/
    
    /*
    initscr();
    cbreak();
    keypad(stdscr,true);
    printw("Hello: ");
    int temp=getch();
    printw(" %d ",temp);
    if(temp==24127){
        printw("BACK SPACE PRESSED!");
    }
    if(temp==KEY_DC){
        printw("Del pressed.");
    }
    getch();
    endwin();
     */
    
    char *p=malloc(20);
    char *q=malloc(20);
    
    strcpy(p,"awesome\0");
    strcpy(q,"goregoregore\0");
    
    
    strcpy(q,p);
    
    printf("P: %s\n",q);
    printf("Q: %s\n",q);
    
    return 0;
}
