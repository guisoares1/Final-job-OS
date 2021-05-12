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
//#define RAND_MAX 1000
struct fila{
   //sema variable
   sem_t mutex;
    
	int capacidade;
	int dados[10];
	int nItens;
	int sinal;
	int totnum;
 }; 
int vet[11000];
// chanel for pipe
int canal1[2],canal2[2]; 
// responsible for create the fifo
void criarFila(struct fila *f, int c );
// responsible for adding to the fifo
void* addfila (void *ptr); 
// function resposible to transfer info of fifo1 whith pipe to process 5 and 6
void* transferePorPipe( void *ptr);
// function resposible to send signal to p4
void rotina(int x);
// clear fifo
void clearfifo( struct fila *f );
// vetor de numeros aleatórios
void prenumale(int *vet);
// send inf to f2 using pipe
void* readTofifo2( struct fila *f2, int *canal, int busyWait, int *nump);
// response to show info of fifo2
void* result(struct fila *f2, int *nump5, int *nump6 );
struct fila *fila_shared;
struct fila *fila_shared2;

int main(){
	pid_t  pid, pid2, pid3, pid4,pid5, pid6, pid7; // processos 
	pthread_t thread1,thread2,thread3,thread4[2],thread5,thread6,thread7;        // Threads
	int i, nump5=0, nump6=0;
	key_t key=5678;
	void *shared_memory = (void *)0;
	int shmid, shmid1; // shared memory id
	
	// transformar em função para f1 e f2
	// Mostrando ao sistema a memória que será compatilhada
	shmid = shmget(key,MEM_SZ,0666|IPC_CREAT);
	if ( shmid == -1 )
	{
		printf("shmget falhou\n");
		exit(-1);
	}
	
	printf("shmid=%d\n",shmid);
	
	shared_memory = shmat(shmid,(void*)0,0);
	
	if (shared_memory == (void *) -1 )
	{
		printf("shmat falhou\n");
		exit(-1);
	}
		
	printf("Memoria compartilhada no endereco=%p\n", shared_memory);

	fila_shared = (struct fila *) shared_memory;
	fila_shared2= (struct fila *) shared_memory;
	// vector with aleatori numbers 1-1000
	prenumale(vet);
	criarFila( fila_shared, 10);
	criarFila( fila_shared2, 10);
	
	if ( pipe(canal1) == -1 ){ printf("Erro pipe()"); return -1; } // canal escrita fila2
	if ( pipe(canal2) == -1 ){ printf("Erro pipe()"); return -1; } // canal escrita fila2
    srand(5);
	
	pid=fork();				
					
	if ( pid > 0 )
	{ // p4
		int j=0;
		for(j=0;j<2;j++)
			pthread_create(&thread4[j], NULL, transferePorPipe( NULL), NULL); // inicia e executa o thread criado
	
		pthread_join(thread4[0], NULL); // finaliza	
		pthread_join(thread4[1], NULL); // finaliza	
		exit(0);
	}else if(pid==0){
		pid2=fork();
		
		if (pid2>0){
			//p1
			int cont=0;					
			pthread_create(&thread1, NULL, addfila( NULL ), NULL); 
			pthread_join(thread1, NULL); // finaliza	
			exit(0);}	
		}else if(pid2==0){
			pid3=fork();
			
			if(pid3>0){
				//	p2 
				pthread_create(&thread2, NULL, addfila( NULL ), NULL); 
				pthread_join(thread2, NULL); // finaliza	
				exit(0);	
			}else if (pid3==0){
				pid4=fork();
				
				if (pid4>0){
					//	p3 
					pthread_create(&thread3, NULL, addfila( NULL ), NULL); 
					pthread_join(thread3, NULL); // finaliza	
					exit(0);
				}else if (pid4==0){
					pid5=fork();
					
					if (pid5>0){
						// p5
						pthread_create(&thread5, NULL, readTofifo2( fila_shared2, canal1, 0, &nump5), NULL); 
						pthread_join(thread5, NULL); // finaliza		
						exit(0);
					}else if (pid5==0){
						pid6=fork();
						if (pid6>0){
							// p6
							pthread_create(&thread6, NULL, readTofifo2( fila_shared2, canal2, 1, &nump6), NULL); 
							pthread_join(thread6, NULL); // finaliza	
							exit(0);
						}else if (pid6==0){
							pid7=fork();
							if (pid7>0){
								// p7	
								pthread_create(&thread7, NULL, result(fila_shared2, &nump5, &nump6 ), NULL); 
								pthread_join(thread7, NULL); // finaliza	
								exit(0);
							}
						}
					}
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

void* addfila (  void *ptr){
	while(1){
		if (fila_shared->sinal == 0){	
			sem_wait((sem_t*)&fila_shared->mutex);
				printf("testestsetres");
				if (fila_shared->nItens<10){// wait clear the fifo
					fila_shared->dados[fila_shared->nItens+1] = vet[fila_shared->totnum]; // rand()
					fila_shared->nItens=fila_shared->nItens+1;
					fila_shared->totnum=fila_shared->totnum+1;
				} if(fila_shared->nItens==10){ 
					signal(SIGUSR1, rotina);
					}  // signal to p4
			sem_post((sem_t*)&fila_shared->mutex);
		}
	 }
	pthread_exit(0); /* exit thread */ 
}

void rotina(int x)
{    
	fila_shared->sinal = 1;
}

void criarFila( struct fila *f, int c) { 
	int cont=0;
	f->capacidade = c;
	sem_init(&f->mutex, 0, 1);
	for(cont=0;cont<10;cont++)
		f->dados[cont] = 0;
	f->nItens = -1; 
	f->sinal = 0;
    f->totnum = -1;

}

void clearfifo( struct fila *f ){
	int cont=0;
	for(cont=0;cont<10;cont++)
		f->dados[cont] = 0;
	f->nItens = -1; // used on p7
	f->sinal = 0;
}

void* transferePorPipe( void *ptr){
	int cont=0, val=0; 
	while (1){
        cont=0; 
		//printf("sinal: %d\n",f->sinal);
		if (fila_shared->sinal == 1){
			sem_wait((sem_t*)&fila_shared->mutex); // SEMA
				for(cont=0;cont<10;cont++){
					printf("%d\n",fila_shared->dados[cont]);
				}
				fila_shared->sinal=0;
				fila_shared->nItens=0;
				
				/*for(cont=0;cont<5;cont++){ 	 // canal 1
					val = fila_shared->dados[cont];
					write(canal1[1],&val,sizeof(int));
				}
				close(canal1[1]);
				for(cont=cont;cont<10;cont++){ // canal 2
					val = fila_shared->dados[cont];
					write(canal2[1],&val,sizeof(int));
					}
				close(canal2[1]);
				clearfifo( fila_shared ); // clear fifo 1 */
			sem_post((sem_t*)&fila_shared->mutex);
		}
					
	}
	pthread_exit(0); /* exit thread */
}

void* readTofifo2( struct fila *f2, int *canal, int wayt, int *num){
	int val=0;
	while(1){	
		sem_wait((sem_t*)&f2->mutex); // SEMA
			if ((f2->nItens<10)){
				read(canal[0],&val,sizeof(int));
				f2->dados[f2->totnum+1] = val;
				f2->nItens=f2->nItens+1;
				f2->totnum=f2->totnum+1;
				f2->sinal=(wayt=0? 1:0);
				num++;
			}
		sem_post((sem_t*)&f2->mutex);
	}
	close(canal[0]);
	pthread_exit(0); /* exit thread */

}

void* result(struct fila *f2, int *nump5, int *nump6){
	int cont=-1;
	while (1){
		sem_wait((sem_t*)&f2->mutex); // SEMA
			if(f2->nItens>-1){
				printf("numero: %d\n",f2->dados[f2->totnum]);
				f2->nItens = f2->nItens-1; 
				}
		sem_post((sem_t*)&f2->mutex);
	}
	pthread_exit(0); /* exit thread */

}
