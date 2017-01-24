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
#include <iostream>
#include <cstdlib>


#define BUF_LEN 1200
#define FRAME_HEIGHT 240
#define FRAME_WIDTH 320
#define PACK_SIZE 1200//udp pack size; note that OSX limits < 8100 bytes

using namespace cv;
using namespace std;


//struct do socketow klientow w watkach
struct cln{
	int cfd;
	struct sockaddr_in caddr;
};
//liczba obslugiwanych klientow
int maxNumClients = 6;

//funkcja odczytujaca Integer od klienta  (problemy z interpretacją miedzy klientem JAVA a serwer w C)
int read_data_from(int fd, void *target, int size)
{
  int read_bytes = 0;

  while (size > 0)
  {
    int result = read(fd, (void *)((long)target + read_bytes), size);
    read_bytes += result;
    size -= result;
  }
  return read_bytes;
}


//funkcja obslugiwania wątku - nowego klienta
void* cthread(void *arg){

    int recvMsgSize = 0;
    char buffer[BUF_LEN];
    int total_pack = -1;

    //zmienne niezbedne do obsluzenia obrazu w OPENCV
    Mat rawData;
    Mat frame;

    int numberOfErrors = 0;

	struct cln* c = (struct cln*)arg;


	printf("Klient %d podlaczyl sie do serwera\n",c->cfd);

    ostringstream ss;
    ss << c->cfd;
    string windowName = ss.str();

    //tworzenie nowego okna do obrazu
    cvStartWindowThread();
    namedWindow(windowName.c_str(),WINDOW_AUTOSIZE);


	while(1){
        //odebranie liczby paczek które przyśle klient
        recvMsgSize = read_data_from(c->cfd, &total_pack, sizeof(int));
        //zamiana Integera na interpretację lokalną
        total_pack = ntohl(total_pack);

        //gdy liczba paczek przekracza 50 - nigdy nie wysyłamy aż tyle, naliczamy błąd i odłączamy klienta
        //klient po zakończeniu połączenia wysyła liczbę 55
        if ((total_pack <= 0) || (total_pack > 50)){
            numberOfErrors++;
            break;
        }
        //zmienna bufor, która kompletuje wszystkie małe bufory i składa 1 ramkę obrazu
        char * longbuf = new char[PACK_SIZE * total_pack];

        //czyszczenie bufora
        buffer[BUF_LEN] ='\0';


        //odbieranie pojedynczych paczek i kompletowanie jednej ramki - BUFOROWANIE
        for(int i = 0; i < total_pack; i++) {
            //odebaranie jednej z total_pack paczek do zlozenia ramki
            recvMsgSize = read(c->cfd, buffer, BUF_LEN);

            if(recvMsgSize > 0){
                if(recvMsgSize <= sizeof(int)){
                    numberOfErrors++;
                    break;
                }
                else if (recvMsgSize == PACK_SIZE) {
                    //kopiowanie zmiennej buffer w kolejne miejsce w zmiennej longbuf
                    memcpy(&longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
                }
                else
                {
                    cout<<"Niepoprawy rozmiar paczki "<< recvMsgSize << endl;
                    continue;
                }
            }
            else{
                numberOfErrors++;
                break;
            }
        }

        if(numberOfErrors == 0){

            //dekodowanie danych do postaci JPEG
            rawData = Mat(1,PACK_SIZE * total_pack, CV_8UC3, longbuf);
            frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);

            if (frame.size().width == 0) {
                cout<<"Nie udało się skompletować ramki"<<endl;
                continue;
            }
            //wyświetlenie ramki w okienku
            imshow(windowName, frame);

        }
        else{
            //wyrzucenie klienta z serwera
            break;
        }

        buffer[BUF_LEN] ='\0';

        delete[] longbuf;

	}

    //niszczenie okna klienta
    cvDestroyWindow(windowName.c_str());

    printf("Klient %d zostal odlaczony od serwera\n",c->cfd);

    //usuwanie zmiennych
	close(c->cfd);
	delete(c);

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	pthread_t tid;
	socklen_t slt;

	int sock = socket(PF_INET,SOCK_STREAM,0);

	struct sockaddr_in saddr;
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(6970);
	saddr.sin_addr.s_addr = INADDR_ANY;


	bind(sock,(struct sockaddr*)&saddr,sizeof(saddr));
	listen(sock,maxNumClients);

    printf(">>> Serwer monitoringu został uruchumiony. <<<\n");
    printf(">>> Oczekiwanie na klienta. <<<\n");

	while(1){
		struct cln* c = new struct cln[sizeof(struct cln)];
		slt = sizeof(c->caddr);
		c->cfd = accept(sock,(struct sockaddr*)&c->caddr,&slt);
		pthread_create(&tid,NULL,cthread,c);
		pthread_detach(tid);
	}

	printf("Serwer został wyłączony pomyślnie\n");

    cvDestroyAllWindows();

	return 0;
}
