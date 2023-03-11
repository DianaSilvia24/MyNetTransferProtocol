#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.cpp"
#include <iostream>

bool is_loggedin = false;

void auth(int sd){

    char* username, *parola;
    write(1,"Username: ",10);
    username=read_line(0);
    write(1,"Parola: ",8);
    parola=read_line(0);
    username[strlen(username)-1]=NULL;
    parola[strlen(parola)-1]=NULL;
    char* block=RC4(username,parola);
    write_block(sd,block,strlen(username));
    delete []block;
    delete []username;
    delete []parola;
    char* mesaj;
    mesaj=read_line(sd);
    if(strcmp("Logare cu succes!\n", mesaj) == 0){
        is_loggedin = true;
    }
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
}


void cd(int sd){
    write(1,"Path: ",6);
    char* path = read_line(0);
    write(sd,path,strlen(path));
    char* mesaj = read_line(sd);
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
    delete []path;
}

void pwd(int sd)
{
    char* mesaj = read_line(sd);
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
}

void ls(int sd){

    char* mesaj;
    int lungime_mesaj;
    while((mesaj=read_block(sd,lungime_mesaj))!= NULL)
    {
        write(1,mesaj,lungime_mesaj);
        delete []mesaj;
    }
}

void mkdir(int sd){
    write(1,"nume director nou: ",19);
    char* nume_director = read_line(0);
    write(sd,nume_director,strlen(nume_director));
    char* mesaj=read_line(sd);
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
    delete []nume_director;
}

void upload(int sd){

    char mesaj[256];
    int fd,lungime;
    unsigned long long citit_total=0;
    unsigned long long lungime_fisier;
    char block[4096];
    write(1,"Path: ",6);
    char *path = read_line(0);
    write(sd,path,strlen(path));
    path[strlen(path)-1] = NULL;
    fd=open(path,O_RDONLY);
    lungime_fisier = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while((lungime=read(fd,block,4096))>=1)
    {
        write_block(sd,block,lungime);
        citit_total+=lungime;
        snprintf(mesaj,256,"Au fost trimisi pana acum %d bytes (%3.2f%)\n",citit_total, (100.0*citit_total)/lungime_fisier);
        write(1,mesaj,strlen(mesaj));
    }
    write_block(sd,NULL,0);
    close(fd);
    char* mesaj_server = read_line(sd);
    write(1,mesaj_server,strlen(mesaj_server));
    delete []path;
    delete []mesaj_server;
}

void rm(int sd){
    write(1,"stergem: ",9);
    char* nume_director = read_line(0);
    write(sd,nume_director,strlen(nume_director));
    char* mesaj=read_line(sd);
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
    delete []nume_director;
}

void download(int sd){

    int fd;
    write(1,"Path: ",6);
    char* path = read_line(0);
    write(sd,path,strlen(path));
    char* mesaj;
    path[strlen(path)-1]=NULL;
    fd=creat(path,0777);
    char* block;
    int lungime;
    while((block=read_block(sd,lungime))!=NULL)
    {
        write(fd,block,lungime);
        delete []block;
    }
    close(fd);
    mesaj=read_line(sd);
    write(1,mesaj,strlen(mesaj));
    delete []mesaj;
    delete []path;

}

void local_cd(char* locatie){

    my_cd(1,locatie);

}

void local_pwd()
{
    my_pwd(1);

}

void local_ls()
{
    my_ls(1);
}

void local_mkdir(char* nume_nou){

    my_mkdir(1,nume_nou);

}

void local_rm(char* nume)
{
    my_rm(1,nume);
}

void quit(int sd){

}


int main(int argc, const char* argv[]){
    if(argc < 3){
        printf("Mod de utilizare: %s <adresa ip a serverului> <portul serverului>\n", argv[0]);
        return -1;
    }

    int sd,size;
    struct sockaddr_in server; /*structura conectare*/
    char *comanda;
    char *rezultat;

    if((sd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("Eroare la conectarea cu server-ul\n");
        return errno;

    }
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr(argv[1]);
    server.sin_port=htons(atoi(argv[2]));

    connect(sd,(struct sockaddr*) &server, sizeof(sockaddr_in));
    while(true){

        printf("comanda> ");
        fflush(stdout); 
        comanda=read_line(0); //citeste comanda de la tastatura de pe ecran


        if(strstr(comanda,"local_"))
        {

            if(strcmp(comanda,"local_cd\n")==0)
            {
                write(1,"Introduceti calea dorita: ",26);
                char* locatie = read_line(0);
                locatie[strlen(locatie)-1]=NULL;
                local_cd(locatie);
                delete []locatie;
            }

            if(strcmp(comanda,"local_mkdir\n")==0)
            {
                write(1,"Introduceti numele directorului nou: ",37);
                char* nume_nou = read_line(0);
                nume_nou[strlen(nume_nou)-1]=NULL;
                local_mkdir(nume_nou);
                delete []nume_nou;
            }

            if(strcmp(comanda,"local_pwd\n")==0)
            {
                local_pwd();
            }

            if(strcmp(comanda,"local_rm\n")==0)
            {
                write(1,"Introduceti numele entitatii sterse: ",37);
                char *nume_stergere = read_line(0);
                nume_stergere[strlen(nume_stergere)-1]=NULL;
                local_rm(nume_stergere);
                delete []nume_stergere;
            }

            if(strcmp(comanda,"local_ls\n")==0)
            {
                local_ls();
            }
        }
        else
        {
            if(strcmp(comanda,"quit\n")==0) //comanda quit care inchide atat server-ul cat si clientul
            {
                break;
                quit(sd);    
            }

            if(strstr(comanda,"auth\n")!=0 && !is_loggedin){
                write(sd,comanda,strlen(comanda));
                auth(sd);
                delete[] comanda;
                continue;
            }else{
                if(!is_loggedin){
                    write(1, "Nu esti logat, nu poti rula comenzi pe server!\n", 47);
                    delete[] comanda;
                    continue;
                }
            }

            write(sd,comanda,strlen(comanda));
                    
            if(strstr(comanda,"cd\n")!=0){
                cd(sd);
            }
        
            if(strstr(comanda,"ls\n")!=0){
                ls(sd);
            }

            if(strstr(comanda,"pwd\n")!=0){
                pwd(sd);
            }
                
        
            if(strstr(comanda,"rm\n")!=0){
                rm(sd);
            }
        
            if(strstr(comanda,"mkdir\n")!=0){
                mkdir(sd);
            }
        
            if(strstr(comanda,"upload\n")!=0){
                upload(sd);
            }
            
            if(strstr(comanda,"download\n")!=0){
                download(sd);
            }
        }
        delete[] comanda;
    }

    close(sd);
    return 0;
}