#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/cv.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
using namespace cv;

struct cln{
	int cfd;
	struct sockaddr_in caddr;
};
void* cthread(void *arg){

	struct cln* c = (struct cln*)arg;

	char accept[] = "monitoringEntry";

	printf("Klient %d podlaczyl sie do serwera\n",c->cfd);
	write(c->cfd, accept, sizeof(accept));

	namedWindow("CV Video Client",CV_WINDOW_AUTOSIZE);

	while(1){
		Mat image = Mat::zeros(480,640,CV_8UC3);
		int imgSize = image.total()*image.elemSize();
		unsigned char* data = image.data;
		char * data2 = reinterpret_cast<char*>(&data);

		char buffer[20];
		if(read(c->cfd,buffer,sizeof(buffer))){
			printf("CHUJOWO W CHUJ\n");
		}

	}

	printf("Klient %d odlaczyl sie od serwera\n",c->cfd);
	close(c->cfd);
	delete(c);

	return EXIT_SUCCESS;
}


int main(int argc, char** argv)
{
	pthread_t tid;
	socklen_t slt;
	int sfd,on=1;

	int sock = socket(PF_INET,SOCK_STREAM,0);

	struct sockaddr_in saddr;
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(6970);
	saddr.sin_addr.s_addr = INADDR_ANY;

	bind(sock,(struct sockaddr*)&saddr,sizeof(saddr));
	listen(sock,5);


	while(1){
		struct cln* c = new struct cln[sizeof(struct cln)];
		slt = sizeof(c->caddr);
        printf("Serwer monitoringu został uruchumiony. Oczekiwanie na klienta.\n");
		c->cfd = accept(sock,(struct sockaddr*)&c->caddr,&slt);
		pthread_create(&tid,NULL,cthread,c);
		pthread_detach(tid);
	}
	return 0;
}
