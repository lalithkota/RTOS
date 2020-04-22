#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<ncurses.h>
#include<pulse/simple.h>
#include<pulse/error.h>

pa_simple *s_read;
pa_simple *s_write;
static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 1
};
int pa_error;

char serverAddressString[20];
int port;

int sd;
int reader_pos_y,reader_pos_x;
int writer_pos_y,writer_pos_x;
char writer_quit_flag=0;
char reader_quit_flag=0;
bool reader_busy=false;
char name[50];
char uname[50];
//bool in_call=false;
//bool reader_in_call_busy=false;
bool is_caller;
bool call_func1_close_flag = false, call_func2_close_flag = false;

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
        else if(temp==KEY_BACKSPACE){
            if(count!=0){
                count--;
                buff[count]=0;
                writer_pos_x--;
                move(writer_pos_y,writer_pos_x);
                delch();
            }
        }
		else if(temp==KEY_DC){
			
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

void call_finish(){
	// on the ui side finish;
	move(LINES-1,0);
	clrtoeol();
	printw("%s: ",name);
	refresh();
	writer_pos_x = strlen(name)+2;
	writer_pos_y = LINES-1;
	
	reader_busy = false;
}
void* call_func1(void*);
void* call_func2(void*);
void* call_start(void* dump){
	
	move(LINES-1,0);
	clrtoeol();
	printw("INCOMING CALL");
	refresh();
	
	struct sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(port+1);
	int call_server_sd = socket(AF_INET,SOCK_STREAM,0);
	
	int count = 0;
	while(true){
		if(connect(call_server_sd,(struct sockaddr *)&server,sizeof(server))<0){
			usleep(100000);
			if(count++>10){
				call_finish();
				return NULL;
			}
		}
		else{
			break;
		}
	}
	
	char bu1[1];
	if(is_caller) bu1[0] = 1;
	else bu1[0] = 0;
	write(call_server_sd, bu1,1);
	
	read(call_server_sd, bu1, 1);
	
	if(bu1[0]==2){
		//print sometihng
		call_finish();
		return NULL;
	}
	if(bu1[0]==0){
		//print something
		call_finish();
		return NULL;
	}
	printw("starting call funcs");
	refresh();
	
	pthread_t func1_id, func2_id;
	pthread_create(&func1_id, NULL, call_func1, &call_server_sd);
	pthread_create(&func2_id, NULL, call_func2, &call_server_sd);
	
	while(!(call_func1_close_flag && call_func2_close_flag));
	
	close(call_server_sd);
	printw("call_quit_success");
	refresh();
	call_finish();
	return NULL;
}

void* call_func1(void * dump){
	int call_server_sd = *((int*)dump);
	char buff[1024];
	
	if (!(s_read = pa_simple_new(NULL, "VoiceChatter", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &pa_error))) {
        fprintf(stderr, __FILE__": pa_simple_new() for read failed: %s\n", pa_strerror(pa_error));
        call_func1_close_flag = true;
		return NULL;
    }
	
	while(true){
		
		if (pa_simple_read(s_read, buff, sizeof(buff), &pa_error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(pa_error));
            break;
        }
		
		if(call_func2_close_flag){
			break;
		}
		if(write(call_server_sd, buff,sizeof(buff))<=0){
			break;
		}
	}
	
	call_func1_close_flag = true;
	return NULL;
}

void* call_func2(void* dump){
	int call_server_sd = *((int*)dump);
	char buff[1024];
	
	if (!(s_write = pa_simple_new(NULL, "VoiceChatter", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &pa_error))) {
        fprintf(stderr, __FILE__": pa_simple_new() for write failed: %s\n", pa_strerror(pa_error));
        call_func2_close_flag = true;
		return NULL;
    }
	
	while(true){
		
		if(call_func1_close_flag){
			break;
		}
		if(read(call_server_sd, buff, sizeof(buff))<=0){
			break;
		}
		
		if(pa_simple_write(s_write,buff,sizeof(buff),&pa_error)<0){
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(pa_error));
            break;
        }
	}
	
	call_func1_close_flag = true;
	return NULL;
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
            else if(buff[1]=='v'){
				reader_busy=true;
				
                move(LINES-1,0);
				clrtoeol();
				printw("INCOMING CALL?");
				
				int key;
				while(true){
					key=getch();
					if(key == 'y'){
						is_caller = true;
						break;
					}
					else if(key=='n'){
						is_caller= false;
						break;
					}
				}
				pthread_t tid;
				pthread_create(&tid,NULL,call_start,NULL);
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
    char call_buff[1024];
    while(1){
		
        if(reader_busy)continue;
        
        move(LINES-1,0);
        clrtoeol();
        
        printw("%s: ",name);
        writer_pos_x=strlen(name)+2;
        writer_pos_y=LINES-1;
        
        int temp_a=myScanf(buff);
        
        if(temp_a==-1)continue;
        
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
            else if(buff[1]=='v'){
				write(sd,"-v\0",strlen("-v\0")+1);
				is_caller = true;
				pthread_t tid;
				pthread_create(&tid,NULL,call_start,NULL);
				
				/*call lalli
				LALLI: "hello"
				GOOD GIRL: "barre....BARRESH EDAVA"
				LALLI: "KOTI"
				GOOD GIRL: "MUMMY IS A BAAAAAA...........AAD GIRL,WE DONT LIKE HER VINNAVA MISTER
							LALLI IS A BAD BOY WE DONT LIKE HIM VINNAVA MISTER "
			    LALLI: "GOOD GIRL ANTE MUMMY KI ASSALU ISTAM UNDADU NENANTE NE ISTAM NUVVU ASSALU ISTAM LEDU"
				*/
				
		
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
    /*if (!(s_read = pa_simple_new(NULL, "VoiceChatter", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &pa_error))) {
        fprintf(stderr, __FILE__": pa_simple_new() for read failed: %s\n", pa_strerror(pa_error));
        return 34;
    }*/
    /*if (!(s_write = pa_simple_new(NULL, "VoiceChatter", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &pa_error))) {
        fprintf(stderr, __FILE__": pa_simple_new() for write failed: %s\n", pa_strerror(pa_error));
        return 34;
    }*/
    
    //unsigned long buff[1024];
    /*int i;
    for(i=0;i<1024;i++){
        buff[i]=i;
    }*/
    /*while(true){
        if (pa_simple_read(s_read, buff, sizeof(buff), &pa_error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(pa_error));
            return 35;
        }

        
        if(pa_simple_write(s_write,buff,sizeof(buff),&pa_error)<0){
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(pa_error));
            return 36;
        }
    }
    
    return 37;*/
    
    initscr();
    cbreak();
    noecho();
    timeout(0);
	nodelay(stdscr,true);
    scrollok(stdscr,true);
    keypad(stdscr,true);
    clear();
    
	struct sockaddr_in server;

	sd=socket(AF_INET,SOCK_STREAM,0);
    
    server.sin_family=AF_INET;
	
    strcpy(serverAddressString, argv[1]);
    port = atoi(argv[2]);
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(port);
    
    
    writer_pos_y=1;writer_pos_x=strlen("(This username is unique for each person, and might be visible to others): ");
    printw("Give a username(in not more than 50 letters)\n(This username is unique for each person, and might be visible to others): ");
    int temp_a=myScanf(uname);	//scan for username which is sent to server, below in the loop start
    uname[temp_a]='\0';
	
	char password[50];
	writer_pos_y++;writer_pos_x=strlen("Give Your Password (in not more than 50 letters. This can be anything): ");
	mvprintw(writer_pos_y,0,"Give Your Password (in not more than 50 letters. This can be anything): ");
	temp_a=myScanf(password);	//scan for password which is sent to server, below in the loop start
	password[temp_a]='\0';
    
    while(1){
        //------
        if(connect(sd,(struct sockaddr *)&server,sizeof(server))<0){
            endwin();
            close(sd);
            printf("Unable to connect. Try again.\n");
            return 1;
        }
        
        write(sd,uname,strlen(uname)+1);		//send username
		write(sd,password,strlen(password)+1);	//send password
    
        char temp_buff[5];
        int temp_read_name_flag=read_line(sd,temp_buff); // read acknowledgement
        if(temp_read_name_flag==1){
			//server quit in bad position .. rarely happens
		}
		else if(temp_buff[0]=='N'){		// Username missmatch ack
            close(sd);
            sd=socket(AF_INET,SOCK_STREAM,0);
            mvprintw(++writer_pos_y,0,"Give a different username(in not more than 50 letters): ");
            writer_pos_x=strlen("Give a different username(in not more than 50 letters): ");
            
            int temp_a=myScanf(uname);
            uname[temp_a]='\0';
            
        }
		else if(temp_buff[0]=='P'){		// Password missmatch ack
			close(sd);
            sd=socket(AF_INET,SOCK_STREAM,0);
            mvprintw(++writer_pos_y,0,"Incorrect Password. Try Again: ");
            writer_pos_x=strlen("Incorrect Password. Try Again: ");
            
            int temp_a=myScanf(password);
            password[temp_a]='\0';
            
		}
        else{							// Accepted ack
			
			writer_pos_y++;writer_pos_x=strlen("Give Your Name (in not more than 50 letters. This can be anything): ");
			mvprintw(writer_pos_y,0,"Give Your Name (in not more than 50 letters. This can be anything): ");
			temp_a=myScanf(name);
			name[temp_a]='\0';
			
			write(sd,name,strlen(name)+1);
			
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
