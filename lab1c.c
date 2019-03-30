//executefilename tcp send ip port inputfile
//executefilename tcp recv ip port outputfile
//executefilename udp send ip port inputfile
//executefilename udp recv ip port outputfile
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#define BUFFSIZE 256
//finished
void error(char *mess){
	perror(mess);
	exit(1);
}
void tcpsend(int argc, char*argv[]){
	//server send
	//clinet receive
	int sockfd, newsockfd, n, protno;
	char buff[256];
	socklen_t  clilen;
	struct sockaddr_in serv_addr, clin_addr;
	//construct a socket	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		error("error on opening socket");
	}
	//initialize serv_addr
	bzero((char*)&serv_addr, sizeof (serv_addr));
	//port number
	protno = atoi (argv[4]);
	//ipv4
	serv_addr.sin_family = AF_INET; 
	//kernel determine ip
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	//transform to right endian (big endian)
	serv_addr.sin_port = htons(protno);
	//bind local thing to socket
	if(bind(sockfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)  )  < 0 ){
		error("error on binding");
	}
	//set listening and at most 5 client connect to server
	listen(sockfd, 5);
	//set client length
	clilen = sizeof(clin_addr);
	//accept the connection and build a newsocket
	newsockfd = accept(sockfd, (struct sockaddr*)&clin_addr, &clilen);
	if(newsockfd < 0){
		error("Error on acceptance");  
	}	
	//send and build a file
	FILE *in;
	int count = 0;
	int check=1, tmp;
	in = fopen(argv[5],"rb");
	//get the file size
	struct stat s;
	stat(argv[5],&s);
	long  sz = s.st_size;
	//send the file size
	write(newsockfd, (char*)&sz, sizeof(sz));
	while(1){
		usleep(10);
		//initialize the buff
		bzero(buff, sizeof(buff));
		//read the file into the buff
		check = fread(buff, sizeof(char), 256, in);
		
		//if the pointer to the file end
		if(check == 0)break;
		//send the file content
		write(newsockfd, buff, check);
	}
	//file close
	fclose(in);
}
void tcprec(int argc, char *argv[]){

	int sockfd, n, protno;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buff[256];
	if(argc < 3){
		fprintf(stderr, "usage %s hostname port\n",argv[0]);
		exit(0);
	}
	//set the port number
	protno = atoi(argv[4]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//get the local ip
	server = gethostbyname(argv[3]);
	//initialize the serv_addr
	bzero((char*) &serv_addr,sizeof(serv_addr));
	//ipv4
	serv_addr.sin_family = AF_INET;
	//copy the server address to the serv_addr
	bcopy((char*)server->h_addr,(char*)& serv_addr.sin_addr.s_addr, server->h_length);
	//set port number
	serv_addr.sin_port = htons(protno);
	//connect to the server
	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		error("connection failed");
	}
	//build file
	FILE *out;
	int tmp = 0;
	long count = 0;
	time_t timep;
	out = fopen(argv[5], "ab");
	//set the file size
	long sz;
	read(sockfd, (char*)&sz, sizeof(sz));
	int i = 1;
	while(1){
		//initialize buff
		bzero(buff, sizeof(buff));
		//receive the package and give to the buff
		tmp = read(sockfd, buff, 256);
		//count the file size
		count += tmp;
		time(&timep);
		
		//print out the percetage and time
		for(; i <= 20;){
			if(i*5 <= count *100 / sz){
				printf("%d %%\n", i*5);
				printf("%s \n", asctime(gmtime(&timep)));
				
				i++;
			}
			else break;
		}	
		
		//write the buff to the file
		fwrite(buff,sizeof(char), tmp, out);
		if(tmp == 0)break;
	}
	printf("completed\n");
	fclose(out);
	close(sockfd);	
}
void udpsend(int argc, char* argv[]){
	
	char sendbuff[BUFFSIZE], recbuff[BUFFSIZE] = {0};
	
	struct sockaddr_in serv_addr, clin_addr;
	 
	int sockfd;
	int protno = atoi(argv[4]);
	//buile socket
	sockfd =  socket(PF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		error("socket failed");
	}

	bzero((char*)&clin_addr, sizeof(clin_addr));
	bzero((char*)&serv_addr, sizeof(serv_addr));
	
	clin_addr.sin_family = AF_INET;
	clin_addr.sin_port = htons(protno);
	//get the ip and trandfer to struct in_addr
	clin_addr.sin_addr.s_addr = inet_addr(argv[3]);
	
	//send
	FILE *in;
	in = fopen(argv[5], "rb");
	struct stat s;
	stat(argv[5],&s);
	long  sz[1]; 
	sz[0] = s.st_size;
	int check;
	
	//send the file size
	int sd  = sendto(sockfd, sz, sizeof(sz), 0,(struct sockaddr*)&clin_addr, sizeof(clin_addr));
	if(sd < 0){
		printf("%d", sd);
		error("failed on sendto");
		exit(1);
	}

	while(1){
		memset(sendbuff, 0, sizeof(sendbuff));
		check = fread(sendbuff,1, BUFFSIZE, in);
		if(check == 0)break;
		//send the buff content to client address
		sd = sendto(sockfd, sendbuff, check, 0, (struct sockaddr*)&clin_addr, sizeof(clin_addr));
		if(sd < 0){
			printf("%d", sd);
			error("failed on sendto");
			exit(1);
		}
		//receive the data
		recvfrom(sockfd, recbuff, BUFFSIZE, 0, NULL, NULL);
		
	}
	fclose(in);
	close(sockfd);

}
void udprec(int argc, char *argv[]){


	socklen_t serlen, clilen;
	int sockfd, recfd;
	char sendbuff[BUFFSIZE], recbuff[BUFFSIZE];
	struct sockaddr_in serv_addr, client_addr;
	int portno = atoi(argv[4]);
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		error("socket failed");
		exit(1);
	}
	bzero((char*)&serv_addr, sizeof(serv_addr));
	bzero((char*)&client_addr, sizeof(client_addr));
	//build client fundamental thing
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr.sin_port = htons(portno);

	if(bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0){
		error("failed on binding");
		exit(1);
	}	 


	FILE *out;
	out = fopen(argv[5],"ab");
	time_t timep;
	int check, count = 0;
	long sz[1];
	int i = 1;
	serlen = sizeof(serv_addr);
	//get the file's size from server
	int re = recvfrom(sockfd, sz, sizeof(sz), 0, (struct sockaddr*)&serv_addr, &serlen);
	if(re < 0){
		error("failed on receive");
		exit(1);
	}
	

	while(1){
			memset(sendbuff, 0, sizeof(sendbuff));
			//get the package from server
			check = recvfrom(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr*)&serv_addr, &serlen);
			if(check > 0){
				count += check;
				fwrite(sendbuff, sizeof(char), check, out);
				sendto(sockfd, recbuff, check, 0, (struct sockaddr*)&serv_addr, serlen);
			}

			//print out the time and percentage
			time(&timep);
			for(; i <= 20;){
				if(i*5 <= count *100 / sz[0]){
					printf("%d %%\n", i*5);
					printf("%s \n", asctime(gmtime(&timep)));
					i++;
				}
				else break;
			}	
			if(count >= sz[0])break;
	}
	printf("completed\n");
	fclose(out);
	close(sockfd);
}



//client
int main(int argc, char *argv[]){

	if(!strcmp(argv[1],"tcp") && !strcmp(argv[2],"recv"))
		tcprec(argc, argv);
	if(!strcmp(argv[1],"tcp") && !strcmp(argv[2],"send"))
		tcpsend(argc, argv);
	if(!strcmp(argv[1],"udp") && !strcmp(argv[2],"send"))
		udpsend(argc, argv);
	if(!strcmp(argv[1], "udp") && !strcmp(argv[2], "recv"))
		udprec(argc, argv);
	

return 0;
}





