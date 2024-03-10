#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NAME_LEN 50
#define MESSAGE_LEN 150
static int sockfd;

//avverte il server che il client è uscito quando riceve un SIGINT
void interrompi(int sig){
    
    if (write(sockfd, "exit" ,4) < 0){
          printf("non puoi scrivere");
    }
    close(sockfd);
    exit(1);
    
}
//leggiamo i messaggi inviati dal server
void *leggi(void * arg){
    char buf[MESSAGE_LEN];
    int s = atoi((char *)arg);
    
    while(read(s, buf , MESSAGE_LEN)){
        //nel caso in cui il server avverta il client che non può connettersi terminiamo
        if (strcmp(buf, "Massimo di client connessi")==0){
           printf("%s .Prova più tardi\n", buf);
           kill(getpid() , SIGINT);
           return NULL;
        }
        //altrimenti stampiamo il messaggio
        printf("%s", buf); 
        bzero(buf, strlen(buf));
    }
     
    return NULL;
}

int main(int argc, char** argv){
    
    
    struct sockaddr_in server_addr;
    struct hostent *server;
    //nome del client
    char name[NAME_LEN];
    //messagio preso in input
    char buffer[MESSAGE_LEN];
    //messaggio inviato al client
    char message[MESSAGE_LEN];
    pthread_t tid;
    time_t ticks;
    
     //controlliamo che gli argomenti passati in input siano 2(host e porta)*/
    if (argc < 3){
       printf("Usa la porta %s\n" , argv[0]);
       return EXIT_FAILURE;
    }
    //creiamo una socket
    int port = atoi(argv[2]);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
       perror("errore in apertura socket");
       return 1;
    }
    
    //otteniamo le informazioni sul server con cui il client vuole connettersi e le passiamo alla struct server_addr
    server =gethostbyname(argv[1]);
    if (server == NULL){
       printf("errore host non trovato");
       return EXIT_FAILURE;
    }
    
    
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&server_addr.sin_addr.s_addr ,server->h_length);
    server_addr.sin_port = htons(port);
    
    //facciamo richiesta di connessione al server
    if ((connect(sockfd, (struct sockaddr *) &server_addr , sizeof(server_addr)) )< 0){
       perror("errore in connect");
       return 1;
    }
    
    bzero(name , NAME_LEN);
    bzero(buffer, MESSAGE_LEN);
    
    /*chiediamo al client di inserire il suo nome finchè non inserisce un stringa della lunghezza corretta e lo inviamo al server. così che al client possa essere associato un nome che
    gli altri client possono visualizzare*/
    do{
          printf("inserisci un nome\n");
          signal(SIGINT,interrompi);
          scanf("%s" , name);
          
          while( getchar() != '\n');
          
    }while(strlen(name) < 2  || strlen(name) >= NAME_LEN-1);
    
   
    if (write(sockfd, name , strlen(name)) < 0){
       perror("errore in scrittura");
    }
    
    
    //creiamo un nuovo thread che gestisca la lettura dei messaggi inviati dal server
    char str[2];
    sprintf(str," %i ",sockfd);
    if (pthread_create(&tid, NULL , &leggi, str) != 0)
        perror("errore creazione thread");
        
      
    printf("scrivi exit per uscire dalla chat\n");
    signal(SIGINT,interrompi);
    /*permettiamo al client di scrivere un messaggio in qualunque momento finchè non decide di uscire dalla chat scrivendo exit. Ogni volta che viene inviato un messaggio viene salvato
    in un file denominato loglocale_nomeclient.txt */
    while(strcmp(buffer, "exit")){
       
       fgets(buffer, MESSAGE_LEN,stdin);
       signal(SIGINT,interrompi);
       char nomefile[50] = "loglocale_";
       strncat(nomefile, name, strlen(name));
       strncat(nomefile, ".txt",4);
       FILE* fd= fopen(nomefile, "a");
       fputs(buffer,fd);
       fclose(fd);
       
       if (buffer[strlen(buffer)-1] == '\n') 
          buffer[strlen(buffer)-1] = '\0';
       
       /*otteniamo il timestamp di invio del messaggio, lo aggiungiamo al messaggio stesso e lo inviamo al server in modo che nel caso venga selezionata la modalità di distribuzione 
       secondo timestamp il server possa ordinare e distribuire i messaggi in base al tempo di invio*/
       ticks = time(NULL);
       sprintf(message, "%.24s %s",ctime(&ticks),buffer);
       
       if (write(sockfd, message ,strlen(message)) < 0){
          printf("non puoi scrivere");
          
          
       }
       
       
   
    }
    
    
    
   
}

