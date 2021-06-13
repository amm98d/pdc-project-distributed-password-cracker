#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <crypt.h>
#include <unistd.h>
#include <math.h>

/*
def decimalToAlphabet(deci):
    alphabet, i, n = 0, 0, 0
    res = ""
    while(deci != 0):
        alpha = deci % 26
        print (alpha)
        print (chr(alpha+97))
        #alphabet = alphabet + alpha * pow(10, i)
        deci = deci//26
        #i += 1
        res = (chr(alpha+97)) + res
    print (res)

decimalToAlphabet(208827064575)
*/

char* decimalToAlphabet (int deci) {
	char *s = malloc(30);
	int i = 0;
	while (deci != 0) {
		int alpha = deci % 26;
		char ch = alpha + 97;
		deci = deci/26;
		s[i] = ch;
		i += 1;
	}
	s[i] = '\0';
	return s;
}

int main(int argc, char* argv[]){	

	//char id[] = "1234";
	//char salt[] = "$6$lKnEdTX3$";
	//char *encrypted = crypt(id, salt);
	//printf("%s\n",encrypted);

	int nprocs, rank;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	/*	Regarding MPI Send & Receive:				*/
	/*	===========================================	*/
	/*	array size communication on tag 		= 0 */
	/*	array itself communication on tag 		= 1 */
	/*	number to search communication on tag 	= 2 */
	/*	abort message communication on tag 		= 3 */

	// master
	if (rank == 0)
	{
		// clearing the console
		int sysRet = system("clear");
		if (sysRet == -1)
		{
			printf("System Method Failed.\n");
			return 0;
		}

		printf("PDC Project - Distributed Password Cracker\n");
		printf("============================================\n\n");

		// initializing variables
		// int count, i=0, toSearch;
		// char username[] = 	;
		// printf("Enter username: ");
		// if (scanf("%s" , username)){}

		// =============================================================================================
		// STEP-1: Take username input
		// =============================================================================================
		char username[30] = "ez";
		// printf("Please enter Username: ");
		// if (scanf("%s", username)){}
		printf("\nYour have entered %s.", username);
		int nameLen = (int)(strlen(username));
		printf(" It's length is %d.\n\n", nameLen);

		// =============================================================================================
		// STEP-2: Read /etc/shadow file
		// =============================================================================================

		//Reading file and extracting required line
		FILE *fp;
		char *line = NULL;
		size_t len = 0;
		ssize_t read;

		// Reading /etc/shadow
		// For now i have copied the content in a local file sample.txt
		// We can replace it with /etc/shadow path and run with sudo command to give the permission to read /etc/shadow file in the program
		fp = fopen("/etc/shadow", "r");
		if (fp == NULL)
		{
			printf("File not found\n");
			fclose(fp);
			MPI_Finalize();
			return 0;
		}

		while ((read = getline(&line, &len, fp)) != -1)
		{
			//comparing username
			int result = strncmp(username, line, nameLen);
			if (result == 0)
			{
				break;
			}
		}
		fclose(fp);

		// target hash and its length
		char *targetHash = malloc(98 * sizeof(char));
		memcpy(targetHash, line + nameLen + 1, 98 * sizeof(char));
		int targetHashLen = (int)strlen(targetHash);

		// =============================================================================================
		// STEP-3: Distribute workloads
		// =============================================================================================

		// [TODO]
		/*		1. Calculate total number of permutations*/
		int numDigits = 2, totalPermutations = pow(26, numDigits);
		printf("%d\n\n", totalPermutations);
		int iproc, chunkStart = 0, chunkLength = totalPermutations / (nprocs - 1), abort = 0;
		MPI_Status status;

		printf("Master: The hash to crack is: %s\n\n", targetHash);
		printf("%s\n\n", decimalToAlphabet(totalPermutations-1));

		// each process gets equal range i.e., (total permutations) / (#slaves)
		for (iproc = 1; iproc < nprocs; iproc++)
		{
			MPI_Send(&targetHashLen, 1, MPI_INT, iproc, 0, MPI_COMM_WORLD);			// targetHashLen sent with tag 0
			MPI_Send(targetHash, targetHashLen, MPI_INT, iproc, 1, MPI_COMM_WORLD); // targetHash sent with tag 1
			MPI_Send(&chunkStart, 1, MPI_INT, iproc, 2, MPI_COMM_WORLD);			// chunkStart sent with tag 2
			MPI_Send(&chunkLength, 1, MPI_INT, iproc, 3, MPI_COMM_WORLD);			// chunkLength sent with tag 3
			chunkStart += chunkLength;
		}

		// =============================================================================================
		// STEP-4: Sit tight and wait for good news
		// =============================================================================================

		/*		MPI_Recv(&abort, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);// abort message received with tag 3*/
		/*		printf("Master: Process %d has found the number!\n", status.MPI_SOURCE);*/
		/*		printf("Master: Informing all processes to abort!\n");*/
		/*		// sending abort message*/
		/*		for (iproc=1;iproc<nprocs;iproc++)*/
		/*			MPI_Send(&abort, 1, MPI_INT, iproc, 3, MPI_COMM_WORLD);// abort messages sent with tag 3*/
	}

	// slaves
	else
	{
		// initializing variables
		int chunkStart, chunkLength, abort = 0, targetHashLen;

		// receiving initial instructions from master
		MPI_Recv(&targetHashLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		   // receive targetHashLen with tag 0
		char *targetHash = malloc(targetHashLen * sizeof(char));							   // allocate memory according to targetHashLen
		MPI_Recv(targetHash, targetHashLen, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // receiving targetHash itself with tag 1
		MPI_Recv(&chunkStart, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);			   // receive chunkStart with tag 2
		MPI_Recv(&chunkLength, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		   // receive chunkLength with tag 3

		printf("Process %d:\n \
		\tHash to crack: %s\n \
		\tChunkStart: %d\n \
		\tChunkLength: %d\n\n",
			   rank, targetHash, chunkStart, chunkLength);

		// // receiving the number to search
		// MPI_Recv(&toSearch, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		// int i;
		// for (i=0;i<chunkLength;i++){
		// 	printf("%d ",arrayToSearch[i]);
		// }
		// printf("\n");

		// // using openmp to create 2 threads
		// #pragma omp parallel num_threads(2)
		// {
		// 	// thread-1: does the searching
		// 	if (omp_get_thread_num()==0){
		// 		int i = 0;
		// 		for (;i<chunkLength;i++){
		// 			// abandon rest of searching if abort message found
		// 			if (abort!=0){
		// 				printf("Process %d: Aborting Search!\n",rank);
		// 				break;
		// 			}
		// 			// if found: notify master
		// 			if (arrayToSearch[i]==toSearch){
		// 				printf("Process %d: I have found the number :-)\n",rank);
		// 				MPI_Send(&i, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);// abort message with tag 3
		// 				break;
		// 			}
		// 			// sleep for demonstration
		// 			if (rank!=1)
		// 				sleep(5);
		// 		}
		// 	}
		// 	// thread-2: waits to hear for abort message
		// 	else if (omp_get_thread_num()==1){
		// 		// waiting for abort message from master with tag 3
		// 		MPI_Recv(&abort, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// 	}
		// }
	}
	MPI_Finalize();
	return 0;
}
