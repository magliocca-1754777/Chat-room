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
#include <time.h>

#define MAX_CLIENTS 4
#define NAME_LEN 50
#define MESSAGE_LEN 150


//numero client connessi
static _Atomic unsigned int clients_count = 0;
//numero messaggi in attesa
int messages_count = 0;
//id univoco del client
static int id = 1;

//struct che rappresenta un messaggio inviato dal client con timestamp
typedef struct{
    time_t ticks;
    char message[MESSAGE_LEN];
}message_t;

//struct che rappresenta un client
typedef struct{
    int client_fd;
    int client_id;
    char name[NAME_LEN];
} client_t;

//lista di client connessi
client_t* clients[MAX_CLIENTS];
//lista di messaggi in attesa di essere inviati (soltanto modalità timestamp)
message_t *lista[10];

//aggiunge un client alla lista dei client connessi
void addclient(client_t* c){

    for (int i = 0; i< MAX_CLIENTS; i++){
        if(!clients[i]){
           clients[i] = c;
           clients_count++;
           break;
        }
    }
}

//rimuove un client dalla lista dei client connessi
void removeclient(client_t* c){
     for (int i = 0; i< MAX_CLIENTS; i++){
         if (clients[i]){
            if (clients[i]->client_id == c->client_id){
               clients[i] = NULL;
               clients_count--;
               break;
            }
         }
     }
} 

//aggiunge un messaggio alla lista dei messaggi in attesa
void addlista(message_t* m, message_t *lista[10]){
      
      for (int i=0; i<messages_count+1;i++){
          if (!lista[i]){
              lista[i] = m;
              
              messages_count++;
              break;
          }
      }
     
} 

//rimuove un messaggio dalla lista dei messaggi in attesa
void removelista(message_t* m, message_t *lista[10]){
      
      for (int i=0; i<messages_count;i++){
          if (lista[i]){
              if(strcmp(lista[i]->message,m->message)==0){
                  lista[i]=NULL;
                  for (int j = i; j<messages_count-1;j++){
                          lista[j] = lista[j+1];
                  }
                  messages_count--;
                  break;
              }
          }
      }
     
} 

//ritorna il messaggio con timestamp minore che dovrà essere inviato
message_t * mindilista(message_t *lista[10]){
      message_t * min = (message_t *)malloc(sizeof(message_t));
      min = lista[0];
 
      for (int i=0; i<10;i++){
          if (lista[i]){ 
             if(difftime(min->ticks,lista[i]->ticks) >= 0){
                  min = lista[i];
                  
             }
          }
      }
      return min;
}

//invia un messaggio a tutti i clienti tranne l'autore  
void inviamessaggio(client_t * c, char* buf){
     //otteniamo l'id del client che ha inviato il messaggio
     char *ch = buf+ (strlen(buf)-1);
     int cid = atoi(ch);
     //e lo eliminiamo dal messaggio
     buf[strlen(buf)-1] = '\n';
     for (int i = 0; i < MAX_CLIENTS; i++){
         if (clients[i]){
            if(clients[i]->client_id != cid){
               
               
               if (write(clients[i]->client_fd, buf , strlen(buf)) < 0)
                   printf("errore in scrittura");
               
               
            }
         }
     }
     //aggiunge ogni messaggio inviato nella chat al file logglobale.txt
     FILE* f = fopen("logglobale.txt","a");
     fputs(buf,f);
     fclose(f);
     
     
}

//gestisce la distribuzione secondo la modalità timestamp
void *chattimestamp(void *c){
     //messaggio letto con timestamp
     char buffer[MESSAGE_LEN];
     //nome
     char name[NAME_LEN];
     //messaggio da inviare
     char *message;
     //client
     client_t *client= (client_t*)c;
     //tempo di invio
     time_t ticks;
     //struct contenente messaggio e timestamp
     message_t *messagetime;
     
     struct tm time;
     char temp[5];
     
     //legge il nome inviato dal client e avverte tutti gli altri client connessi che il client si è unito alla chat 
     if (read(client->client_fd, name, NAME_LEN) < 0){
              perror("errore in lettura");
     }
     if (strcmp(name, "exit")){
        strncpy(client->name ,name, strlen(name));
        sprintf(buffer ,"%s si è unito al gruppo %i", client->name,client->client_id);
     
        inviamessaggio(client , buffer);
        sprintf(buffer ,"%s si è unito al gruppo \n", client->name);
        printf("%s",buffer);
   
     
        bzero(name , strlen(name));
        bzero(buffer,MESSAGE_LEN);
     }
     
     
     //legge tutti i messaggi inviati dal client 
     while(read(client->client_fd, buffer , MESSAGE_LEN)){
        message = buffer + 25;
        //quando il client scrive exit avverte gli altri che il client si è disconesso
         if(strcmp(message, "exit")== 0 ||strcmp(buffer, "exit")== 0){
            //aggiungiamo al messaggio il nome e l'id del client che lo ha inviato per poi rimuoverlo prima di inviare il messaggio
           	sprintf(buffer ,"%s è uscito dal gruppo %i", client->name,client->client_id);
           	buffer[strlen(buffer)-1] = '\n';
           	printf("%s", buffer);
           	inviamessaggio(client , buffer);
           	break;
         }
        //utilizzando la stringa inviata dal client con il timestamp di invio ricaviamo tutte le informazioni per creare la struct tm
        
        //otteniamo il giorno della settimana
        if (strcmp(message, "exit")){
            strncpy(temp,buffer,3);
            switch(temp[0]){
     	    case 'M':
 		       time.tm_mday = 1;
 		       break;
     	    case 'T':
 		        if(temp[1]=='u'){
	 		        time.tm_mday = 2;
	 		        break;	
	 	        }else{
	 		        time.tm_mday = 4;
	 		        break;	
	 	        }
     	    case 'W':
 		       time.tm_mday = 3;
	 	       break;	
     	    case 'F':
 		       time.tm_mday = 5;
	 	       break;
     	    case 'S':
 		       if(temp[1]=='a'){
	 		        time.tm_mday = 6;
	 		        break;	
	 	       }else{
	 		        time.tm_mday = 0;
	 		        break;	
	 	       }
		    default:
		        printf("Errore giorno!!");
	        }
     	
            strncpy(temp,buffer+4,3);
            
            //otteniamo il mese 
            switch(temp[0]){
     	    case 'J':
 		         if(temp[1]=='a'){
 			           time.tm_mon=0;
 		         }else{ 
 			           if(temp[2]=='n'){
 				            time.tm_mon=5;
 			       }else{ 
 				            time.tm_mon=0;
 			       }
 		         }
 		         break;
     	    case 'F':
 		         time.tm_mon=1;
 		         break;
     	    case 'M':
 		         if(temp[2]=='r'){
 			           time.tm_mon=2;
 		         }else{
 			           time.tm_mon=4;
 		         }
 		         break;
     	    case 'A':
     	         if(temp[1]=='p'){
 			           time.tm_mon=3;
 		         }else{
 			           time.tm_mon=7;
 		         }
 		         break;	
     	    case 'S':
     	        time.tm_mon=8;
     	        break;
     	    case 'O':
     	        time.tm_mon=9;
     	        break;
     	    case 'N':
     	        time.tm_mon=10;
     	        break;
     	    case 'D':
     	        time.tm_mon=11;
     	        break;
     	    default:
		        printf("Errore mese!!");
	        }
	
            //otteniamo giorno, ora, minuti e secondi
            strncpy(temp,buffer+9,2);
            time.tm_mday= atoi(temp);
            strncpy(temp,buffer+12,2);
            time.tm_hour= atoi(temp);
            strncpy(temp,buffer+15,2);
            time.tm_min= atoi(temp);
            strncpy(temp,buffer+18,2);
            time.tm_sec= atoi(temp);
            strncpy(temp,buffer+21,4);
            time.tm_year= atoi(temp);
            
            //aggiungiamo al messaggio il timestamp di invio e l'id del client
            sprintf(buffer ,"%s : %s %i:%i:%i %i", client->name,message,time.tm_hour,time.tm_min,time.tm_sec,client->client_id);
            
            ticks = mktime(&time);
            //creiamo la struct contente messaggio e timestamp e la aggiungiamo alla lista dei messaggi in attesa
            messagetime=(message_t*)malloc(sizeof(message_t));
            messagetime->ticks = ticks;
            strncpy(messagetime->message,buffer,strlen(buffer));
            addlista(messagetime,lista);
            
            // aspettiamo 5 secondi prima di iniziare a svuotare la lista dei messaggi
            sleep(5);
            while (messages_count != 0){
                
                messagetime = mindilista(lista);
                messagetime->message[strlen(client->name)+strlen(message) +11] = '\0';
                inviamessaggio(client,messagetime->message);
                
                messagetime->message[strlen(client->name)+strlen(message) +10] = '\n';
                
                printf("%s",messagetime->message);
                removelista(messagetime,lista);
                
            }
            //inizializziamo la lista a NULL così che possa essere riusata
            for (int i = 0; i< 10; i++){
                lista[i] = NULL;
            }
            
         }
         
         bzero(buffer,MESSAGE_LEN);
         bzero(message,MESSAGE_LEN);
     }
     //chiudiamo la connessione del client e lo rimuoviamo dalla lista dei client connessi
     close(client->client_fd);
     removeclient(client);
     pthread_detach(pthread_self());
     return NULL;
     
}

//gestisce la distribuzione secondo la modalità fifo
void *chatfifo(void * c){
     //messaggio ricevuto
     char buffer[MESSAGE_LEN];
     //nome client
     char name[NAME_LEN];
     //messaggio da inviare
     char message[MESSAGE_LEN];
     client_t *client= (client_t*)c;
     //tempo di ricezione
     time_t ticks;
     //stringa con messaggio e tempo di ricezione
     char messagetime[1025];
     struct tm *tic;
     
     //legge il nome inviato dal client e avverte tutti gli altri client connessi che il client si è unito alla chat 
     if (read(client->client_fd, name, NAME_LEN) < 0){
              perror("errore in lettura");
     }
     if (strcmp(name, "exit")){
        strncpy(client->name ,name, strlen(name));
        sprintf(buffer ,"%s si è unito al gruppo %i", client->name,client->client_id);
        inviamessaggio(client , buffer);
        sprintf(buffer ,"%s si è unito al gruppo \n", client->name);
        printf("%s",buffer);
     
     
        bzero(name , strlen(name));
        bzero(buffer,MESSAGE_LEN);
        bzero(message,MESSAGE_LEN);
     }
     
     //legge tutti i messaggi inviati dal client 
     while(read(client->client_fd, buffer , MESSAGE_LEN)){
           
           //quando il client scrive exit o manda un SIGINT avverte gli altri che il client si è disconesso
           if (strcmp(buffer, "exit")==0 || strcmp(buffer+25, "exit") == 0){
           	    //aggiungiamo al messaggio l'id del client che lo ha inviato per poi rimuoverlo prima di inviare il messaggio
           	    sprintf(buffer ,"%s è uscito dal gruppo %i", client->name,client->client_id);
           	    inviamessaggio(client , buffer);
           	    buffer[strlen(buffer)-1] = '\n';
           	    printf("%s", buffer);
           	    break;
           }
           if (strcmp(buffer+25, "exit")){
                //prendiamo il tempo di ricezione del messaggio inviato dal client e lo aggiungiamo al messaggio stesso
                ticks = time(NULL);
                tic = localtime(&ticks);
                snprintf(messagetime, sizeof(messagetime), "%i:%i:%i"  ,tic->tm_hour ,tic->tm_min , tic->tm_sec);
                
                //aggiungiamo al messaggio l'id del client che lo ha inviato per poi rimuoverlo prima di inviare il messaggio
           	    sprintf(message,"%s : %s %s %i",client->name,buffer+25,messagetime,client->client_id);
           	    inviamessaggio(client ,message);
           	    message[strlen(message)-1] = '\n';
           	    printf("%s", message);
           	}
           	
           	
            bzero(buffer,MESSAGE_LEN);
           	bzero(message,MESSAGE_LEN);
     }
     //chiudiamo la connessione del client e lo rimuoviamo dalla lista dei client connessi
     close(client->client_fd);
     removeclient(client);
     pthread_detach(pthread_self());
     return NULL;
}


int main(int argc, char ** argv){
    //controlliamo che venga passa in input un argomento(la porta)
    if (argc != 2){
       printf("Usa la porta %s\n" , argv[0]);
       return EXIT_FAILURE;
    }
    
    int port = atoi(argv[1]);
    int serv_fd, cli_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_size;
    client_t *client;
    pthread_t tid;
    //messaggio di avvertimento per il limite di client connessi
    char erroreconn[MESSAGE_LEN];
    //modalità di distribuzione
    char mode[15];
    
    //creiamo la socket
    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd == -1){
       perror("errore in apertura socket");
       return 1;
    }
    
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    //associamo un nome alla socket creata
    if (bind(serv_fd , (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
       perror("in binding");
       return 1;
    }
    //mettiamo il server in ascolto
    if (listen(serv_fd, 10) < 0){
       printf("errore listen");
       return EXIT_FAILURE;
    }
    
    printf("BENVENUTO NELLA CHAT\n");
    client_size = sizeof(client_addr);
    
    //inseriamo da terminale la modalità di distribuzione
    do{
      printf("scegli la modalità di distrbuzione dei messaggi. timestamp o fifo?");
      scanf("%s", mode);
    }while(strcmp(mode, "fifo") && strcmp(mode,"timestamp"));
    
    while(1){
           //accettiamo la richiesta di connessione da parte di un client
           cli_fd = accept(serv_fd , (struct sockaddr*) &client_addr , &client_size);
           
           if (cli_fd < 0){
               printf("errore accettando connessione");
               continue;
           }
           //se sono già connessi MAX_CLIENTS client mandiamo un messaggio di avvertimento al client che sta cercando di connettersi e chiudiamo la connessione
           if ((clients_count ) == MAX_CLIENTS ){
              sprintf(erroreconn,"Massimo di client connessi");
              if (write(cli_fd, erroreconn , MESSAGE_LEN) <0) 
                  perror("errore in scrittura");
              close(cli_fd);
              continue;
           }
           //creiamo un nuovo client e lo aggiungiamo alla coda dei client connessi
           client = (client_t *)malloc(sizeof(client_t));
           client->client_fd = cli_fd;
           client->client_id = id++;
           
           addclient(client); 
           
           //a seconda della modalità di distribuzione scelta creiamo un diverso thread che la gestisca
           if (strcmp(mode, "fifo")== 0){
               
               if (pthread_create(&tid, NULL , &chatfifo, (void *)client) !=0)
                  perror("errore creazione thread");
           
           }
           else{
              
              if (pthread_create(&tid, NULL , &chattimestamp, (void *)client) !=0)
                  perror("errore creazione thread");
           }
    }
    return 0;           
           
}
