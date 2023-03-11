#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
using namespace std;

char* read_line(int fd) //citeste comanda
{
    char *s, c;
    int size=1;
    
    c='\0';
    s= new char[size]();

    while(c!='\n')
    {

        if(read(fd, &c,1)<1)
        {
        	c='\n';
        }
        if(strlen(s)>=size)
        {
            size=size*2;
            char *aux;
            aux=new char[size]();
            strcpy(aux, s);
            delete[] s;
            s=aux;
        }
        int i = strlen(s);
        s[i]=c;
        s[i+1]='\0';

    }
    return s;

}
char* read_block(int fd,int &lungime)
{
	read(fd,&lungime,4);
	if(lungime==0)
	{
		return NULL;		
	}

	char* mesaj= new char[lungime];
	read(fd,mesaj,lungime);

	return mesaj;

}

void write_block(int fd, char* s,int lungime)
{

	if(lungime==0)
	{
		write(fd,&lungime,4);
	}

	else 
	{
		write(fd,&lungime,4);
		write(fd,s,lungime);
	}
}

char* RC4(char *username,char* password)
{
	int k;
	unsigned char j, i, q;
	char* output = new char[strlen(username)];

	char S[256];
	for(k = 0; k<256; k++){
		S[k] = k;
	}

	j = 0;
	for(k = 0; k<256; k++){
		j = ( j + S[k] + password[k%strlen(password)] ) % 256;
		swap(S[k], S[j]);
	}

	i = 0;
	j = 0;
	for(k = 0; k<strlen(username); k++){
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		q = (S[i] + S[j]) % 256;
		swap(S[i], S[j]);
		output[k] = username[k] ^ S[q];
	}
	return output;

}
void my_cd(int fd, char* cale)
{
	chdir(cale);
	write(fd,"Ok\n",3);
}

void my_rm(int fd, char *filename)
{
	remove(filename);
	write(fd,"Ok\n",3);
}

void my_mkdir(int fd,char *path)
{
	mkdir(path,0777);
	write(fd,"Ok\n",3);

}

void my_pwd(int fd)
{
	char path[4098];
	getcwd(path,4096);
	strcat(path,"\n");
	write(fd,path,strlen(path));
}

void my_ls(int fd)
{
	DIR* director=opendir(".");
	dirent* ent_curent;
	struct stat* ent_info = new struct stat;

	while((ent_curent=readdir(director)) != NULL)
	{
		lstat(ent_curent->d_name,ent_info);
		char tip_ent[17];
		tip_ent[0] = NULL;

		if(S_ISREG(ent_info->st_mode))
		{
			strcpy(tip_ent,"fisier");
		}
		if(S_ISDIR(ent_info->st_mode))
		{
			strcpy(tip_ent,"director");
		}
		if(S_ISCHR(ent_info->st_mode))
		{
			strcpy(tip_ent,"character_device");
			
		}
		if(S_ISBLK(ent_info->st_mode))
		{
			strcpy(tip_ent,"block_device");
			
		}
		if(S_ISFIFO(ent_info->st_mode))
		{
			strcpy(tip_ent,"fifo");
			
		}
		if(S_ISLNK(ent_info->st_mode))
		{
			strcpy(tip_ent,"link");
			
		}
		if(S_ISSOCK(ent_info->st_mode))
		{
			strcpy(tip_ent,"socket");
			
		}

		char mesaj[4096];
		mesaj[0]=NULL;
		snprintf(mesaj,4096,"%s (%d) : %s \n",ent_curent->d_name,ent_info->st_size,tip_ent);
		if(fd == 1){
			write(fd, mesaj, strlen(mesaj));
		}else{
			write_block(fd,mesaj,strlen(mesaj));
		}
	}
	if(fd != 1){
		write_block(fd,NULL,0);
	}

}