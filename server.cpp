#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.cpp"
#include <signal.h>
#include <iostream>
#include <vector>

using namespace std;

struct user{
    char* username;
    char* parola;
    bool enabled=false;
};

vector<user> users;

void read_users()
{
    int fd=open("users",O_RDONLY);
    char* linie;
    do{
        linie=read_line(fd);
        if(strlen(linie)<=1)
        {
            break;
        }

        linie[strlen(linie)-1]=NULL;

        user u;
        u.parola=strchr(linie,':');
        u.username=linie;
        u.parola[0]='\0';
        u.parola++;
        users.push_back(u);


    }while(strlen(linie)>1);
    close(fd);

    fd=open("whitelist",O_RDONLY);
    do{
        linie=read_line(fd);
        if(strlen(linie)<=1)
            {
                break;
            }
        linie[strlen(linie)-1]=NULL;
        for(int i=0;i<users.size();i++)
        {
            if(strcmp(users[i].username,linie)==0)
            {
                users[i].enabled=true;
            }
        }

    }while(strlen(linie)>1);
    close(fd);

    fd=open("blacklist",O_RDONLY);
    do{
        linie=read_line(fd);
        if(strlen(linie)<=1)
            {
                break;
            }
        linie[strlen(linie)-1]=NULL;
        for(int i=0;i<users.size();i++)
        {
            if(strcmp(users[i].username,linie)==0)
            {
                users[i].enabled=false;
            }
        }

    }while(strlen(linie)>1);
    close(fd);
}
void sighandler(int sig)
{
    wait(NULL);
}

bool is_loggedin=false , keep_running=true, admin_priviledges=false;

void admin_auth(char* username,char*passwd)
{
    
    if((strcmp(username,"Daya")==0)&&(strcmp(passwd,"alune")==0))
    {
        admin_priviledges=true;
    }
    else
    {

        cout<<"No admin 4 u\n";
    }
}


void auth(int client){

    int lungime;
    char* block=read_block(client,lungime);
    for(int i=0;i<users.size();i++)
    {
        if(!users[i].enabled){
            continue;
        }
        if(lungime!=strlen(users[i].username))
        {
            continue;
        }

        char* tok=RC4(users[i].username,users[i].parola);
        if(memcmp(block,tok,lungime)==0){
            is_loggedin=true;
            write(client,"Logare cu succes!\n",18);
            break;
        }
    }
    if(!is_loggedin) 
    {
        write(client,"Username sau parola incorecta\n",30);
    }
}


void cd(int client){

    char* path = read_line(client);
    path[strlen(path)-1]='\0';
    my_cd(client,path);
    delete []path;
}

void pwd(int client)
{
    my_pwd(client);
}

void ls(int client){

    my_ls(client);
}

void mkdir(int client){

    char* nume_director= read_line(client);
    nume_director[strlen(nume_director)-1]='\0';
    my_mkdir(client,nume_director);
    delete []nume_director;

}

void upload(int client){
    
    int fd;
    char* path = read_line(client);
    path[strlen(path)-1]=NULL;
    fd=creat(path,0777);
    char* block;
    int lungime;
    while((block=read_block(client,lungime))!=NULL)
    {
        write(fd,block,lungime);
        delete []block;
    }

    close(fd);
    write(client,"Transfer realizat cu succes!\n",29);
    delete []path;
}

void rm(int client){

    char* nume_director= read_line(client);
    nume_director[strlen(nume_director)-1]='\0';    
    my_rm(client,nume_director);
    delete []nume_director;
}

void download(int client){

    int fd,lungime;
    char block[4096];
    char *path = read_line(client);
    path[strlen(path)-1] = NULL;
    fd=open(path,O_RDONLY);
    while((lungime=read(fd,block,4096))>=1)
    {
        write_block(client,block,lungime);
    }
    write_block(client,NULL,0);
    close(fd);
    write(client,"Transfer realizat cu succes!\n",29);
    delete []path;
}


void handle_client(int client)
{
    printf("Clientul s-a conectat\n");

    while(keep_running)
    {
        printf("Astept comanda\n");
        char* comanda=read_line(client); //citeste de pe fifo comenzile date de catre client
        if(strcmp(comanda,"quit\n")==0) //comanda quit care inchide atat server-ul cat si clientul
        {
            keep_running=false; //punct de iesire din while in caz de quit
            delete[] comanda;
            continue;
        }

        if(strstr(comanda,"auth\n")!=0 && !is_loggedin){
            auth(client);
            delete[] comanda;
            continue;
        }

        if(!is_loggedin){

            write(client,"Nu esti logat\n",14);
            delete[] comanda;
            continue;
        }

        if(strstr(comanda,"cd\n")!=0){
            cd(client);
        }
        if(strstr(comanda,"ls\n")!=0){
            ls(client);
        }
        if(strstr(comanda,"rm\n")!=0){
            rm(client);
        }
        if(strstr(comanda,"pwd\n")!=0){
            pwd(client);
        }
        if(strstr(comanda,"mkdir\n")!=0){
            mkdir(client);
        }
        if(strstr(comanda,"upload\n")!=0){
            upload(client);
        }
        
        if(strstr(comanda,"download\n")!=0){
            download(client);
        }

        delete[] comanda;

    }
    close(client);
    is_loggedin=false;

}


int main(int argc, const char* argv[]){
    if(argc < 2){
        printf("Mod de utilizare: %s <portul pe care se face bind>\n", argv[1]);
        return -1;
    }
    read_users();
    for(int i=0;i<users.size();i++)
    {
        printf("username: %s, passwd: %s, enabled: %s\n",users[i].username,users[i].parola, users[i].enabled ? "true" : "false" );
    }

    struct sockaddr_in server;    // structura folosita de server
    struct sockaddr_in from;  
    char *comanda;
    char *rezultat;
    int size,sd;

    if((sd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("Eroare la socket\n");
        return errno;
    }
    int enable = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(atoi(argv[1]));

    printf("Astept sa se conecteze clientul...\n");
    if((bind(sd,(struct sockaddr *)&server,sizeof(struct sockaddr)))==-1)
    {
        perror("Eroare la bind()\n");
        return errno;
    }

    if((listen(sd,5))==-1)
    {
        perror("Eroare la listen(), probabil prea multi deodata.\n");
        return errno;

    }

    signal(SIGUSR2,sighandler);

    while(1)
    {   
        int client;
        socklen_t length=sizeof(from);
        if((client=accept(sd,(struct sockaddr *)&from, &length))<0)
        {
            perror("Eroare la conectarea clientului\n");
            continue;
        }
        pid_t pid;
        if((pid=fork())<0)
        {
            perror("Eroare fork\n");
        }
        
        if(pid==0)
        {
            handle_client(client);
            kill(pid,SIGUSR2);
            exit(0);
        }
    }
    /*inchidere capete fifo*/
    close(sd);


    return 0;
}