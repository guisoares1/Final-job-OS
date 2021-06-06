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

#define MEM_SZ 4096
#define MEMO_Sema sizeof(int)
#define BUFF_SZ MEM_SZ-sizeof(sem_t)-sizeof(int)
#define FIFOSIZE 10
//#define RAND_MAX 1000
struct fila{
   //sema variable
	sem_t mutex;
	int dados[FIFOSIZE];
	int nItens;
	int sinal;
	int totnum;
	int StopAllProcess;
//	int QtdP5;    
//	int Qtdp6;
 }; 
int vet[11000];
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
void sinal(int p);
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
struct fila *fila_shared;
struct fila *fila_shared2;
int main(){
	pid_t  pid, pid2, pid3, pid4,pid5, pid6, pid7; // processos 
	pthread_t thread1,thread2,thread3,thread4[2],thread5,thread6,thread7;        // Threads
	int i, nump5=0, nump6=0, cont;
	void *shared_memory = (void *)0;
	void *shared_memory2 = (void *)0;
	//int shmid, shmid1; // shared memory id
	
	// transformar em função para f1 e f2
	CreatSharedMemory(&shared_memory); 
	//printf("Memoria compartilhada no endereco=%p\n", shared_memory);
	fila_shared = (struct fila *) shared_memory; 
	CreatSharedMemory(&shared_memory2);
	//printf("Memoria compartilhada no endereco=%p\n", shared_memory2);  
	fila_shared2= (struct fila *) shared_memory2;
	// vector with aleatori numbers 1-1000
	prenumale(vet);
	criarFila( fila_shared);
	criarFila( fila_shared2);
	
	if ( pipe(canal1) == -1 ){ printf("Erro pipe()"); return -1; } // canal escrita fila2
	if ( pipe(canal2) == -1 ){ printf("Erro pipe()"); return -1; } // canal escrita fila2
    srand(5);
//	signal(SIGUSR1, sinal); // executar isso inicialmente no código
	for(int i=0;i<7;i++) // loop will run n times (n=5)
    {	if (i==0){
			if(fork() == 0)
			{
				//p1
				int cont=0;					
			//	printf("pid 1: %d\n",getpid());
				pthread_create(&thread1, NULL, addfila, NULL); 
				pthread_join(thread1, NULL); // finaliza	
				exit(0);
			}
		}
		else if(i==1){
			if(fork() == 0)
			{   //P2
			//	printf("pid 2: %d\n",getpid());
				pthread_create(&thread2, NULL, addfila, NULL); 
				pthread_join(thread2, NULL); // finaliza	
				exit(0);
			}
		}
		else if(i==2){
			if(fork() == 0)
			{
				//	p3 
			//	printf("pid 3: %d\n",getpid());
				pthread_create(&thread3, NULL, addfila, NULL); 
				pthread_join(thread3, NULL); // finaliza	
				exit(0);
			}

		}
		else if(i==3){
			if(fork() == 0)
			{
				// p4
			//	printf("pid 2: %d\n",getpid());
				for(cont=0;cont<2;cont++)
					if (cont==0)
						pthread_create(&thread4[cont], NULL, transferePorPipe, NULL);   // inicia e executa o thread criado
					else
						pthread_create(&thread4[cont], NULL, transferePorPipe2, NULL);   // inicia e executa o thread criado
				pthread_join(thread4[0], NULL); // finaliza	
				pthread_join(thread4[1], NULL); // finaliza	
				exit(0);
			}

		}
		else if(i==4){
			if(fork() == 0)
			{
				// p5
					pthread_create(&thread5, NULL, readp5, NULL); 
					pthread_join(thread5, NULL); // finaliza		
					exit(0);
			}
		}
		else if(i==5){
			if(fork() == 0)
			{
				// p6
					pthread_create(&thread6, NULL, readp6, NULL); 
					pthread_join(thread6, NULL); // finaliza	
					exit(0);
			}

		}
		else if(i==6){
			if(fork() == 0)
			{
				// p7	
				pthread_create(&thread7, NULL, result, NULL); 
				pthread_join(thread7, NULL); // finaliza	
				exit(0);
			}
		}
    }

	exit(0);
}
	//exit(0); 
void prenumale(int *vet){
	int cont=0,num=0;
	for(cont=0;cont<11000;cont++){
		num=rand();
		vet[cont]=(num=0? 1:num%1000);
	}
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

void sinal(int p)
{    
	fila_shared->sinal = 1;
}

void* addfila ( void *ptr){
	while(fila_shared2->StopAllProcess==1){
		
		sem_wait((sem_t*)&fila_shared->mutex);
 			if (fila_shared->sinal == 0){		
				 if (fila_shared->nItens<9){// wait clear the fifo
				//	printf("entrei dentro do add: %d\n",fila_shared->nItens);
					fila_shared->nItens=fila_shared->nItens+1;
					fila_shared->totnum++;
					fila_shared->dados[fila_shared->nItens] = vet[fila_shared->totnum]; // rand()
				} else{ 	
					fila_shared->sinal = 1;
				//	while(fila_shared->sinal == 0); // change before
				//	kill(getpid(),SIGUSR1);
				//	while(fila_shared->sinal == 1){};
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
	for(cont=0;cont<FIFOSIZE;cont++)
		f->dados[cont] = 0;
	f->nItens = -1; 
	f->sinal = 0;
    f->totnum = 0;
	f->StopAllProcess=1;
//	f->QtdP5=0;
//	f->Qtdp6=0;

}

void clearfifo( struct fila *f ){
	int cont=0;
	for(cont=0;cont<10;cont++)
		f->dados[cont] = 0;

	fila_shared->nItens = -1; 
	fila_shared->sinal = 0;
}

void* transferePorPipe( void *ptr){
	int cont=0, val=0; 
	//printf("pid 1: %d\n",getpid());
	while (fila_shared2->StopAllProcess==1){
        cont=0; 
		sem_wait((sem_t*)&fila_shared->mutex); // SEMA
			if (fila_shared->sinal == 1 && fila_shared->nItens>-1){
				val = fila_shared->dados[fila_shared->nItens]; // get the number of the end of row
				write(canal1[1],&val,sizeof(int));			   // writing on chanel, conection of p4 and p5		
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
	int cont=0, val=0; 
	//printf("pid 2: %d\n",getpid());
	while (fila_shared2->StopAllProcess==1){
        cont=0; 
		sem_wait((sem_t*)&fila_shared->mutex); // SEMA		
			if (fila_shared->sinal == 1 && fila_shared->nItens>-1){
			
				val = fila_shared->dados[fila_shared->nItens]; // get the number of the end of row
				write(canal2[1],&val,sizeof(int));			   // writing on chanel, conection of p4 and p6		
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
	int val=0;
	while(fila_shared2->StopAllProcess==1){	
	//	printf("aqui em baixo:");
		while ( fila_shared2->sinal != 0 ){ // Busy wait
			if(fila_shared2->StopAllProcess!=1)
				break; 
		} 
		read(canal1[0],&val,sizeof(int));
		if(val!=0){
		//	printf("fila 2%d, pid: %d \n",val, getpid());
			if ((fila_shared2->nItens<9)){
				fila_shared2->nItens=fila_shared2->nItens+1;
				fila_shared2->totnum=fila_shared2->totnum+1;
				fila_shared2->dados[fila_shared2->nItens] = val;
			//	fila_shared2->QtdP5++;
			}	
		}
		fila_shared2->sinal=1;
	}
	close(canal1[0]);
	pthread_exit(0); /* exit thread */

}

void* readp6( void *ptr ){
	int val=0;
	while(fila_shared2->StopAllProcess==1){	
		while ( fila_shared2->sinal != 1 ){ // Busy wait
			if(fila_shared2->StopAllProcess!=1) 
				break;
		} 
		read(canal2[0],&val,sizeof(int));
		if(val>0){
			if (fila_shared2->sinal==1){
				if ((fila_shared2->nItens<9)){
					fila_shared2->nItens=fila_shared2->nItens+1;
					fila_shared2->totnum=fila_shared2->totnum+1;
					fila_shared2->dados[fila_shared2->nItens] = val;
				//	fila_shared2->QtdP6++;
				}	
			}
		}
		fila_shared2->sinal=2;
	}
	close(canal2[0]);
	pthread_exit(0); /* exit thread */

}


void* result( void *ptr ){
	int cont=-1;
	while (fila_shared2->StopAllProcess==1){
		while ( fila_shared2->sinal != 2 ){} // Busy wait
		if(fila_shared2->nItens>-1){
			printf("numero: %d\n",fila_shared2->dados[fila_shared2->nItens]);
			fila_shared2->nItens = fila_shared2->nItens-1; 
		}
		fila_shared2->sinal=0;	
			if (fila_shared2->totnum==10000){
				fila_shared2->StopAllProcess=0;               // response to stop all process build 
				break;
			}	
	}
	printf("deu certo   %d\n",fila_shared2->totnum);
	//printf("deu certo  p5: %d, p6: %d\n",fila_shared2->QtdP5,fila_shared2->QtdP6);
	// relatório:
	pthread_exit(0); /* exit thread */

}