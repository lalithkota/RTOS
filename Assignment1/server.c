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

int sd;
int clientsds[100];
char** names;
int** group_members;
pthread_t client_threads[100];

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

void* start_client_exec(void* dump){
    void** arg_dump=dump;
    //char** names=((char**)(arg_dump[0]));
    int my_id=(int)(arg_dump[0]);
    //int* other_clients=(int*)arg_dump[2];
    int *other_clients= clientsds;
    //int** group_members=((int**)(arg_dump[3]));
    int* main_thread_i=arg_dump[1];
    
    int receiver_id=0;
    int group_id=0;
    
    write(other_clients[my_id-1],"-m Hello\0",strlen("-m Hello\0")+1);
    
    char buff[1000];
    
    while(1){
        
        int temp_a = read_line(other_clients[my_id-1],buff);
        if(temp_a==1){
            other_clients[my_id-1]=0;
            printf("%s left the server.\n",names[my_id-1]);
            names[my_id-1][0]=0;
            
            return NULL;
        }
        if(buff[0]=='-'){
            
            write(other_clients[my_id-1],names[my_id-1],strlen(names[my_id-1]));
            write(other_clients[my_id-1],": ",2);
            write(other_clients[my_id-1],buff,strlen(buff)+1);
            
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
                    }
                }
                write(other_clients[my_id-1],"\0",1);
            }
            else if(buff[1]=='q'){
                close(other_clients[my_id-1]);
                other_clients[my_id-1]=0;
                printf("%s left the server.\n",names[my_id-1]);
                names[my_id-1][0]=0;
                
                return NULL;
            }
            else if(buff[1]=='g'){
                if(buff[2]=='n'){
                    bool is_it_there=false;
                    int i,j=0;
                    for(i=0;i<100 && names[i]!=NULL;i++){
                        if(names[i][0]==':'){
                            j++;
                            if(strcmp(buff+4,names[i]+1)==0){
                                is_it_there=true;
                                break;
                            }
                            
                        }
                    }
                    if(is_it_there){
                        write(other_clients[my_id-1],"-m Please try with a different name.\0",strlen("-m Please try with a different name.\0")+1);
                    }
                    else{
                        printf("New Group created: %s\n",buff+4);
                        names[i]=malloc(50*sizeof(char));
                        names[i][0]=':';
                        strcpy(names[i]+1,buff+4);
                        receiver_id=i+1;
                        group_id=j+1;
                        //printf("OK! %d\n",j);
                        
                        other_clients[receiver_id-1]=1;
                        (*main_thread_i)++;
                        
                        write(other_clients[my_id-1],"-m Group created : ",strlen("-m Group created : "));
                        write(other_clients[my_id-1],names[i]+1,strlen(names[i]+1));
                        write(other_clients[my_id-1],"\0",1);
                        group_members[group_id-1]=(int*)malloc(100*sizeof(int));
                        group_members[group_id-1][0]=1;
                        group_members[group_id-1][1]=my_id;
                        
                        //printf("%d\n",group_members[group_id-1][1]);
                        
                    }
                    
                }
                else if(buff[2]=='a'){
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                    }
                    else{
                        bool is_it_there=false;
                        int i;
                        for(i=0;i<100 && names[i]!=NULL;i++){
                            if(strcmp(buff+4,names[i])==0){
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
                            for(j=1;j<=group_members[group_id-1][0];j++){
                                if(group_members[group_id-1][j]==0)continue;
                                if(strcmp(buff+4,names[group_members[group_id-1][j]-1])==0){
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
                                write(other_clients[i],"-m ",strlen("-m "));
                                write(other_clients[i],names[my_id-1],strlen(names[my_id-1]));
                                write(other_clients[i]," added you to the group: ",strlen(" added you to the group: "));
                                write(other_clients[i],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                                write(other_clients[i],"\0",1);
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
                    for(i=1; i <= group_members[group_id-1][0] ; i++){
                        if(group_members[group_id-1][i]==0)continue;
                        if(strcmp(names[my_id-1],names[group_members[group_id-1][i]-1])==0){
                            group_members[group_id-1][i]=0;
                            break;
                        }
                    }
                    
                    
                    int j;
                    for(j=1;j<=group_members[group_id-1][0];j++){
                        
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
                    for(i=1; i <= group_members[group_id-1][0] ; i++){
                        //printf("OK %d\n",group_members[group_id-1][i]);
                        //printf("OK\n");
                        if(group_members[group_id-1][i]==0)continue;
                        if(names[group_members[group_id-1][i]-1][0]!='\0'){
                            write(other_clients[my_id-1],"\n",1);
                            write(other_clients[my_id-1],names[group_members[group_id-1][i]-1],strlen(names[group_members[group_id-1][i]-1]));
                            
                        }
                    }
                    write(other_clients[my_id-1],"\0",1);
                }
                else if(buff[2]=='c'){
                    if(receiver_id==0 || names[receiver_id-1][0]!= ':'){
                        write(other_clients[my_id-1],"-m Please select a group first.\0",strlen("-m Please select a group first.\0")+1);
                        continue;
                    }
                    bool is_it_there=false;
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
                        for(j=1;j<=group_members[group_id-1][0];j++){
                            
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
                    }
                }
                
                else{
                    bool is_it_there=false;
                    int i,j=0;
                    for(i=0;i<100 && names[i]!=NULL;i++){
                        if(names[i][0]==':'){
                            j++;
                            if(strcmp(buff+3,names[i]+1)==0){
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
                        for(k=1;k<=group_members[j-1][0];k++){
                            if(group_members[j-1][k]==0)continue;
                            if(strcmp(names[my_id-1],names[group_members[j-1][k]-1])==0){
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
                bool is_it_there=false;
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
                }
            }
        }
        else{
            
            if(receiver_id==0){
                write(other_clients[my_id-1],names[my_id-1],strlen(names[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[my_id-1],"-m Select receiver first\0",strlen("-m Select receiver first\0")+1);
            }
            else if(other_clients[receiver_id-1]==0){
                write(other_clients[my_id-1],names[my_id-1],strlen(names[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[my_id-1],"-m Receiver left the server.\0",strlen("-m Receiver left the server.\0")+1);
            }
            else if(other_clients[receiver_id-1]==1){
                int i;
                //printf("OK %d\n",group_members[group_id-1])
                for(i=1;i<=group_members[group_id-1][0];i++){
                    
                    if(group_members[group_id-1][i]==0)continue;
                    if(other_clients[group_members[group_id-1][i]-1]==0)continue;
                    if(names[group_members[group_id-1][i]-1][0]==':')continue;
                    
                    write(other_clients[group_members[group_id-1][i]-1],names[receiver_id-1]+1,strlen(names[receiver_id-1]+1));
                    write(other_clients[group_members[group_id-1][i]-1],": ",2);
                    write(other_clients[group_members[group_id-1][i]-1],names[my_id-1],strlen(names[my_id-1]));
                    write(other_clients[group_members[group_id-1][i]-1],": ",2);
                    write(other_clients[group_members[group_id-1][i]-1],buff,strlen(buff)+1);
                }
                
            }
            else{
                
                write(other_clients[my_id-1],names[my_id-1],strlen(names[my_id-1]));
                write(other_clients[my_id-1],": ",2);
                write(other_clients[my_id-1],buff,strlen(buff)+1);
                
                write(other_clients[receiver_id-1],names[my_id-1],strlen(names[my_id-1]));
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
    
	struct sockaddr_in server,client;
    //int sd;
    
    //int clientsds[100];
    
    //char** names;
    names=malloc(100*sizeof(char*));
    
    //int** group_members;
    group_members=malloc(100*sizeof(int*));
    
    //pthread_t client_threads[100];
    
    unsigned clientLen;
	
	sd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    char* serverAddressString=argv[1];
    
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr(serverAddressString);
	server.sin_port=htons(atoi(argv[2]));

	bind(sd,(struct sockaddr *)&server,sizeof(server));
	listen(sd,100);
	
    printf("Server Address : %s at %d\n",serverAddressString,ntohs(server.sin_port));
    
    char buff[100];
    
    int i=0;
	while(1){
        clientLen=sizeof(client);
		clientsds[i]=accept(sd,(struct sockaddr *)&client,&clientLen);
        
        //debug statement
        //printf("Good.\n");
        
        read_line(clientsds[i],buff);
        
        bool is_it_there=false;
        int j;
        for(j=0;j<100 && names[j]!=NULL;j++){
            if(strcmp(buff,names[j])==0){
                is_it_there=true;
                break;
            }
        }
        if(is_it_there){
            write(clientsds[i],"NOP\0",strlen("NOP\0")+1);
            close(clientsds[i]);
            continue;
        }
        else{
            write(clientsds[i],"YEP\0",strlen("YEP\0")+1);
        }
        
        names[i]=malloc(50*sizeof(char));
        
        strcpy(names[i],buff);
        printf("%s joined the server.\n",names[i]);
        
        void* arg_dump[2];
        //arg_dump[0]=names;
        arg_dump[0]=(void*)((long)(i+1));
        //arg_dump[2]=clientsds;
        //arg_dump[3]=group_members;
        arg_dump[1]=&i;
        pthread_create(&client_threads[i],NULL,start_client_exec,arg_dump);
        
        //printf("%s joined the server.\n",);
        i++;
	}
	close(sd);
	return 0;
}

