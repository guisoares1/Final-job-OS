#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>    /* POSIX Threads */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <ctype.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define MEM_SZ 4096
#define MEMO_Sema sizeof(int)
#define BUFF_SZ MEM_SZ-sizeof(sem_t)-sizeof(int)
#define FIFOSIZE 10
//#define RAND_MAX 10000
struct fila{
   //sema variable
	sem_t mutex;
	int dados[FIFOSIZE];
	int nItens;
	int sinal;
	int totnum;
	int StopAllProcess;
	int qtdp5;    
	int qtdp6;
	clock_t timebegin;
 };
 struct filaRel{
	int dados[10000];	 
 };
// chanel for pipe
int canal1[2],canal2[2]; 
// responsible for create the fifo
void criarFila(struct fila *f);
// responsible for adding to the fifo
void* addfila (void *ptr); 
// function resposible to transfer info of fifo1 whith pipe to process 5 
void* transferePorPipe( void *ptr);
// function resposible to transfer info of fifo1 whith pipe to process 6
 void* transferePorPipe2( void *ptr);
// function resposible to send signal to p4
void sinal();
// clear fifo
void clearfifo( struct fila *f );
// vetor de numeros aleatórios
void prenumale(int *vet);
// send inf to f2 using pipe (p4 -> p5) 
void* readp5( void *ptr );
// send inf to f2 using pipe (p4 -> p6)
void* readp6( void *ptr );
// response to show info of fifo2
void* result( void *ptr );
// response to creat shared memory for fifos
void CreatSharedMemory(void ** shared_memory);
void contadorf2( struct fila *f, int pipe);
void changesinalf2( struct fila *f, int sinal);
struct fila *fila_shared;
struct fila *fila_shared2;
struct filaRel *fila_shared3;
void* result2( void *ptr );
void* result3( void *ptr );
void relp7();
int main(){
	clock_t timebegin, elapsedTime;
	pid_t  pid, pid2, pid3, pid4,pid5, pid6, pid7; // processos 
	pthread_t thread1,thread2,thread3,thread4,thread5,thread6,thread7[2];        // Threads
	int i, nump5=0, nump6=0, cont,val;
	void *shared_memory = (void *)0;
	void *shared_memory2 = (void *)0;
	void *shared_memory3 = (void *)0;
	double finaltime;
	//int shmid, shmid1; // shared memory id
	
	// transformar em função para f1 e f2
	CreatSharedMemory(&shared_memory); 
	//printf("Memoria compartilhada no endereco=%p\n", shared_memory);
	fila_shared = (struct fila *) shared_memory; 
	CreatSharedMemory(&shared_memory2);
	//printf("Memoria compartilhada no endereco=%p\n", shared_memory2);  
	fila_shared2= (struct fila *) shared_memory2;
	fila_shared2->timebegin=clock();
//	fila_shared3= (struct filaRel *) shared_memory3;
	criarFila( fila_shared);
	criarFila( fila_shared2);
	
	if ( pipe(canal1) == -1 ){ printf("Erro pipe()"); return -1; } // canal 1 escrita fila2
	if ( pipe(canal2) == -1 ){ printf("Erro pipe()"); return -1; } // canal 2 escrita fila2
    
    srand(5);
	signal(SIGUSR1, sinal); 
	for(int i=0;i<7;i++) // loop will run n times (n=5)
    {	if (i==0){
			if(fork() == 0)
			{
				//p1
				int cont=0;					
				addfila(NULL);
				exit(0);
			}
		}
		else if(i==1){
			if(fork() == 0)
			{   //P2
				addfila(NULL);	
				exit(0);
			}
		}
		else if(i==2){
			if(fork() == 0)
			{
				//	p3 
				addfila(NULL);	
				exit(0);
			}

		}
		else if(i==3){
			if(fork() == 0)
			{
				// p4
				pthread_create(&thread4, NULL, transferePorPipe2, NULL);   // inicia e executa o thread criado
				transferePorPipe(NULL);	
				pthread_join(thread4, NULL); // finaliza
				exit(0);
			}

		}
		else if(i==4){
			if(fork() == 0)
			{
				// p5
		 	 	readp5(NULL);	
				exit(0);   
			}
		}
		else if(i==5){
			if(fork() == 0)
			{
				// p6
			  	readp6(NULL); 	
				exit(0);   
			}

		}
		else if(i==6){
			if(fork() == 0)
			{
				// p7	
  		  	  	pthread_create(&thread7[0], NULL, result3, NULL);   // inicia e executa o thread criado
				pthread_create(&thread7[1], NULL, result2, NULL);   // inicia e executa o thread criado
				result(NULL);
				pthread_join(thread7[0], NULL); 
				pthread_join(thread7[1], NULL); 	 
				exit(0);    
			}
		}
    }
	//exit(0);
}
// Function that creat a shared memory
void CreatSharedMemory(void ** shared_memory)
{	int shmid;
	key_t key=5678;
	shmid = shmget(key,MEM_SZ,0666|IPC_CREAT);
	if ( shmid == -1 )
	{
		printf("shmget falhou\n");
		exit(-1);
	}
	
	printf("shmid=%d\n",shmid);
	
	*shared_memory = shmat(shmid,(void*)0,0);
	if (shared_memory == (void *) -1 )
	{
		printf("shmat falhou\n");
		exit(-1);
	}
}

void sinal()
{    
	fila_shared->sinal = 1;
}

void* addfila ( void *ptr){
	int num=0;
	while(fila_shared2->StopAllProcess==1){
		
		sem_wait((sem_t*)&fila_shared->mutex);
			 if (fila_shared->sinal == 0){		
				if (fila_shared->nItens<9){// wait clear the fifo
					fila_shared->nItens++;
					num=rand()%1000;
					fila_shared->dados[fila_shared->nItens] = (num==0? 1:num);
				} 
				if (fila_shared->nItens>8) { 	
					kill(getpid(),SIGUSR1);
				}   // signal to p4
			}
		sem_post((sem_t*)&fila_shared->mutex);
	 }
	pthread_exit(0); /* exit thread */ 
}

void criarFila( struct fila *f) { 
	int cont=0;
	if ( sem_init((sem_t *)&f->mutex,1,1) != 0 )
	{
			printf("sem_init falhou\n");
			exit(-1);
	}
	for(cont=0;cont<10;cont++)
		f->dados[cont] = 0;
	f->nItens = -1; 
	f->sinal = 0;
    f->totnum = 0;
	f->StopAllProcess=1;
	f->qtdp5=0;
	f->qtdp6=0;

}

void clearfifo( struct fila *f ){
	int cont=0;
	for(cont=0;cont<10;cont++)
		f->dados[cont] = 0;
	f->nItens = -1; 
	f->sinal = 0;
}

void* transferePorPipe( void *ptr){
	printf("pid 1: %d\n",getpid());
	while (fila_shared2->StopAllProcess==1){
		sem_wait((sem_t*)&fila_shared->mutex); // SEMA
			if (fila_shared->sinal == 1 && fila_shared->nItens <9){
				write(canal1[1],&fila_shared->dados[fila_shared->nItens],sizeof(int));			   // writing on chanel, conection of p4 and p5		
				fila_shared->nItens--;
				if(fila_shared->nItens==-1){
					clearfifo(fila_shared )	;	 
				}
			} 
		sem_post((sem_t*)&fila_shared->mutex);				
	}
	close(canal1[1]);
	pthread_exit(0); /* exit thread */

}

void* transferePorPipe2( void *ptr){
	while (fila_shared2->StopAllProcess==1){
		sem_wait((sem_t*)&fila_shared->mutex); // SEMA			
			if (fila_shared->sinal == 1 && fila_shared->nItens<9){
				write(canal2[1],&fila_shared->dados[fila_shared->nItens],sizeof(int));			   // writing on chanel, conection of p4 and p6		
				fila_shared->nItens--; 
				if(fila_shared->nItens==-1){
					clearfifo(fila_shared); 
				}
			}
		sem_post((sem_t*)&fila_shared->mutex);
					
	}
	close(canal2[1]);
	pthread_exit(0); /* exit thread*/ 
} 

void* readp5( void *ptr ){
	int val, n=0;
	while(fila_shared2->StopAllProcess==1){	
		while ( fila_shared2->sinal != 0 ){if(fila_shared2->StopAllProcess==0)exit(0);}
		fila_shared2->qtdp5++;
		if (fila_shared2->nItens<9) {
			fila_shared2->nItens++;
			n=read(canal1[0],&fila_shared2->dados[fila_shared2->nItens],sizeof(int));
			if (n==-1){
				printf("Erro!!");
				exit(0);
			}
		}
		changesinalf2(fila_shared2,1);
	}
	close(canal1[0]);
	pthread_exit(0); /* exit thread */
}

void* readp6( void *ptr ){
	int val,n=0;
	while(fila_shared2->StopAllProcess==1){	
		while ( fila_shared2->sinal != 1 ){if(fila_shared2->StopAllProcess==0)exit(0);}
		fila_shared2->qtdp6++;
		if (fila_shared2->nItens<9) {
			fila_shared2->nItens++;
			n=read(canal2[0],&fila_shared2->dados[fila_shared2->nItens],sizeof(int));
			if (n==-1){
				printf("Erro!!");
				exit(0);
			}
		}
		changesinalf2(fila_shared2,2);
	}
	close(canal2[0]);
	pthread_exit(0); /* exit thread */
}

void changesinalf2( struct fila *f, int sinal){
	f->sinal=sinal;
}

void* result( void *ptr ){
	int cont=-1;
	while (fila_shared2->StopAllProcess==1){
		while ( fila_shared2->sinal != 2 ){if(fila_shared2->StopAllProcess==0)exit(0);} // Busy wait
		if(fila_shared2->StopAllProcess==0)exit(0);
		if(fila_shared2->nItens>-1){
			printf("1 - numero: %d, %d\n",fila_shared2->dados[fila_shared2->nItens],fila_shared2->totnum);
			fflush(stdout);
			fila_shared2->nItens--; 
			fila_shared2->totnum++;
			if (fila_shared2->totnum==10000){
				relp7();
				fila_shared2->StopAllProcess=0;               // response to stop all process build 
			}
		}
		changesinalf2(fila_shared2,3);	
	}	
	pthread_exit(0); /* exit thread */
}

void* result2( void *ptr ){
	int cont=-1;
	while (fila_shared2->StopAllProcess==1){
		while ( fila_shared2->sinal != 3 ){if(fila_shared2->StopAllProcess==0)exit(0);} // Busy wait
		if(fila_shared2->StopAllProcess==0)exit(0);
		if(fila_shared2->nItens>-1){
			printf("2 - numero: %d, %d\n",fila_shared2->dados[fila_shared2->nItens],fila_shared2->totnum);
			fflush(stdout);
			fila_shared2->nItens--; 
			fila_shared2->totnum++;
			if (fila_shared2->totnum==10000){
				relp7();
				fila_shared2->StopAllProcess=0;               // response to stop all process build
			}
		}
		changesinalf2(fila_shared2,4);	
	}	
	pthread_exit(0); /* exit thread */
}

void* result3( void *ptr ){
	int cont=-1;
	while (fila_shared2->StopAllProcess==1){
		while ( fila_shared2->sinal != 4 ){if(fila_shared2->StopAllProcess==0)exit(0);} // Busy wait
		if(fila_shared2->StopAllProcess==0)exit(0);
		if(fila_shared2->nItens>-1){
			printf("3 - numero: %d, %d\n",fila_shared2->dados[fila_shared2->nItens],fila_shared2->totnum);
			fflush(stdout);
			fila_shared2->nItens--; 
			fila_shared2->totnum++;
			if (fila_shared2->totnum==10000){
				relp7();
				fila_shared2->StopAllProcess=0;               // response to stop all process build 
			}
		}
		changesinalf2(fila_shared2,0);	
	}	
	pthread_exit(0); /* exit thread */
}

void relp7(){
	printf("--------------------------------------\n");fflush(stdout);
	printf("Números impressos %d\n",fila_shared2->totnum);fflush(stdout);
//	printf("p5 %d,p6 %d\n",fila_shared2->qtdp5,fila_shared2->qtdp6);fflush(stdout);
	printf("Tempo %f  \n",(double)(clock()-fila_shared2->timebegin)/CLOCKS_PER_SEC);fflush(stdout);	
}
