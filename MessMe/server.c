#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdbool.h>
#include<string.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<signal.h>
//#include<pulse/simple.h>
//#include<pulse/error.h>

//pa_simple *s_write;

char* serverAddressString;
int port_no;
int sd;
int clientsds[100];
char** names;
char** unames;
char** passwords;
int** group_members;
pthread_t client_threads[100];
bool call_thread2_close_flag = false;
bool call_thread_close_flag = false;

bool i_lock = false;


void sig_handler(int sig){
    if(sig==SIGINT){
        printf("\nServer stopping execution\n");
        int i;
        for(i=0;i<100;i++){
            //pthread_kill(client_threads[i],SIGKILL);
            close(clientsds[i]);
        }
        close(sd);
        exit(0);
    }
    else if(sig==SIGSEGV){
        printf("... Segmentation Fault.\n");
        int i;
        for(i=0;i<100;i++){
            //pthread_kill(client_threads[i],SIGKILL);
            close(clientsds[i]);
        }
        close(sd);
        exit(0);
    }
}


int read_line(int sfd,char* myLine){
    char buff;
    int temp_a=read(sfd,&buff,1);
    if(temp_a<=0)return 1;
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

void new_group_req(int* returner, char* buff){
	if(returner==NULL){
		printf("give valid 2 unit returner array\n");
		return;
	}
	printf("take_new_group\n");
	bool is_it_there=false;
	int i,j=0;
	for(i=0;i<100 && unames[i]!=NULL;i++){
		if(unames[i][0]==':'){
			j++;
			if(strcmp(buff+4,unames[i]+1)==0){
				is_it_there=true;
				break;
			}
		}
	}
	if(is_it_there){
		//write(other_clients[my_id-1],"-m Please try with a different name.\0",strlen("-m Please try with a different name.\0")+1);
		int len = strlen(buff);
		buff[len]='1';
		buff[len+1]='\0';
		new_group_req(returner, buff);
		return;
	}
	
	
	returner[0]=i;
	returner[1]=j;
}

void* call_thread_func2(void*);
void* call_thread_func1(void*);

void* call_thread_main(void* dump){
	struct sockaddr_in server, client;
	
	int call_server_sd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(port_no + 1);

	bind(call_server_sd,(struct sockaddr *)&server,sizeof(server));
	listen(call_server_sd,2);
	
	unsigned clientLen = sizeof(client);
	int client1 = accept(call_server_sd,(struct sockaddr *)&client,&clientLen);
	int client2 = accept(call_server_sd,(struct sockaddr *)&client,&clientLen);
	
	//read both acks
	char bu1[1];
	read(client1,bu1,1);
	
	char bu2[1];
	read(client2,bu2,1);
	
	
	//write final ack back to both .. if bad then return
	if(bu1[0]!=1 || bu2[0]!=1){
		bu1[0] = 2;
		write(client1,bu1,1);
		write(client2,bu1,1);
		
		close(client1);
		close(client2);
		close(call_server_sd);
		return NULL;
	}
	bu1[0] = 1;
	write(client1,bu1,1);
	write(client2,bu1,1);
	
	int arg_dump[2];
	arg_dump[0]=client1;
	arg_dump[1]=client2;
	
	pthread_t func1_thread_id, func2_thread_id;
	pthread_create(&func2_thread_id,NULL,call_thread_func1,arg_dump);
	pthread_create(&func1_thread_id,NULL,call_thread_func2,arg_dump);
	
	printf("A new call started\n");
	
	while(!(call_thread_close_flag && call_thread2_close_flag)); // 
	
	call_thread2_close_flag = false;
	call_thread_close_flag = false;
	
	close(client1);
	close(client2);
	close(call_server_sd);
	printf("quit success\n");
	return NULL;
}

void* call_thread_func1(void* dump){
	int* arg_dump = dump;
	int client1 = arg_dump[0];
	int client2 = arg_dump[1];
	
	char buff[1024];
	while(true){
		if(read(client2, buff, 1024)<=0){
			printf("func1 read fail\n");
			break;
		}
		
		if(call_thread2_close_flag){
			break;
		}
		
		if(write(client1, buff, 1024)<=0){
			printf("func1 read fail\n");
			break;
		}
	}
	call_thread_close_flag = true;
	return NULL;
}

void* call_thread_func2(void* dump){
	int* arg_dump = dump;
	int client1 = arg_dump[0];
	int client2 = arg_dump[1];
	
	char buff[1024];
	
	int count=0;
	while(true){
		if(read(client1, buff, 1024)<=0){
			printf("func2 read fail\n");
			break;
		}
		
		if(call_thread_close_flag){
			break;
		}
		
		if(write(client2, buff, 1024)<=0){
			printf("func2 write fail\n");
			break;
		}
	}
	call_thread2_close_flag = true;
	return NULL;
}

void* start_client_exec(void* dump){
    void** arg_dump=dump;
    //char** names=((char**)(arg_dump[0]));
    int my_id=(int)((long)arg_dump[0]);
    //int* other_clients=(int*)arg_dump[2];
    int *other_clients= clientsds;
    //int** group_members=((int**)(arg_dump[3]));
    int* main_thread_i=arg_dump[1];
    
    int receiver_id=0;
    int group_id=0;
    
    char buff[1000];
    
    char call_buff[1024];
    
    while(1){
        
        
        int temp_a = read_line(other_clients[my_id-1],buff);
        if(temp_a==1){
			close(other_clients[my_id-1]);
            other_clients[my_id-1]=0;
            printf("%s left the server.\n",names[my_id-1]);
			
			int j=0;
			for(j=0;j<100;j++){
				if(other_clients[j]!=0 && other_clients[j]!=1 && unames[my_id-1][0]!=0){
					write(other_clients[j],"-q ",strlen("-q "));
					write(other_clients[j],unames[my_id-1],strlen(unames[my_id-1]));
					write(other_clients[j],"\0",1);
				}
			}
			
			names[my_id-1][0]=0;
			//unames[my_id-1][0]=0;
            
            return NULL;
        }
        
        if(buff[0]=='-'){
            // echo back to client
            write(other_clients[my_id-1],unames[my_id-1],strlen(unames[my_id-1]));
            write(other_clients[my_id-1],": ",2);
            write(other_clients[my_id-1],buff,strlen(buff)+1);
            //
			
            if(buff[1]=='s'){
                if(buff[2]=='r'){
                    printf("%s stopped chatting with %s\n",names[my_id-1],names[receiver_id-1]);
                    receiver_id=0;
                    group_id=0;
					continue;
                }
                if(buff[3]==':'){
                    write(other_clients[my_id-1],"-m Please use -g to select group\0",strlen("-m Please use -g to select group\0")+1);
                }
				if(buff[2]=='u'){
					bool is_it_there=false;
					int i;
					for(i=0;i<100 && unames[i]!=NULL;i++){
						if(strcmp(buff+4,unames[i])==0){
							is_it_there=true;
							break;
						}
					}
					if(is_it_there){
						if(my_id==i+1){
							write(other_clients[my_id-1],"-m Sorry for your lonliness.\0",strlen("-m Sorry for your lonliness.\0")+1);
						}
						else{
							receiver_id=i+1;
							printf("%s is talking to %s.\n",names[my_id-1],names[receiver_id-1]);
						}
					}
					else{
						write(other_clients[my_id-1],"-m No such person found.",strlen("-m No such person found."));
						if(receiver_id!=0){
							write(other_clients[my_id-1]," Still connected to ",strlen(" Still connected to "));
							write(other_clients[my_id-1],names[receiver_id-1],strlen(names[receiver_id-1]));
						}
						write(other_clients[my_id-1],"\0",1);
					}
				}
                else{
					bool is_it_there=false;
					int i;
					for(i=0;i<100 && names[i]!=NULL;i++){
						if(strcmp(buff+3,names[i])==0){
							is_it_there=true;
							break;
						}
					}
					if(is_it_there){
						if(my_id==i+1){
							write(other_clients[my_id-1],"-m Sorry for your lonliness.\0",strlen("-m Sorry for your lonliness.\0")+1);
						}
						else{
							receiver_id=i+1;
							printf("%s is talking to %s.\n",names[my_id-1],names[receiver_id-1]);
						}
					}
					else{
						write(other_clients[my_id-1],"-m No such person found.",strlen("-m No such person found."));
						if(receiver_id!=0){
							write(other_clients[my_id-1]," Still connected to ",strlen(" Still connected to "));
							write(other_clients[my_id-1],names[receiver_id-1],strlen(names[receiver_id-1]));
						}
						write(other_clients[my_id-1],"\0",1);
					}
				}
            }
            else if(buff[1]=='o'){
                if(receiver_id==0){
                    write(other_clients[my_id-1],"-m No one.\0",strlen("-m No one.\0")+1);
                }
                else if(names[receiver_id-1][0]!=':'){
                    write(other_clients[my_id-1],"-m Talking to ",strlen("-m Talking to "));
                    write(other_clients[my_id-1],names[receiver_id-1],strlen(names[receiver_id-1]));
                }
                else{
                    write(other_clients[my_id-1],"-m Talking to group ",strlen("-m Talking to group "));
                    write(other_clients[my_id-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                }
                write(other_clients[my_id-1],"\0",1);
            }
            else if(buff[1]=='l'){
                write(other_clients[my_id-1],"-m Here is the list:",strlen("-m Here is the list:"));
                int i;
                for(i=0; i<100 && names[i]!=NULL;i++){
                    if(names[i][0]!='\0'){
                        write(other_clients[my_id-1],"\n",1);
                        write(other_clients[my_id-1],names[i],strlen(names[i]));
						write(other_clients[my_id-1]," , uname:",strlen(" , uname:"));
						write(other_clients[my_id-1],unames[i],strlen(unames[i]));
                    }
                }
                write(other_clients[my_id-1],"\0",1);
            }
            /*else if(buff[1]=='q'){
                close(other_clients[my_id-1]);
                other_clients[my_id-1]=0;
                printf("%s left the server.\n",names[my_id-1]);
                names[my_id-1][0]=0;
                
                return NULL;
            }*/
            else if(buff[1]=='g'){
                if(buff[2]=='n'){
					char temp_name[50];
					strcpy(temp_name,buff);
					
					int* returner = malloc(2*sizeof(int));
					returner[0]=-1;
					returner[1]=-1;
                    
					new_group_req(returner, buff);
					if(returner[0]==-1){
						printf("something wrong\n");
						continue;
					}
					int i=returner[0];
					int j=returner[1];
                    
					//else{
					
					while(i_lock);
					i_lock = true;
					(*main_thread_i)++;
					i_lock = false;
					
					names[i]=malloc(50*sizeof(char));
					unames[i]=malloc(50*sizeof(char));
					names[i][0]=':';
					unames[i][0]=':';
					strcpy(names[i]+1,temp_name+4);
					strcpy(unames[i]+1,buff+4);
					receiver_id=i+1;
					group_id=j+1;
					//printf("OK! %d\n",j);
					
					other_clients[receiver_id-1]=1;
					
					write(other_clients[my_id-1],"-m Group created : ",strlen("-m Group created : "));
					write(other_clients[my_id-1],names[i]+1,strlen(names[i]+1));
					write(other_clients[my_id-1],"\0",1);
					
					write(other_clients[my_id-1],"-n ",strlen("-n "));
					write(other_clients[my_id-1],names[receiver_id-1],strlen(names[receiver_id-1]));
					write(other_clients[my_id-1]," -u ",strlen(" -u "));
					write(other_clients[my_id-1],unames[receiver_id-1],strlen(unames[receiver_id-1])+1);
					
					
					group_members[group_id-1]=(int*)malloc(100*sizeof(int));
					group_members[group_id-1][0]=1;					//first member of the members array is the size of it
					group_members[group_id-1][1]=receiver_id-1;		//second member is the index of this group in the all_clients/other_clients/names/unames array
					group_members[group_id-1][2]=my_id;				//third element onwards, it stores (the index of the person/group_member + 1) .. and the first member of a group is the one who created it
					
					printf("New Group created: %s\n",buff+4);
					//printf("%d\n",group_members[group_id-1][1]);                        
                    
					//}//else end
                    
                }
                else if(buff[2]=='a'){
					//first check if such user is present .. then check if he/she is actually already in the group
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                    }
                    else{
                        bool is_it_there=false;
                        int i;
                        for(i=0;i<100 && unames[i]!=NULL;i++){
                            if(strcmp(buff+4,unames[i])==0){
                                is_it_there=true;
                                break;
                            }
                        }
                        if(!is_it_there){
                            write(other_clients[my_id-1],"-m No such user available. Use -l for list of users.\0",strlen("-m No such user available. Use -l for list of users.\0")+1);
                        }
                        else{
                            bool is_it_group=false;
                            //bool is_self_group=false;
                            int j;
                            for(j=2;j<group_members[group_id-1][0] + 2;j++){
                                if(group_members[group_id-1][j]==0)continue;
                                if(strcmp(buff+4,unames[group_members[group_id-1][j]-1])==0){
                                    is_it_group=true;
                                    break;
                                }
                                
                            }
                            
                            
                            if(is_it_group){
                                write(other_clients[my_id-1],"-m That person already present in the group\0",strlen("-m That person already present in the group\0")+1);
                            }
                            else{
                                group_members[group_id-1][0]+=1;
                                group_members[group_id-1][j]=i+1;
                                //printf("%d\n",group_members[group_id-1][j]);
                                write(other_clients[my_id-1],"-m Added.\0",strlen("-m Added.\0")+1);
                                printf("%s added %s to %s.\n",names[my_id-1],names[i],names[receiver_id-1]);
								if(other_clients[i]!=0 && other_clients[i]!=1){
									write(other_clients[i],"-m ",strlen("-m "));
									write(other_clients[i],names[my_id-1],strlen(names[my_id-1]));
									write(other_clients[i]," added you to the group: ",strlen(" added you to the group: "));
									write(other_clients[i],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
									write(other_clients[i],"\0",1);
									
									write(other_clients[i],"-n ",strlen("-n "));
									write(other_clients[i],names[receiver_id-1],strlen(names[receiver_id-1]));
									write(other_clients[i]," -u ",strlen(" -u "));
									write(other_clients[i],unames[receiver_id-1],strlen(unames[receiver_id-1]));
									write(other_clients[i],"\0",1);
								}
                            }
                        }
                        
                    }
                    
                }
                else if(buff[2]=='r'){
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                        continue;
                    }
                    int i;
                    for(i=2; i < group_members[group_id-1][0] + 2; i++){
                        if(group_members[group_id-1][i]==0)continue;
                        if(strcmp(unames[my_id-1],unames[group_members[group_id-1][i]-1])==0){
                            group_members[group_id-1][i]=0;
                            break;
                        }
                    }
                    
                    
                    int j;
                    for(j=2;j< group_members[group_id-1][0] + 2;j++){
                        
                        if(group_members[group_id-1][j]==0)continue;
                        if(other_clients[group_members[group_id-1][i]-1]==0)continue;
                        if(names[group_members[group_id-1][j]-1][0]==':')continue;
                        
                        write(other_clients[group_members[group_id-1][j]-1],"-m ",strlen("-m "));
                        write(other_clients[group_members[group_id-1][j]-1],names[my_id-1],strlen(names[my_id-1]));
                        write(other_clients[group_members[group_id-1][j]-1]," left the group ",strlen(" left the group "));
                        write(other_clients[group_members[group_id-1][j]-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                        write(other_clients[group_members[group_id-1][j]-1],".\0",strlen(".\0")+1);
                    }
                    write(other_clients[my_id-1],"-m You left the group ",strlen("-m You left the group "));
                    write(other_clients[my_id-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                    write(other_clients[my_id-1],".\0",strlen(".\0")+1);
                    
                    printf("%s left the group %s\n",names[my_id-1],names[receiver_id-1]);
                    
                    group_id=0;
                    receiver_id=0;
                    
                    /*
                    bool is_it_there=false;
                    int i;
                    for(i=1; i <= group_members[group_id-1][0] ; i++){
                        if(strcmp(buff+4,names[group_members[group_id-1][i]-1])==0){
                            is_it_there=true;
                            break;
                        }
                    }
                    if(!is_it_there){
                        write(other_clients[my_id-1],"-m No such member is present is the group.\0",strlen("-m No such member is present is the group.\0")+1);
                    }
                    else{
                        int temp_a=group_members[group_id-1][i];
                        printf("%d\n",temp_a);
                        group_members[group_id-1][i]=0;
                        int j;
                        for(j=1;j<=group_members[group_id-1][0];j++){
                            
                            if(group_members[group_id-1][j]==0)continue;
                            if(names[group_members[group_id-1][j]-1][0]==':')continue;
                            
                            write(other_clients[group_members[group_id-1][j]-1],"-m ",strlen("-m "));
                            write(other_clients[group_members[group_id-1][j]-1],names[my_id-1],strlen(names[my_id-1]));
                            write(other_clients[group_members[group_id-1][j]-1]," removed ",strlen(" removed "));
                            write(other_clients[group_members[group_id-1][j]-1],names[temp_a-1],strlen(names[temp_a-1]));
                            write(other_clients[group_members[group_id-1][j]-1]," from the group ",strlen(" from the group "));
                            write(other_clients[group_members[group_id-1][j]-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                            write(other_clients[group_members[group_id-1][j]-1],".\0",strlen(".\0")+1);
                        }
                        write(other_clients[temp_a-1],"-m ",strlen("-m "));
                        write(other_clients[temp_a-1],names[my_id-1],strlen(names[my_id-1]));
                        write(other_clients[temp_a-1]," removed you from the group ",strlen(" removed you from the group "));
                        write(other_clients[temp_a-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                        write(other_clients[temp_a-1],".\0",strlen(".\0")+1);
                        
                        printf("%s removed %s from the group %s\n",names[my_id-1],names[temp_a-1],names[receiver_id-1]);
                    }*/
                }
                else if(buff[2]=='l'){
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                        continue;
                    }
                    write(other_clients[my_id-1],"-m Here are the members in the group:",strlen("-m Here are the members in the group:"));
                    int i;
                    for(i=2; i < group_members[group_id-1][0] + 2; i++){
                        //printf("OK %d\n",group_members[group_id-1][i]);
                        //printf("OK\n");
                        if(group_members[group_id-1][i]==0)continue;
                        if(names[group_members[group_id-1][i]-1][0]!='\0'){
                            write(other_clients[my_id-1],"\n",1);
                            write(other_clients[my_id-1],unames[group_members[group_id-1][i]-1],strlen(unames[group_members[group_id-1][i]-1]));
                            
                        }
                    }
                    write(other_clients[my_id-1],"\0",1);
                }
                else if(buff[2]=='c'){
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                        continue;
                    }
					printf("%s changed the group %s to %s\n",names[my_id-1],names[receiver_id-1]+1,buff+4);
                        
					int j;
					for(j=2;j< group_members[group_id-1][0] + 2;j++){
						
						if(other_clients[group_members[group_id-1][j]-1]==0)continue;	//if receiver left server
						if(group_members[group_id-1][j]==0)continue;					//if receiver left group
						if(names[group_members[group_id-1][j]-1][0]==':')continue;		//if receiver is a group in itself
						
						write(other_clients[group_members[group_id-1][j]-1],"-m ",strlen("-m "));
						write(other_clients[group_members[group_id-1][j]-1],names[my_id-1],strlen(names[my_id-1]));
						write(other_clients[group_members[group_id-1][j]-1]," changed the group ",strlen(" changed the group "));
						write(other_clients[group_members[group_id-1][j]-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
						write(other_clients[group_members[group_id-1][j]-1]," to ",strlen(" to "));
						write(other_clients[group_members[group_id-1][j]-1],buff+4,strlen(buff+4));
						write(other_clients[group_members[group_id-1][j]-1],".\0",strlen(".\0")+1);
						
						write(other_clients[group_members[group_id-1][j]-1],"-cn :",strlen("-cn :"));
						write(other_clients[group_members[group_id-1][j]-1],buff+4,strlen(buff+4));
						write(other_clients[group_members[group_id-1][j]-1]," -u ",strlen(" -u "));
						write(other_clients[group_members[group_id-1][j]-1],unames[receiver_id-1],strlen(unames[receiver_id-1]));
						write(other_clients[group_members[group_id-1][j]-1],"\0",1);
					}
					
					strcpy(names[receiver_id-1]+1,buff+4);
                    /*bool is_it_there=false;
                    int i;
                    for(i=0; i <100 && names[i]!=NULL; i++){
                        if(names[i][0]==':'){
                            if(strcmp(buff+4,names[i]+1)==0){
                                is_it_there=true;
                                break;
                            }
                        }
                    }
                    if(is_it_there){
                        if(receiver_id-1==i){
                            write(other_clients[my_id-1],"-m Already Same Name.\0",strlen("-m Already same Name.\0")+1);
                        }
                        else{
                            write(other_clients[my_id-1],"-m Try different name.\0",strlen("-m Try different name.\0")+1);
                        }
                    }
                    else{
                        printf("%s changed the group %s to %s\n",names[my_id-1],names[receiver_id-1]+1,buff+4);
                        
                        int j;
                        for(j=2;j< group_members[group_id-1][0] + 2;j++){
                            
                            if(group_members[group_id-1][j]==0)continue;
                            if(names[group_members[group_id-1][j]-1][0]==':')continue;
                            
                            write(other_clients[group_members[group_id-1][j]-1],"-m ",strlen("-m "));
                            write(other_clients[group_members[group_id-1][j]-1],names[my_id-1],strlen(names[my_id-1]));
                            write(other_clients[group_members[group_id-1][j]-1]," changed the group ",strlen(" changed the group "));
                            write(other_clients[group_members[group_id-1][j]-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                            write(other_clients[group_members[group_id-1][j]-1]," to ",strlen(" to "));
                            write(other_clients[group_members[group_id-1][j]-1],buff+4,strlen(buff+4));
                            write(other_clients[group_members[group_id-1][j]-1],".\0",strlen(".\0")+1);
                        }
                        
                        strcpy(names[receiver_id-1]+1,buff+4);
                    }*/
                }
                
                else{
                    bool is_it_there=false;
                    int i,j=0;
                    for(i=0;i<100 && unames[i]!=NULL;i++){
                        if(unames[i][0]==':'){
                            j++;
                            if(strcmp(buff+3,unames[i]+1)==0){
                                is_it_there=true;
                                break;
                            }
                        }
                    }
                    
                    if(!is_it_there){
                        write(other_clients[my_id-1],"-m No such group is Present.\0",strlen("-m No such group is Present.\0")+1);
                    }
                    else{
                        bool is_self_group=false;
                        int k;
                        for(k=2;k< group_members[j-1][0] + 2;k++){
                            if(group_members[j-1][k]==0)continue;
                            if(strcmp(unames[my_id-1],unames[group_members[j-1][k]-1])==0){
                                is_self_group=true;
                                break;
                            }
                        }
                        //printf("ok2\n");
                        if(!is_self_group){
                            write(other_clients[my_id-1],"-m You are not from this group.\0",strlen("-m You are not from this group.\0")+1);
                        }
                        else{
                            group_id=j;
                            receiver_id=i+1;
                            printf("%s is talking to %s\n",names[my_id-1],names[receiver_id-1]);
                        }
                    }
                }
                
            }
            else if(buff[1]=='c'){
                /*bool is_it_there=false;
                int i;
                for(i=0;i<100 && names[i]!=NULL;i++){
                    if(strcmp(buff+3,names[i])==0){
                        is_it_there=true;
                        break;
                    }
                }
                if(!is_it_there){
                    //printf("OK2\n");
                    printf("%s changed to %s\n",names[my_id-1],buff+3);
                    strcpy(names[my_id-1],buff+3);
                    write(other_clients[my_id-1],"-m -c p\0",strlen("-m -c p\0")+1);
                }
                else{
                    //printf("OK3\n");
                    write(other_clients[my_id-1],"-m -c n ",strlen("-m -c n "));
                    write(other_clients[my_id-1],names[my_id-1],strlen(names[my_id-1]));
                    write(other_clients[my_id-1],"\0",1);
                }*/
				printf("%s changed to %s\n",names[my_id-1],buff+3);
				strcpy(names[my_id-1],buff+3);
				write(other_clients[my_id-1],"-m -c p\0",strlen("-m -c p\0")+1);
				
				int j;
				for(j=0;names[j]!=NULL;j++){
					if(my_id-1!=j && clientsds[j]!=0 && clientsds[j]!=1 && names[j][0]!=0){
						write(clientsds[j],"-cn ",strlen("-cn "));
						write(clientsds[j],names[my_id-1],strlen(names[my_id-1]));
						write(clientsds[j]," -u ",strlen(" -u "));
						write(clientsds[j],unames[my_id-1],strlen(unames[my_id-1]));
						write(clientsds[j],"\0",1);
					}
				}
            }
            
            else if(buff[1]=='v'){
				
				if(receiver_id==0){
					write(other_clients[my_id-1],"-m Select receiver first\0",strlen("-m Select receiver first\0")+1);
					continue;
				}
				
                pthread_t main_call_thread_id;
				pthread_create(&main_call_thread_id,NULL,call_thread_main,NULL);
				
				write(other_clients[receiver_id-1],"-v ",strlen("-v "));
				write(other_clients[receiver_id-1],unames[my_id-1],strlen(unames[my_id-1])+1);
				
            }
            
        }
        else{
            
            if(receiver_id==0){
                write(other_clients[my_id-1],unames[my_id-1],strlen(unames[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[my_id-1],"-m Select receiver first\0",strlen("-m Select receiver first\0")+1);
            }
            else if(other_clients[receiver_id-1]==0){
                write(other_clients[my_id-1],unames[my_id-1],strlen(unames[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[my_id-1],"-m Receiver left the server.\0",strlen("-m Receiver left the server.\0")+1);
            }
            else if(other_clients[receiver_id-1]==1){
                int i;
                //printf("OK %d\n",group_members[group_id-1])
                for(i=2;i<group_members[group_id-1][0] + 2;i++){
                    
                    if(group_members[group_id-1][i]==0)continue;
                    if(other_clients[group_members[group_id-1][i]-1]==0)continue;
                    if(names[group_members[group_id-1][i]-1][0]==':')continue;
                    
                    write(other_clients[group_members[group_id-1][i]-1],unames[receiver_id-1],strlen(unames[receiver_id-1]));
                    write(other_clients[group_members[group_id-1][i]-1],": ",2);
                    write(other_clients[group_members[group_id-1][i]-1],unames[my_id-1],strlen(unames[my_id-1]));
                    write(other_clients[group_members[group_id-1][i]-1],": ",2);
                    write(other_clients[group_members[group_id-1][i]-1],buff,strlen(buff)+1);
                }
                
            }
            else{
                
                write(other_clients[my_id-1],unames[my_id-1],strlen(unames[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[receiver_id-1],unames[my_id-1],strlen(unames[my_id-1]));
                write(other_clients[receiver_id-1],": ",2);
                write(other_clients[receiver_id-1],buff,strlen(buff)+1);
            }
            
        }
    }
    
    return NULL;
}

int main(int argc,char** argv){
    signal(SIGINT,sig_handler);
    signal(SIGSEGV,sig_handler);
    
    /*static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    
    int pa_error;
    
    if (!(s_write = pa_simple_new(NULL, "VoiceChatter", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &pa_error))) {
        fprintf(stderr, __FILE__": pa_simple_new() for write failed: %s\n", pa_strerror(pa_error));
        return 34;
    }*/
    
	struct sockaddr_in server,client;
    //int sd;
    
    //int clientsds[100];
    
    //char** names;
    names=malloc(100*sizeof(char*));
	unames=malloc(100*sizeof(char*));
	passwords=malloc(100*sizeof(char*));
    
    //int** group_members;
    group_members=malloc(100*sizeof(int*));
    
    //pthread_t client_threads[100];
    
    unsigned clientLen;
	
	sd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    serverAddressString=argv[1];
	port_no = atoi(argv[2]);
    
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(port_no);

	bind(sd,(struct sockaddr *)&server,sizeof(server));
	listen(sd,100);
	
    printf("Server Address : %s at %d\n",serverAddressString,ntohs(server.sin_port));
    
    char buff[100];
    
    int i=0;
	while(1){
        clientLen=sizeof(client);
		int temp_sd = accept(sd,(struct sockaddr *)&client,&clientLen);
        
		while(i_lock);	//accept the socket then check for i_lock 
		i_lock=true;	//when released push the socket into array
		
		clientsds[i]= temp_sd;
		
        //debug statement
        //printf("Good.\n");
        
        read_line(clientsds[i],buff);
        
		int old_user_join_temp_i=-1;
        bool is_it_there=false;
        int j;
        for(j=0;j<100 && unames[j]!=NULL;j++){
            if(strcmp(buff,unames[j])==0){
                is_it_there=true;
                break;
            }
        }
        if(is_it_there){
			//read_line for password .. in buff
			read_line(clientsds[i],buff);
			if(clientsds[j]==0){
				// an old user is joining in.. check the password .. if true
				// store the current i in a temporary variable 
				// proceed with joining him... later swap back the i
				
				printf("ok1\n");
				if(strcmp(buff,passwords[j])==0){
					old_user_join_temp_i = i;
					clientsds[j]=clientsds[i];
					i=j;
					write(clientsds[i],"YEP\0",strlen("YEP\0")+1);	// Send (old user join) accepted
					read_line(clientsds[i],names[i]);
					
				}
				else{
					write(clientsds[i],"PNO\0",strlen("PNO\0")+1);	// Send password missmatch
					close(clientsds[i]);
					i_lock = false;
					continue;
				}
			}
			else{
				write(clientsds[i],"NOP\0",strlen("NOP\0")+1);		// Send username missmatch (meaning that such a user is already signed in)
				close(clientsds[i]);
				i_lock = false;
				continue;
			}
        }
        else{
            write(clientsds[i],"YEP\0",strlen("YEP\0")+1);			// Send(new user join) accepted
			
			unames[i]=malloc(50*sizeof(char));
			names[i]=malloc(50*sizeof(char));
			passwords[i]=malloc(50*sizeof(char));
        
			strcpy(unames[i],buff);
		
			read_line(clientsds[i],passwords[i]);
			read_line(clientsds[i],names[i]);
        }
        
    
		
        printf("%s joined the server, uname: %s.\n",names[i],unames[i]);
		
		//send name to everyone else
		for(j=0;names[j]!=NULL;j++){
			if(i!=j && clientsds[j]!=0 && clientsds[j]!=1 && names[i][0]!=0){
				write(clientsds[j],"-n ",strlen("-n "));
				write(clientsds[j],names[i],strlen(names[i]));
				write(clientsds[j]," -u ",strlen(" -u "));
				write(clientsds[j],unames[i],strlen(unames[i]));
				write(clientsds[j],"\0",1);
			}
		}
		
		//get everyone else's names
		for(j=0;names[j]!=NULL;j++){
			if(i!=j && clientsds[i]!=0 && clientsds[i]!=1 && names[j][0]!=0){
				if(names[j][0]!=':'){
					write(clientsds[i],"-n ",strlen("-n "));
					write(clientsds[i],names[j],strlen(names[j]));
					write(clientsds[i]," -u ",strlen(" -u "));
					write(clientsds[i],unames[j],strlen(unames[j]));
					write(clientsds[i],"\0",1);
				}
				else{
					// meaning it is a group
					// send only groups in which he is present
				}
			}
		}
		
		//sending the groups he was already in; seperately
		for(j=0;group_members[j]!=NULL;j++){
			int k;
			for(k=2;k<group_members[j][0]+2;k++){
				if(group_members[j][k]-1==i){
					write(clientsds[i],"-n ",strlen("-n "));
					write(clientsds[i],names[group_members[j][1]],strlen(names[group_members[j][1]]));
					write(clientsds[i]," -u ",strlen(" -u "));
					write(clientsds[i],unames[group_members[j][1]],strlen(unames[group_members[j][1]]));
					write(clientsds[i],"\0",1);
					break;
				}
			}
		}
		
        
        void* arg_dump[2];
        //arg_dump[0]=names;
        arg_dump[0]=(void*)((long)(i+1));
        //arg_dump[2]=clientsds;
        //arg_dump[3]=group_members;
        arg_dump[1]=&i;
        pthread_create(&client_threads[i],NULL,start_client_exec,arg_dump);
        
        //printf("%s joined the server.\n",);
		
		if(old_user_join_temp_i == -1){
			write(clientsds[i],"-m Hello\0",strlen("-m Hello\0")+1);
			i++;
		}
        else{
			write(clientsds[i],"-m Welcome Back.\0",strlen("-m Welcome Back.\0")+1);
			i = old_user_join_temp_i;
		}
		// remove the lock on i;
		i_lock = false;
	}
	close(sd);
	return 0;
}

