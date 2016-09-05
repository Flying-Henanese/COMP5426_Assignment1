#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 2048


int main(int argc, char** argv) {
	int myid;
    int numprocs;
	int result = 0;
	char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
	memset(buf, 0, sizeof(char)*BUF_SIZE);
	MPI_Status status;  
	MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank (MPI_COMM_WORLD, &myid);
	double time1,time2;//used to acquire the running time

	if(numprocs == 1){//single process 
		time1=MPI_Wtime();//Initial time of the program
		int len;
		len=buf_read(buf, buf+BUF_SIZE);//The length of integer that has been loaded
		while(len>0) {
			result += collapse(buf, len);
			result = (result - 1)%9 +1;
			len = buf_read(buf, buf+BUF_SIZE);//Load next part of the integer
		}
		printf("result: %d\n", result);
		time2=MPI_Wtime();//end time
	    printf("The time spent is %f seconds\n",time2-time1);

	} else if(myid == 0) {
		time1=MPI_Wtime();
		int Len = buf_read(buf, buf+BUF_SIZE);
		int allocatedJobs = 0;
		int i = 1;
		for(i=1; i<=numprocs-1; i++) {
			if(Len> 0) {
				MPI_Send(buf, Len, MPI_CHAR, i, 1, MPI_COMM_WORLD);//send data to slave process
				printf("Send %d digits to process %d\n",Len,i);
				allocatedJobs++;
				Len = buf_read(buf, buf+BUF_SIZE);//proceed with pointer
			} else {
				printf("No digit left, terminate process %d\n",i);
				MPI_Send(buf, 0, MPI_CHAR, i, 1, MPI_COMM_WORLD);//no digit left and terminate slave process
			}
		}

		char tempResult = 0;
		while(allocatedJobs) {//retrieve answers from slave process
			MPI_Recv(&tempResult, 1, MPI_CHAR, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
			printf("Retrieve result %d from process %d\n",tempResult,status.MPI_SOURCE);
			result += tempResult;
			allocatedJobs--;

			if(Len) {//some remaining digits haven't been proceed. offer rLen digits to the process which just return its answer to master
				MPI_Send(buf, Len, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
				printf("Send %d digits to process %d\n",Len,status.MPI_SOURCE);
				allocatedJobs++;
				Len = buf_read(buf, buf+BUF_SIZE);
			} else {
				MPI_Send(buf, 0, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD);//terminates the slave process
			    printf("No digit left, terminate process %d\n",status.MPI_SOURCE);
			}
		}

		Len = sprintf(buf, "%d", result);
		result = collapse(buf, Len);//find the ultimate collapse.
		time2=MPI_Wtime();
	    printf("The time spent is %f seconds\n",time2-time1);
		printf("result: %d\n", result);
		free(buf);
	} else {//slave process
		int RemLen = 0;
		char* RemBuf = (char*)malloc(sizeof(char)*BUF_SIZE);
		MPI_Recv(RemBuf, BUF_SIZE, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_CHAR, &RemLen);
		printf("Process %d Retrieve %d digits from Master\n",myid,RemLen);
		while(RemLen>0) {//if master still have digits not proceed
			char temp = collapse(RemBuf, RemLen);
			MPI_Send(&temp, 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
			MPI_Recv(RemBuf, BUF_SIZE, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);//wait for response from master
			MPI_Get_count(&status, MPI_CHAR, &RemLen);//rLen will be '0' if all works were finished.
		}
		printf("Process %d go to die\n",myid);
		free(RemBuf);
	}
	MPI_Finalize(); 
	return 0;
}

int collapse(char* buf, int len) {
	if(len == 0)
		return 0;
	else if(len == 1)
		return (*buf)-'0';
	else if(len==2){
        return ((buf[0]-'0')+(buf[1]-'0')-1)%9+1;
		}
	else {
		return (collapse(buf,len/2) + collapse(buf+len/2,len-len/2)-1)%9+1;
		}
	}


int buf_read(char* buf, const char * end) {
    int count;
    count = fread(buf, 1, end - buf, stdin);
    if (count > 0 && buf[count - 1] == '\n') {
        --count;
    }
    return count;
}
