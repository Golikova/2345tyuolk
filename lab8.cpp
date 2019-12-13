#include <iostream>
#include<stdio.h>
#include <cfloat>
#include<stdlib.h>
#include <time.h>
#include <string>
#include "mpi.h"
using namespace std;

#define MASTER 0

FILE *fp;
double **graph=NULL;
double *graphArray=NULL;
double *minDistance=NULL;
int *transitionals=NULL;
int *visited=NULL;
int num;
int	rankNum,     // айди процесса 
	numtasks,    // количество процессов
  	start,       // начальная вершина
  	size,        // размер процесса
	initNode=0,  //корневая вершина
	currentNode, //текущая вершина
	totalVisited=0;

int edgesNum;
int* sourceNode;
int* destNode;

char fileName[]="graphs/graph.txt";
char resultFile[] = "graphs/resultset.dat";

void initGraph() {

	graph = (double**)malloc(num*sizeof(double*));
	minDistance=(double*)malloc(num*sizeof(double));
	transitionals=(int *)malloc(num*sizeof(int));
	
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
		    minDistance[i]=DBL_MAX;
			transitionals[i]=i;
			if(rankNum==MASTER)
	    	visited[i]=0;
	}

  	//строим ребра
  	sourceNode =(int*)malloc(edgesNum*sizeof(int));
  	destNode =(int*)malloc(edgesNum*sizeof(int));

	for (int i = 0; i < edgesNum; i++)
  	{
  			sourceNode[i] = 0;
  			destNode[i] = 0;
  	}

	srand(time(0));

  	for (int i = 0; i < edgesNum; i++)
  	{
  		while (sourceNode[i] == destNode[i]){ 			
  			sourceNode[i] = rand()%num;
  			destNode[i] = rand()%num;
  		}  		
  	}

  	for (int i = 0; i < edgesNum; i++)
  	{
  		int edgeWeight = rand()%15;
  		graph[sourceNode[i]][destNode[i]] = edgeWeight;
  		graph[destNode[i]][sourceNode[i]] = edgeWeight;
  	}

}

void writeFile(int nodes, float edges, long double time) {
	if((fp = fopen (resultFile, "a" ))==NULL) {
 		printf("Не удалось прочитать файл\n");
 	}

 	string nodesStr = to_string(nodes);
 	string edgesStr = to_string(edges);
 	string timeStr = to_string(time);

 	fputs(nodesStr.c_str(), fp);
 	fputs("\t", fp);
 	fputs(edgesStr.c_str(), fp);
 	fputs("\t", fp);
 	fputs(timeStr.c_str(), fp);
 	fputs("\n", fp);

 	fclose(fp);

}

void readFile(char c[]) {
	
 	if((fp = fopen (c, "r" ))==NULL) {
 		printf("Не удалось прочитать файл\n");
 	}
	
 	fscanf(fp, "%d" ,&num);
    
 	graph = (double**)malloc(num*sizeof(double*));
  	minDistance=(double*)malloc(num*sizeof(double));
  	transitionals=(int *)malloc(num*sizeof(int));

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
		    minDistance[i]=DBL_MAX;
			transitionals[i]=i;
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

void calcTaskAmount() {

	int nmin, nleft, nnum;
	//определяем размер процесса
	nmin=num/numtasks;
	nleft=num%numtasks;
	int k=0;

	for (int i = 0; i < numtasks; i++) {
	    nnum = (i < nleft) ? nmin + 1 : nmin;
	    if(i==rankNum){
	       start=k;
	       size=nnum;
	      // printf ("Процесс№%2d  считает с вершины %2d до %2d \n", rankNum,start, start+size);
	    }
	  	k+=nnum;
	}

}

void printGraph(){

 	for(int i = 0; i < num; i++){
 		for (int j = 0; j < num; ++j){
      if(graph[i][j]!=DBL_MAX)
        printf("{%.0f}\t",graph[i][j]);
      else
        printf("{-}\t");
    }
    printf("\n");
 	}

}

void updateminDistance(){

	for(int i=start;i<start+size;i++){

		if(graph[currentNode][i]<DBL_MAX){
			if((graph[currentNode][i]+minDistance[currentNode])<minDistance[i]){
				minDistance[i]=graph[currentNode][i]+minDistance[currentNode];
				transitionals[i]=currentNode;
			}
		}
	}

}


void recieveDataFromSlave(){

	MPI_Status status;

  	int buffer[2];
  	for(int i=1;i<numtasks;i++){
    MPI_Recv(buffer,2,MPI_INT,i,0,MPI_COMM_WORLD,&status);
    int iStart=buffer[0];
    int iSize=buffer[1];
    MPI_Recv(&minDistance[iStart],iSize,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&status);
    MPI_Recv(&transitionals[iStart],iSize,MPI_INT,i,2,MPI_COMM_WORLD,&status);
  }

}

void sendDataToRoot(){

  int buffer[2];
  buffer[0]=start;
  buffer[1]=size;
  MPI_Send(buffer,2,MPI_INT,0,0,MPI_COMM_WORLD);
  MPI_Send(&minDistance[start],size,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
  MPI_Send(&transitionals[start],size,MPI_INT,0,2,MPI_COMM_WORLD);

}

int main(int argc, char *argv[]) {

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rankNum);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

	float part = 0;
	num = 2;
	edgesNum =2;

	while (num < 150) {

		part = 0.9;
		while ((edgesNum < (num*num)-num) &&
				(part > 0.01)) {

	//начать мерять время
	long double ti= MPI_Wtime();


	MPI_Barrier(MPI_COMM_WORLD);
	
	if (rankNum == MASTER)
	{
		/*cout << "Количество вершин: ";
		cin >> num;
		cout << "Количество ребер: ";
		cin >> edgesNum;*/
		initGraph();
	}

	MPI_Bcast(&num,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	graphArray = (double*)malloc((num*num)*sizeof(double));

	if (rankNum == MASTER) {
		//превращаем матрижу смежности графа в одномерный массив
		int ind=0;
		for (int i = 0; i < num; i++)
		{
			for (int j = 0; j < num; j++)
			{
				graphArray[ind] = graph[i][j];
				ind++;
			}
		}

		//printGraph();
	}

	MPI_Bcast(graphArray, num*num, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if (rankNum != MASTER) {
		graph = (double**)malloc(num*sizeof(double*));
	  	minDistance=(double*)malloc(num*sizeof(double));
	  	transitionals=(int *)malloc(num*sizeof(int));
		int ind=0;
		for (int i = 0; i < num; i++)
		{
			graph[i] = (double*)malloc(num*sizeof(double));
			minDistance[i]=DBL_MAX;
			transitionals[i]=i;
			for (int j = 0; j < num; j++)
			{
				graph[i][j] = graphArray[ind];
				ind++;
			}
		}
	}

	calcTaskAmount();
	transitionals[currentNode]=-1;
	minDistance[currentNode]=0;

	while(totalVisited<num){

		updateminDistance();

		if(rankNum==MASTER){
			recieveDataFromSlave();
			visited[currentNode]=1;
			totalVisited++;
		    double min=DBL_MAX;
		    int index=0;

		    for(int i=0;i<num;i++){
		      if(visited[i]!=1 && minDistance[i]<min){
		        min=minDistance[i];
		        index=i;
		      }
		    }

			currentNode=index;
		}
		else {
			sendDataToRoot();
		}

		MPI_Bcast(&currentNode,1,MPI_INT,0,MPI_COMM_WORLD);
		MPI_Bcast(minDistance,num,MPI_DOUBLE,0,MPI_COMM_WORLD);
		MPI_Bcast(transitionals,num,MPI_INT,0,MPI_COMM_WORLD);
		MPI_Bcast(&totalVisited,1,MPI_INT,0,MPI_COMM_WORLD);
	}

	//закончиить мерить время
	long double tf= MPI_Wtime();

	if(rankNum==MASTER){
/*		printf("Минимальные расстояния [");
		for(int i=0;i<num-1;i++)
			printf("%.2f, ",minDistance[i]==DBL_MAX ? -1:minDistance[i]);
		printf("%.2f]\n",minDistance[num-1]==DBL_MAX ? -1:minDistance[num-1]);

		printf("Промежуточных вершин [");
		for(int i=0;i<num-1;i++)
			printf("%d, ",transitionals[i]);
		printf("%d]\n",transitionals[num-1]);
*/
		cout << "Время выполнения: " << (tf-ti) << endl;


		writeFile(num,(1-part)*100, tf-ti);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	free(graphArray);
	free(graph);
	free(minDistance);
	free(transitionals);
	free(visited);
	free(sourceNode);
	free(destNode);

	part = part - 0.1;

	edgesNum = num/part;
	cout << "Ребер " << edgesNum << "Вершин: " << num << "Часть: " << part <<  endl;
	}
	part = 0.9;
	edgesNum = 2;
	num++;
}

	MPI_Finalize();
  return 0;
}





