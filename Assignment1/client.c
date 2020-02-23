#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<ncurses.h>

int sd;
int reader_pos_y,reader_pos_x;
int writer_pos_y,writer_pos_x;
char writer_quit_flag=0;
char reader_quit_flag=0;
bool reader_busy=false;
char name[50];

int read_line(int sfd,char* myLine){
    char buff;
    int temp_a=read(sfd,&buff,1);
    if(temp_a<=0){
        return 1;
    }
    if(buff=='\0'){
        *myLine='\0';
    }
    else{
        *myLine=buff;
        myLine++;
        return read_line(sfd,myLine);
    }
    return 0;
}

int myScanf(char* buff){
    int count=0;
    while(1){
        if(reader_busy)continue;
        
        int temp=getch();
        if(temp == ERR){
            continue;
        }
        else if(temp=='\n'){
            writer_pos_x=0;
            move(writer_pos_y,writer_pos_x);
            break;
        }
        else if(temp==KEY_LEFT){
            writer_pos_x--;
            count--;
            move(writer_pos_y,writer_pos_x);
        }
        else if(temp==KEY_RIGHT){
            if(buff[++count]!=0){
                writer_pos_x++;
            }
            else{
                count--;
            }
            move(writer_pos_y,writer_pos_x);
        }
        else if(temp==127){
            if(count!=0){
                count--;
                buff[count]=0;
                writer_pos_x--;
                move(writer_pos_y,writer_pos_x);
                delch();
            }
        }
        else{
            buff[count]=temp;
            addch(temp);
            count++;
            writer_pos_x++;
        }
    }
    return count;
}

int scroll_up_end(int a){
    scrl(a);
    reader_pos_y-=a;
    
    char buff[1000];
    mvinnstr(LINES-1-a, 0, buff, COLS-1);
    
    move(LINES-1-a,0);
    clrtoeol();
    
    mvprintw(LINES-1,0,"%s",buff);
    return 0;
}

void* reader_func(void* dump){
    /*void** arg_dump=dump;
    int sd=*((int*)arg_dump[0]);
    char* reader_quit_flag=arg_dump[1];
    char* name=arg_dump[2];*/
    
    char buff[1000];
    
    //reader_pos_x=0;reader_pos_y=1;
    
    while(1){
        int server_working=read_line(sd,buff);
        
        if(server_working==1){
            reader_quit_flag=1;
            return NULL;
        }
        
        if (reader_pos_y==LINES-3) {
            scroll_up_end(1);
        }
        
        if(buff[0]=='-'){
            if(buff[1]=='m'){
                if(buff[3]=='-'){
                    if(buff[4]=='c'){
                        if(buff[6]=='p'){
                            strcpy(buff+3,"Name change successful.\0");
                        }
                        else if(buff[6]=='n'){
                            strcpy(name,buff+8);
                            strcpy(buff+3,"Name change unsuccessful. Reverting back in the next step. Press Enter again, and try again with a different name.\0");
                        }
                    }
                }
                int i,x=0;
                for(i=3;buff[i]!=0;i++){
                    if(buff[i]=='\n')x++;
                }
                if(reader_pos_y+x>LINES-3)scroll_up_end(reader_pos_y+x-LINES+3+1);
                
                reader_busy=true;
                move(reader_pos_y,reader_pos_x);
                clrtoeol();
                printw("Server says: %s",buff+3);
                
                
                
                reader_pos_x=0;
                reader_pos_y+=x+1;
                move(writer_pos_y,writer_pos_x);
                reader_busy=false;
            }
        }
        else{
            
            reader_busy=true;
            move(reader_pos_y,reader_pos_x);
            clrtoeol();
            
            printw("%s",buff);
            
            
            int i,x=0;
            for(i=3;buff[i]!=0;i++){
                if(buff[i]=='\n')x++;
            }
            
            reader_pos_x=0;
            reader_pos_y++;
            move(writer_pos_y,writer_pos_x);
            reader_busy=false;
            
        }
        
    }
    return NULL;
}

void* writer_func(void* dump){
    /*void** arg_dump=dump;
    int sd=*((int*)arg_dump[0]);
    char* writer_quit_flag=arg_dump[1];
    char* name=arg_dump[2];*/
    
    char buff[1000];
    while(1){
        if(reader_busy)continue;
        mvprintw(LINES-1,0,"%s: ",name);
        clrtoeol();
        writer_pos_x=strlen(name)+2;
        writer_pos_y=LINES-1;
        
        int temp_a=myScanf(buff);
        clrtoeol();
        buff[temp_a]='\0';
        
        if(temp_a==0)continue;
        
        if(buff[0]=='-'){
            if(buff[1]=='q'){
                writer_quit_flag=2;
                break;
            }
            else if(buff[1]=='r'){
                erase();
                reader_pos_y=0;
                reader_pos_x=0;
            }
            else if(buff[1]=='n'){
                if(buff[2]!=0)scroll_up_end(buff[2]-48-1);
                else scroll_up_end(2-1);
            }
            else if(buff[1]=='c'){
                strcpy(name,buff+3);
            }
        }
        
        if(reader_pos_y==LINES-3)scroll_up_end(1);
        
        /*
        move(reader_pos_y,reader_pos_x);
        clrtoeol();
        printw("%s: %s",name,buff);
        reader_pos_x=0;
        reader_pos_y++;
         */
        
        int temp_b=write(sd,buff,strlen(buff)+1);
        
        if(temp_b<0){
            writer_quit_flag=1;
            break;
        }
    }
    return NULL;
}

int main(int arc, char** argv){
    initscr();
    cbreak();
    noecho();
    timeout(0);
    scrollok(stdscr,true);
    keypad(stdscr,true);
    clear();
    
	struct sockaddr_in server;

	sd=socket(AF_INET,SOCK_STREAM,0);
    
    server.sin_family=AF_INET;
	
    char* serverAddressString=argv[1];
    
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(atoi(argv[2]));
    
    
    writer_pos_y=0;writer_pos_x=45;
    printw("Give your name(in not more than 50 letters): ");
    int temp_a=myScanf(name);
    name[temp_a]='\0';
    
    while(1){
        //------
        if(connect(sd,(struct sockaddr *)&server,sizeof(server))<0){
            endwin();
            close(sd);
            printf("Unable to connect. Try again.\n");
            return 1;
        }
        
        write(sd,name,strlen(name)+1);
    
        char temp_buff[4];
        int temp_read_name_flag=read_line(sd,temp_buff);
        if(temp_read_name_flag==1 || temp_buff[0]=='N'){
            close(sd);
            sd=socket(AF_INET,SOCK_STREAM,0);
            mvprintw(++writer_pos_y,0,"Give a different name(in not more than 50 letters): ");
            writer_pos_x=52;
            
            temp_a=myScanf(name);
            name[temp_a]='\0';
        }
        else{
            reader_pos_x=0;reader_pos_y=writer_pos_y+1;
            break;
        }
        //------
    }
    
    pthread_t reader;
    pthread_t writer;
    
    
    
    //void* arg_dump_reader[3];arg_dump_reader[0]=&sd;arg_dump_reader[1]=&reader_quit_flag;arg_dump_reader[2]=name;
    //void* arg_dump_writer[3];arg_dump_writer[0]=&sd;arg_dump_writer[1]=&writer_quit_flag;arg_dump_writer[2]=name;
    pthread_create(&reader,NULL,reader_func,NULL);//arg_dump_reader);
    pthread_create(&writer,NULL,writer_func,NULL);//arg_dump_writer);
    
    while(1){
        if(writer_quit_flag==1 || reader_quit_flag==1){
            close(sd);
            endwin();
            printf("Cannot establish connection to server.\n");
            return 1;
        }
        if(writer_quit_flag==2){
            close(sd);
            endwin();
            printf("Bye!\n");
            return 0;
        }
    }
    endwin();
	return 2;
}
