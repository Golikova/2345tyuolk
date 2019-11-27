#include <iostream>
#include<stdio.h>
#include <cfloat>
#include<stdlib.h>
#include <time.h>
#include "mpi.h"
using namespace std;

#define SEED 5
#define MASTER 0

FILE *fp;
double **graph=NULL;
double *minDist=NULL;
int *parents=NULL;
int *visited=NULL;
int num=0;
int	rankNum,    // айди процесса 
	numtasks,    // количество процессов
  start,       // начальная вершина
  size,        // размер процесса
	initNode=0,  //главный процесс
	actNode,
	totalVisited=0;

void loadFile(char c[]);
void printGraph();
void splitWork();
void updateMinDist();
void collectFromWorkers();
void reportToMaster();

int main(int argc, char *argv[]) {

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rankNum);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

	char c[]="graphs/bcs.txt";
	loadFile(c);
	MPI_Barrier(MPI_COMM_WORLD);
	splitWork();
	//начать мерять время
	double ti= MPI_Wtime();
	if(rankNum==MASTER){
 		printGraph();
		actNode=initNode;
	}
	//Broadcast actual Node
	MPI_Bcast(&actNode,	1,	MPI_INT, 0,	MPI_COMM_WORLD); 

	parents[actNode]=-1;
	minDist[actNode]=0;

	while(totalVisited<num){
		updateMinDist();

		if(rankNum==MASTER)
			collectFromWorkers();
		else
			reportToMaster();

		if(rankNum==MASTER){
			visited[actNode]=1;
			totalVisited++;
	    double min=DBL_MAX;
	    int index=0;
	    for(int i=0;i<num;i++){
	      if(visited[i]!=1 && minDist[i]<min){
	        min=minDist[i];
	        index=i;
	      }
	    }
			actNode=index;
		}

		//Broadcast actual Node
		MPI_Bcast(&actNode,1,MPI_INT,0,MPI_COMM_WORLD);
		//Broadcast arreglo de distancias minimas
		MPI_Bcast(minDist,num,MPI_DOUBLE,0,MPI_COMM_WORLD);
		//Broadcast arreglo de distancias minimas
		MPI_Bcast(parents,num,MPI_INT,0,MPI_COMM_WORLD);
		//Broadcast totalVisited
		MPI_Bcast(&totalVisited,1,MPI_INT,0,MPI_COMM_WORLD);

	}
	//Termina de medir el tiempo
	double tf= MPI_Wtime();
	if(rankNum==MASTER){
		//Imprime el arreglo de distancias minimas
		printf("Минимальные расстояния [");
		for(int i=0;i<num-1;i++)
			printf("%.2f, ",minDist[i]==DBL_MAX ? -1:minDist[i]);
		printf("%.2f]\n",minDist[num-1]==DBL_MAX ? -1:minDist[num-1]);

		//Imprim el arreglo de padres
		printf("Промежуточных вершин [");
		for(int i=0;i<num-1;i++)
			printf("%d, ",parents[i]);
		printf("%d]\n",parents[num-1]);

		//Imprime el tiempo de ejecucion
		printf("Время выполнения: %f\n",(tf-ti) );
	}

	MPI_Barrier(MPI_COMM_WORLD);
	free(graph);
	free(minDist);
	free(parents);
	free(visited);
	MPI_Finalize();
  return 0;
}

void loadFile(char c[]) {
	
 	if((fp = fopen (c, "r" ))==NULL) {
 		printf("Не удалось прочитать файл\n");
 	}
	
 	fscanf(fp, "%d" ,&num);
    
 	graph = (double**)malloc(num*sizeof(double*));
  	minDist=(double*)malloc(num*sizeof(double));
  	parents=(int *)malloc(num*sizeof(int));

	if(rankNum==MASTER)
  	visited=(int *)malloc(num*sizeof(int));

	// создаем матрицу графа
	for(int i=0;i<num;i++){
			graph[i] = (double*)malloc(num*sizeof(double));
			for(int j=0;j<num;j++){
	      		// инициализируем массив графа на бесконечностях
				graph[i][j]=DBL_MAX;
			}
		    // инициализируем вектор расстояний на бесконечностях
		    minDist[i]=DBL_MAX;
			parents[i]=i;
			if(rankNum==MASTER)
	    	visited[i]=0;
	}

	int a,b;
  	double d;
 	
 	while(feof(fp)==0){
 		fscanf(fp,"%d\t%d\t%lf",&a,&b,&d);
 		graph[a][b]=d;
    	graph[b][a]=d;
 	}

}

void splitWork() {

	int nmin, nmax, nnum;

	//определяем размер процесса
	nmin=num/numtasks;
	nmax=num%numtasks;
	int k=0;

	for (int i = 0; i < numtasks; i++) {
	    nnum = (i < nmax) ? nmin + 1 : nmin;

	    if(i==rankNum){
	       start=k;
	       size=nnum;
	       printf ("Процесс№%2d  начал с вершины %2d  размер =%2d \n", rankNum,start, size);
	    }

	  	k+=nnum;
	}

}

void printGraph(){
	//Imprimimos el grafo
 	for(int i = 0; i < num; i++){
 		for (int j = 0; j < num; ++j){
      if(graph[i][j]!=DBL_MAX)
        printf("%.2f\t",graph[i][j]);
      else
        printf("INF\t");
    }
    printf("\n");
 	}
}

void updateMinDist(){
	for(int i=start;i<start+size;i++){
		if(graph[actNode][i]<DBL_MAX){
			if((graph[actNode][i]+minDist[actNode])<minDist[i]){
				minDist[i]=graph[actNode][i]+minDist[actNode];
				parents[i]=actNode;
			}
		}
	}
}
void collectFromWorkers(){
	MPI_Status status;

  int buffer[2];
  for(int i=1;i<numtasks;i++){
    //Recibe parametros de los trabajadores
    MPI_Recv(buffer,2,MPI_INT,i,0,MPI_COMM_WORLD,&status);
    int iStart=buffer[0];
    int iSize=buffer[1];
    //Recibe la parte del arreglo de distancias minimas de cada trabajador
    MPI_Recv(&minDist[iStart],iSize,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&status);
		//Recibe la parte del arreglo de padres de cada trabajador
    MPI_Recv(&parents[iStart],iSize,MPI_INT,i,2,MPI_COMM_WORLD,&status);
  }
}
void reportToMaster(){
  int buffer[2];
  buffer[0]=start;
  buffer[1]=size;
  //Envia los parametros al proceso Maestro
  MPI_Send(buffer,2,MPI_INT,0,0,MPI_COMM_WORLD);
  //Envia su parte del arrgelo de distancias minimas al proceso Maestro
  MPI_Send(&minDist[start],size,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
	//Envia su parte del arrgelo de padres al proceso Maestro
  MPI_Send(&parents[start],size,MPI_INT,0,2,MPI_COMM_WORLD);
}