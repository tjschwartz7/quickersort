/********************************************************************************************************************************************************/
/*                                                                                                                                                      */
/* LAB 2 - QUICKERSORT - Written by Trenton Schwartz                                                                                                    */
/*                                                                                                                                                      */
/********************************************************************************************************************************************************/
/*This program depends on a file of billions of unsorted integers.                                                                                      */
/*The goal of this project is to sort these integers in some way specified by the user -                                                                */
/*That is, when running the program, the user selects from various flags which                                                                          */
/*specify details of the program's execution. I will specify these flags here:                                                                          */
/*1. SIZE - REQUIRED - (1,1000000000) - Specifies how many integers from the file will be sorted.                                                       */
/*2. ALTERNATE - OPTIONAL - (I/i/S/s) - DEFAULT = S - Specifies whether the alternate sorting algorithm used will be shellsort (S/s) or insertion (I/i).*/
/*3. THRESHOLD - OPTIONAL - (3, SIZE) - DEFAULT = 10 - Specifies the size a partition will be when it is sorted using an alternate algorithm.           */
/*4. SEED - OPTIONAL - Used in calculations when determining a data sample. Not specifying a seed will result in a random selection.                    */
/*5. MULTITHREAD - OPTIONAL - (Y/y/N/n) - DEFAULT = 'Y' -  Determines whether or not the program is multithreaded.                                      */
/*6. PIECES - OPTIONAL - (2, SIZE) - DEFAULT = 10 - Specifies how many partitions to divide our sample into. Ignored when MULTITHREAD is false.         */
/*7. MAXTHREADS - OPTIONAL - DEFAULT = 4 - Specifies the maximum number of threads to allow in the program. Ignored when MULTITHREAD is false.          */
/*8. MEDIAN - OPTIONAL - (Y/y/N/n) - DEFAULT = 'n' - Specify whether each segment will be partitioned using median-of-three.                            */
/*9. EARLY - OPTIONAL - (Y/y/N/n) - DEFAULT = 'n' - Specify whether we start a thread immediately after the first partitioning.                         */
/*These flags can be specified in any order by the user.                                                                                                */
/*After handling the flags, the first thing the program will do is load the numbers from the file into an array according to the flags.                 */
/*From here, it will sort these values also according to the flags, and display timing statistics to the user.                                          */
/********************************************************************************************************************************************************/

//Step 1: Process the flags -----------
//For each string in args
    //If it matches a flag pattern
        //If data is valid
            //Set appropriate flag data
        //Else
            //Throw error message and abort program
//Step 2: Load in integer data -----------
//If file random.dat does not exist
    //Execute MakeData program
//If SEED is greater than -1
    //Load file from SEED to SEED + SIZE
//Else
    //Call gettimeofday
    //Set var to microseconds
    //Seed SRAND with var to get a random variable x
    //Set x equal to x modulo 1 billion
    //Load file from x to x + SIZE
//Step 2: Init threads
//Create an array of pthread_t of size MAXTHREADS
//Create an array of pthread_attr_t of size MAXTHREADS
//Step 3: The Sorting (pseudocode for Quicksort itself not included)
//If MULTITHREAD is enabled
    //Call partition on the array PIECES - 1 times until you have PIECES partitions of hi and lo.
    //Sort these by partitions by hi - lo by ascending
//For each piece in PIECES
    //Call Quicksort (use thread if enabled)
        //If HI - LO < 2
            //Return
        //If HI - LO == 2
            //Sort values
            //Return
        //If HI - LO > 2 AND <= THRESHOLD
            //Apply alternate sorting algorithm
            //Return
        //If HI - LO > THRESHOLD
            //If MEDIAN is selected
                //Choose median value of left, right, center values
                //Swap median with the leftmost value
            //Set leftmost value in array to pivot
            //If multithreading enabled
                //Pivot larger segment
            //Else
                //Partition the left one

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define BILLION 1000000000


void Quicksort(int* partition, int lo, int hi);
static inline void PrintDebug(int* list, int size);
static inline void InsertionSort(int* partition, int lo, int hi);
static inline void Shellsort(int* partition, int lo, int hi);
bool isSorted(int* partition, int size);
static inline void InterpretFlags(int argc, char* argv[]);
//DEBUG
void PrintArray(int *partition, int size);
static inline void VerifyFlags();
void* ThreadedQuicksort(void* piece);
void* PollThreads(void * param);

int SIZE;
char ALTERNATE;
int THRESHOLD;
int SEED;
char MULTITHREAD;
int PIECES;
int MAXTHREADS;
char MEDIAN;
char EARLY;

u_int32_t* INTEGER_ARRAY;

struct pieces 
{
    int hi;
    int lo;
    int size;
};




int main(int argc, char* argv[], char* env[])
{
    pthread_t earlyTid;
    pthread_attr_t earlyAttr;

    struct timeval programStartTime;
    struct timeval programEndTime;
    gettimeofday(&programStartTime, NULL);
    struct timeval sortStartTime;
    struct timeval sortEndTime;
    int cpuCycles;
    double cpuTime;


    InterpretFlags(argc, argv);
    //VerifyFlags();

    int hi = SIZE - 1;
    int lo = 0;

    //SEED has been omitted
    if(SEED < 0)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srand(tv.tv_usec);
        SEED = rand() % BILLION; //SEED shall not exceed 1 billion.
    }


    //Load random.dat into array

    //Open the file
    FILE *fileptr = fopen("random.dat", "r");
    INTEGER_ARRAY = malloc(sizeof(u_int32_t) * SIZE);

    //Start reading file at memory address &fileptr[0] + SEED to SIZE
    //If SEED + SIZE exceeds 1 billion
    //Read in file from SEED to 1 billion then read from 0 to SIZE - (1 billion - SEED)

    struct timeval timeBeforeFileRead;
    struct timeval timeAfterFileRead;

    if(SEED + SIZE > BILLION)
    {
        gettimeofday(&timeBeforeFileRead, NULL);
        fseek(fileptr, SEED * sizeof(u_int32_t), SEEK_SET);
        int numberOfIntegersRead = fread(INTEGER_ARRAY, sizeof(u_int32_t), (BILLION - SEED), fileptr);
        fseek(fileptr, 0, SEEK_SET);
        int finalNumberOfIntegersRead = fread(&INTEGER_ARRAY[numberOfIntegersRead], sizeof(u_int32_t), (SIZE - numberOfIntegersRead), fileptr);
        gettimeofday(&timeAfterFileRead, NULL);
    }
    //Read in file from SEED to SEED + SIZE integers
    else
    {
        gettimeofday(&timeBeforeFileRead, NULL);
        fseek(fileptr, SEED * sizeof(u_int32_t), SEEK_SET);
        int numberOfIntegersRead = fread(INTEGER_ARRAY, sizeof(u_int32_t), SIZE, fileptr);
        gettimeofday(&timeAfterFileRead, NULL);
    }

    double loadTime =(timeAfterFileRead.tv_sec - timeBeforeFileRead.tv_sec)
                          +  ((timeAfterFileRead.tv_usec-timeBeforeFileRead.tv_usec) * pow(10, -6));

    gettimeofday(&sortStartTime, NULL);
    clock_t sortStartCpu = clock();

    //If MULTITHREAD is enabled
    //Separate array into PIECES pieces
    if(MULTITHREAD == 'Y' || MULTITHREAD == 'y')
    {

        if(PIECES > SIZE)
        {
            printf("Pieces is too large for the set size. Please try again.\n");
            abort();
        }
        //Create array of pieces for sorting purposes.
        //struct pieces init[PIECES];
        struct pieces pcs[PIECES];

        //This variable sticks to the current size of the array
        int currentPiecesSize = 0;

        //This variable sticks to the largest struct in the array
        int currentPartitionIndex = 0;

        //Before partitioning, check if EARLY is enabled
        if(EARLY == 'y' || EARLY == 'Y')
        {
    
            int locationsSize = 11;
            int locationsArray[] = {0, (int)(SIZE)*.1, (int)(SIZE)*.2, (int)(SIZE)*.3, (int)(SIZE)*.4, (int)(SIZE)*.5, (int)(SIZE)*.6, (int)(SIZE)*.7, (int)(SIZE)*.8, (int)(SIZE)*.9, SIZE-1};
            //We need to find the second smallest item in locationsArray
            //We make two passes - one  finds the smallest, second finds the second smallest.

            //Get the smallest item
            int smallestItem =  __INT_MAX__;
            int smallestIndex = -1;
            for(int i = 0; i < locationsSize; i++)
            {
                //Value in integer array is smaller than current smallest item
                if(INTEGER_ARRAY[locationsArray[i]] < smallestItem)
                {
                    smallestItem = INTEGER_ARRAY[locationsArray[i]];
                    smallestIndex = locationsArray[i];
                }
            }

            int secondSmallestItem = __INT_MAX__;
            int secondSmallestIndex = -1;
            for(int i = 0; i < locationsSize; i++)
            { 
                //Value in integer array is smaller than current second smallest item and is not the same value as the smallest index
                if(INTEGER_ARRAY[locationsArray[i]] < secondSmallestItem && locationsArray[i] != smallestIndex)
                {
                    secondSmallestItem = INTEGER_ARRAY[locationsArray[i]];
                    secondSmallestIndex = locationsArray[i];
                }
            } 

            //We now have the second smallest item and its index.
            {int temp = INTEGER_ARRAY[0]; INTEGER_ARRAY[0] = INTEGER_ARRAY[secondSmallestIndex]; INTEGER_ARRAY[secondSmallestIndex] = temp;} // swap values in [0] and [secondSmallestIndex]

            //Partition once
            int i=lo; 
            int j=hi+1; // Partition starts here: start at the two ends
            int X = INTEGER_ARRAY[lo];    // pick up the 26 – Pivot value in X
            do {
                do i++; while (INTEGER_ARRAY[i] < X && i < hi);
                do j--; while (INTEGER_ARRAY[j] > X && j > lo);
                if (i < j) {int temp = INTEGER_ARRAY[i]; INTEGER_ARRAY[i] = INTEGER_ARRAY[j]; INTEGER_ARRAY[j] = temp;} // swap values in [i] and [j]
                else       break;
            } while (1);
            {int temp = INTEGER_ARRAY[lo]; INTEGER_ARRAY[lo] = INTEGER_ARRAY[j]; INTEGER_ARRAY[j] = temp;} // swap values in [lo] and [j]

            //Segments: lo to j-1 and j+1 to hi

            int leftSegmentSize = (j-1) - lo + 1;
            int rightSegmentSize = (hi - (j+1)+1);

            //Now we want to start an EARLY thread on the smaller partition 
            //Keeping in mind, the optimal length of the partition is 25% of size

            //Sort the left side - its smaller
            if(leftSegmentSize < rightSegmentSize)
            {
                struct pieces theEarlyPiece;
                theEarlyPiece.hi = j-1;
                theEarlyPiece.lo = lo;
                theEarlyPiece.size = (j-1)-lo+1;
                printf("EARLY Launching %d to %d (%2.2f%%)\n", theEarlyPiece.lo, theEarlyPiece.hi, (theEarlyPiece.size / (double)SIZE) * 100);
                pthread_attr_init(&earlyAttr);
                pthread_create(&earlyTid, &earlyAttr, &ThreadedQuicksort, &theEarlyPiece);
                lo = j+1;
            }
            //Sort the right side - its smaller
            else
            {
                struct pieces theEarlyPiece;
                theEarlyPiece.hi = hi;
                theEarlyPiece.lo = j+1;
                theEarlyPiece.size = hi - (j+1)+1;
                printf("EARLY Launching %d to %d (%f%%)\n", theEarlyPiece.lo, theEarlyPiece.hi, (theEarlyPiece.size / (double)SIZE) * 100);
                pthread_attr_init(&earlyAttr);
                pthread_create(&earlyTid, &earlyAttr, &ThreadedQuicksort, &theEarlyPiece);
                hi = j - 1;

            }


            //We are now running the early thread
        }

        //Partition step

        //Partition array
        //While(currentSize < PIECES)
        //Select next largest segment to partition
        if(PIECES > 1)
            while(currentPiecesSize < PIECES)
            {
                printf("Partitioning %9d - %9d (%9d)...",lo, hi, ((hi - lo) +1));
                //if using median-of-three, apply it to put median in A[lo]
                if(MEDIAN == 'y' || MEDIAN == 'Y')
                {
                    int k = (hi - lo) / 2;
                    int median = 0;
                    int A = INTEGER_ARRAY[lo]; 
                    int B = INTEGER_ARRAY[k]; 
                    int C = INTEGER_ARRAY[hi];

                    if ((A >= B && A <= C) || (A >= C && A <= B))
                    {
                        median = lo; // i.e., A is between B & C (BAC or CAB)
                    }
                    else if ((B >= A && B <= C) || (B >= C && B <= A))
                    {
                        median = k; // B between A & C (ABC CBA)
                    }
                    else
                    {
                        median = hi;   // C is all that's left!
                    } 
                    {int temp = INTEGER_ARRAY[lo]; INTEGER_ARRAY[lo] = INTEGER_ARRAY[median]; INTEGER_ARRAY[median] = temp;} // swap values in [lo] and [median]
                }

                int i=lo; 
                int j=hi+1; // Partition starts here: start at the two ends
                int X = INTEGER_ARRAY[lo];    // pick up the 26 – Pivot value in X
                do {
                    do i++; while (INTEGER_ARRAY[i] < X && i < hi);
                    do j--; while (INTEGER_ARRAY[j] > X && j > lo);
                    if (i < j) {int temp = INTEGER_ARRAY[i]; INTEGER_ARRAY[i] = INTEGER_ARRAY[j]; INTEGER_ARRAY[j] = temp;} // swap values in [i] and [j]
                    else       break;
                } while (1);
                {int temp = INTEGER_ARRAY[lo]; INTEGER_ARRAY[lo] = INTEGER_ARRAY[j]; INTEGER_ARRAY[j] = temp;} // swap values in [lo] and [j]

                //Segments: lo to j-1 and j+1 to hi

                int leftSegmentSize = (j-1) - lo + 1;
                int rightSegmentSize = (hi - (j+1)+1);

                //Add both segments to pieces

                //Sort these two
                if(leftSegmentSize > rightSegmentSize)
                {
                    //Add right segment to pieces
                    pcs[currentPartitionIndex].hi = hi;
                    pcs[currentPartitionIndex].lo = j+1;
                    pcs[currentPartitionIndex++].size = rightSegmentSize;

                    //Add left segment to pieces
                    pcs[currentPartitionIndex].hi = j-1;
                    pcs[currentPartitionIndex].lo = lo;
                    pcs[currentPartitionIndex].size = leftSegmentSize;
                }
                else
                {
                    //Add left segment to pieces
                    pcs[currentPartitionIndex].hi = j-1;
                    pcs[currentPartitionIndex].lo = lo;
                    pcs[currentPartitionIndex++].size = leftSegmentSize;

                    //Add right segment to pieces
                    pcs[currentPartitionIndex].hi = hi;
                    pcs[currentPartitionIndex].lo = j+1;
                    pcs[currentPartitionIndex].size = rightSegmentSize;
                }
                currentPiecesSize = currentPartitionIndex + 1;

                //Otherwise, its already sorted
                if(currentPiecesSize > 2)
                {
                    //Sort the segments
                    for(int i = 0; i < currentPiecesSize; i++)
                    {
                        //This sorts the entire list from 0 to i
                        //Used decrement loop because - it might save an instruction in x86 and save clock cycles over increment loop
                        for(int j = currentPiecesSize - 1; j > 0; j--)
                        {
                            if(pcs[j].size < pcs[j-1].size) 
                            {
                                //Swap
                                int tempSize = pcs[j].size; 
                                int tempHi = pcs[j].hi;
                                int tempLo =  pcs[j].lo;

                                pcs[j].size = pcs[j-1].size;
                                pcs[j].hi = pcs[j-1].hi;
                                pcs[j].lo = pcs[j-1].lo;

                                pcs[j-1].size = tempSize;
                                pcs[j-1].hi = tempHi;
                                pcs[j-1].lo = tempLo;
                            }
                        }
                    }
                }

                //Next - partition the largest segment. Add the other segment to pieces.
                //This is located at pcs[currentPiecesSize-1]
                printf("result: %9d - %9d (%2.1f / %2.1f)\n", pcs[currentPartitionIndex].lo, pcs[currentPartitionIndex].hi, 
                ((j-1 - lo+1) / (double)(hi-lo+1)*100), ((hi - j+1+1) / (double)(hi-lo+1))*100);
                hi = pcs[currentPartitionIndex].hi;
                lo = pcs[currentPartitionIndex].lo;
                

                //PrintArray(array, SIZE);
            }
            else
            {
                pcs[0].hi = hi;
                pcs[0].lo = lo;
                pcs[0].size = hi - lo +1;
            }
        //Array has been partitioned

        //Print pieces information
        for(int i = 0; i < PIECES; i++)
        {
            printf("%d: %9d - %9d (%9d - %3.2f)\n", i, pcs[i].lo, pcs[i].hi, pcs[i].size, (pcs[i].size / (double)SIZE)*100);
        }

        //Create our thread arrays
        pthread_t *tidArray = malloc(sizeof(pthread_t) * MAXTHREADS);
        pthread_attr_t *attrArray = malloc(sizeof(pthread_attr_t) * MAXTHREADS);


        //Sort the largest segment first - sort MAXTHREADS at once

        //Sort the partitions
        int partitionsSorted = 0;
        int currentMaxPiece = PIECES - 1;

        //First we start the initial MAXTHREADS threads
        for(int i = 0; i < MAXTHREADS; i++)
        {
            printf("Launching thread to sort %9d - %9d (%9d) (row: %d)\n", pcs[currentMaxPiece].lo, pcs[currentMaxPiece].hi, pcs[currentMaxPiece].size, currentMaxPiece);
            pthread_attr_init(&attrArray[i]);
            pthread_create(&tidArray[i], &attrArray[i], &ThreadedQuicksort, &pcs[currentMaxPiece--]);
        }

        //Then, we check these threads until one finishes
        //At that time, we start the next largest segment in PIECES
        bool exitLoop = false;
        while(!exitLoop)
        {
            //Wait 100 us
            int uret = usleep(100);

            //Loop through each active thread to check if its finished
            for(int i = 0; i < MAXTHREADS; i++)
            {
                //All partitions are sorted
                if(partitionsSorted >= PIECES)
                {
                    exitLoop = true;
                    break;
                }
                //Thread is finished
                int ret = pthread_tryjoin_np(tidArray[i], NULL);
                if(ret == EBUSY)
                    continue;
                
                //There is at least one more thread to launch
                if(currentMaxPiece >= 0)
                {
                    printf("Launching thread to sort %9d - %9d (%9d) (row: %d)\n", pcs[currentMaxPiece].lo, pcs[currentMaxPiece].hi, pcs[currentMaxPiece].size, currentMaxPiece);
                    pthread_attr_init(&attrArray[i]);

                    //Begin thread execution
                    pthread_create(&tidArray[i], &attrArray[i], &ThreadedQuicksort, &pcs[currentMaxPiece--]);
                }

                //We've sorted one more partition - increment
                partitionsSorted++;
                
            }
        }
        //If early both enabled and is not finished
        if(EARLY == 'Y' || EARLY == 'y' && pthread_tryjoin_np(earlyTid, NULL) == EBUSY)
        {
            printf("EARLY THREAD STILL RUNNING AT END\n");
            pthread_join(earlyTid, NULL);
        } 
        //Otherwise, it has now been joined and we can continue.
        
        


        //Calculate time taken to sort the array
        //This is used later to display timing statistics
        gettimeofday(&sortEndTime, NULL);
        clock_t sortEndCpu = clock();
        cpuTime = (sortEndCpu - sortStartCpu) / (double)CLOCKS_PER_SEC;
    }
    else
    {
        //Calculate time taken to sort the array
        //This is used later to display timing statistics
        clock_t sortStartCpu = clock();
        gettimeofday(&sortStartTime, NULL);

        Quicksort(INTEGER_ARRAY, lo, hi);
        gettimeofday(&sortEndTime, NULL);
        clock_t sortEndCpu = clock();
        cpuTime = (sortEndCpu - sortStartCpu) / (double)CLOCKS_PER_SEC;
    }

    double sortTotalTime =(sortEndTime.tv_sec - sortStartTime.tv_sec)
                        +  ((sortEndTime.tv_usec-sortStartTime.tv_usec) * pow(10, -6));

    gettimeofday(&programEndTime, NULL);
    double programTotalTime =(programEndTime.tv_sec - programStartTime.tv_sec)
                        +  ((programEndTime.tv_usec-programStartTime.tv_usec) * pow(10, -6));

    //Display program statistics
    printf("Load: %3.3f Sort (Wall/CPU): %3.3f / %3.3f Total: %3.3f\n", loadTime, sortTotalTime, cpuTime, programTotalTime);

    //Final check to see if array is sorted
    if (!isSorted(INTEGER_ARRAY, SIZE)) printf("ERROR - Data Not Sorted\n");
}

/// @brief This function exists to get called by a thread - it calls the Quicksort function in the thread.
/// @param piece This parameter is used to define the boundaries that this thread is responsible for sorting.
/// @return Does not return
void* ThreadedQuicksort(void* piece)
{
    struct pieces *pc = piece;
    int lo = pc->lo;
    int hi = pc->hi;

    
    Quicksort(INTEGER_ARRAY, lo, hi);
    pthread_exit(0);
}



//This function writes to the global variables thus should be the first function called
/// @brief InterpretFlags takes in the command line arguments and parses any flags that may have been specified there
/// @param argc The length of argv array
/// @param argv An array of arguments
/// @param flag A pointer to a flag struct to be loaded with data
static inline void InterpretFlags(int argc, char* argv[])
{

    //Set default flags before user input
    SIZE = -1;
    ALTERNATE = 'S';
    THRESHOLD = 10;

    //Negative seed implies the seed is omitted
    SEED = -1;
    MULTITHREAD = 'Y';
    PIECES = 10;
    MAXTHREADS = 4;
    MEDIAN = 'n';
    EARLY = 'n';



    //This flag is required. If it has not been set upon function completion, abort.
    bool sizeHasBeenSet = false;

    //Save the flag for the next iteration
    char* sFlag = (char*)malloc(sizeof(char) * 3);

    //Argv[0] = ./lab2 (skip this)
    //This loop parses each argument
    //It maps a value to its corresponding flag
    for(int i = 1; i < argc; i++)
    {
        char* argument = argv[i];
        
        //Argument is a flag - save it in flag variable
        //This conditional exists to catch common errors with flag specification
        if(argument[0] == '-')
        {
            //Flag is just '-': abort program
            if(strlen(argument) < 2)
            {
                printf("Flag not specified. Abort.\n");
                abort();
            }
            //Flag is longer than -xx, meaning it is invalid.
            //The longest possible length is 3, if it equals '-m3'.
            else if(strlen(argument) > 3)
            {
                printf("Invalid flag length  (> 3). Abort.\n");
                abort();
            }

            sFlag = argument;
        }
        //Argument is a value corresponding to a flag
        else
        {   

            //sFlag is empty - value corresponds to no valid flag.
            if(strlen(sFlag) < 1)
            {
                printf("Value corresponds to no valid flag. Abort.\n");
                abort();
            }

            //This is a value corresponding to sFlag - handle it here

            //SIZE parameter
            if(strcmp(sFlag, "-n") == 0)
            {
                sizeHasBeenSet = true;
                SIZE = atoi(argument);

                //SIZE is invalid!
                if(SIZE == 0 || SIZE > 1000000000) 
                {
                    printf("Invalid size.\nPlease try again.\n");
                    abort();
                }
            }
            //ALTERNATE parameter
            else if(strcmp(sFlag, "-a") == 0)
            {
                char value = argument[0];
                if(value != 'S' && value != 's'
                && value != 'I' && value != 'i')
                {
                    printf("Value is invalid.\nAcceptable responses for flag -a include:\n");
                    printf("S/s or I/i\nPlease try again.\n");
                    abort();
                }
                ALTERNATE = argument[0]; 
            }
            //THRESHOLD parameter
            else if(strcmp(sFlag, "-s") == 0)
            {
                THRESHOLD = atoi(argument);

                //THRESHOLD is invalid!
                if(THRESHOLD  <= 0) 
                {
                    printf("Invalid value.\nPlease try again.\n");
                    abort();
                } 
            }
            //SEED parameter
            else if(strcmp(sFlag, "-r") == 0)
            {
                int value = atoi(argument);
                if (argument[0] != '0' && value == 0)
                {
                    printf("Value is invalid.\nAcceptable responses for -r must include a positive integer.\n");
                    abort();
                }
                SEED = abs(value) % BILLION; //Seed shall not exceed 1 billion
            }
            //MULTITHREAD parameter
            else if(strcmp(sFlag, "-m") == 0)
            {
                char value = argument[0];
                if(value != 'Y' && value != 'y'
                && value != 'N' && value != 'n')
                {
                    printf("Value is invalid.\nAcceptable responses for flag -m include:\n");
                    printf("Y/y or N/n\nPlease try again.\n");
                    abort();
                }
                MULTITHREAD = value; 

            }
            //PIECES parameter
            else if(strcmp(sFlag, "-p") == 0)
            {
                PIECES = atoi(argument);

                //PIECES is invalid!
                if(PIECES == 0) 
                {
                    printf("Invalid value.\nPlease try again.\n");
                    abort();
                } 
            }
            //MAXTHREADS parameter
            else if(strcmp(sFlag, "-t") == 0)
            {
                MAXTHREADS = atoi(argument);

                //MAXTHREADS is invalid!
                if(MAXTHREADS == 0) 
                {
                    printf("Invalid value.\nPlease try again.\n");
                    abort();
                } 
            }
            //MEDIAN parameter
            else if(strcmp(sFlag, "-m3") == 0)
            {
                char value = argument[0];
                if(value != 'Y' && value != 'y'
                && value != 'N' && value != 'n')
                {
                    printf("Value is invalid.\nAcceptable responses for flag -m3 include:\n");
                    printf("Y/y or N/n\nPlease try again.\n");
                    abort();
                }
                MEDIAN = argument[0]; 
            }
            //EARLY parameter
            else if(strcmp(sFlag, "-e") == 0)
            {
                char value = argument[0];
                if(value != 'Y' && value != 'y'
                && value != 'N' && value != 'n')
                {
                    printf("Value is invalid.\nAcceptable responses for flag -e include:\n");
                    printf("Y/y or N/n\nPlease try again.\n");
                    abort();
                }
               EARLY = argument[0]; 
            }
            //No valid flag
            else
            {
                printf("Flag not recognized.\nPlease try again.\n");
                abort();
            }




            //Reset memory by copying the empty string into its memory block
            strncpy(sFlag, "", strlen(argument));


        }
    }

    //We never received a (valid) size parameter - terminate program
    if(!sizeHasBeenSet || SIZE < 1) 
    {
        printf("Size (-n) is a required parameter.\nPlease try again.\n");
        abort();
    }

    if((MULTITHREAD == 'Y' || MULTITHREAD == 'y') && MAXTHREADS > PIECES)
    {
        printf("MAXTHREADS must be no more than PIECES.\nPlease try again.\n");
        abort();
    }
}

/// @brief The quicksort function - the vast majority of the sorting accomplished by this program happens here.
/// @param lo The low boundary to be sorted.
/// @param hi The high boundary to be sorted.
void Quicksort(int* partition, int lo, int hi)
{
    //Size is one - nothing  to sort
    if(hi - lo + 1 == 1)
    {
        return;
    }
    //Size equals two
    if(hi - lo +  1 == 2)
    {
        if(partition[hi] < partition[lo]) {int temp = partition[lo]; partition[lo] = partition[hi]; partition[hi] = temp;} // swap values in [lo] and [hi]
    }
    //Normal case
    else if(lo < hi)
    {
        if(hi - lo + 1 <= THRESHOLD)
        {
            //Alternate sorting method
            if(ALTERNATE == 'S' || ALTERNATE == 's')
            {
                //Shell sort
                int size = hi - lo + 1;
                int k = 1; 

                //Calculate index for Hibbard's sequence
                while (k <= size) k *= 2; 
                k = (k / 2) - 1;
                do {    
                    for (int i = lo; i < ( hi - k+1); i++) // for each comb position
                    for (int j = i; j >= lo; j -= k)    // Tooth-to-tooth is k
                        if (partition[j] <= partition[j + k]) break; // move upstream/exit?
                        else {int temp = partition[j]; partition[j] = partition[j+k]; partition[j+k] = temp;} // swap values in [j] and [j+k]
                    
                    k = k >> 1; // or k /= 2;
                }
                while (k > 0);
            }
            else if(ALTERNATE == 'I' || ALTERNATE == 'i')
            {
                //long long count = 0;
                //Insertion sort
                for(int i = lo+1; i <= hi; i++)
                {
                    //This sorts the entire list from 0 to i
                    for(int j = i; j > lo; j--)
                    {
                        //count++;

                        //if(i > hi) printf("I IS GREATER THAN HI!!\ni=%d\nhi=%d\n",i,hi);
                        //if(j <= lo) printf("J IS LESS THAN OR EQUAL TO LO!!\nj=%d\nlo=%d\n",j,lo);

                        //These two values aren't sorted
                        if(INTEGER_ARRAY[j] < INTEGER_ARRAY[j-1])
                        {
                            int temp = INTEGER_ARRAY[j]; INTEGER_ARRAY[j] = INTEGER_ARRAY[j-1]; INTEGER_ARRAY[j-1] = temp; // swap values in [j-1] and [j]
                        }
                        else break; // current value has gone upstream far enough; no sense looking any farther to the left!
                    }
                }
                //printf("Number of iterations=%lld\n", count);
            }
            return;
        }
        else
        {
            //if using median-of-three, apply it to put median in A[lo]
            if(MEDIAN == 'y' || MEDIAN == 'Y')
            {
                int k = (hi - lo) / 2;
                int median = 0;
                int A = partition[lo]; 
                int B = partition[k]; 
                int C = partition[hi];

                if ((A >= B && A <= C) || (A >= C && A <= B))
                {
                    median = lo; // i.e., A is between B & C (BAC or CAB)
                }
                else if ((B >= A && B <= C) || (B >= C && B <= A))
                {
                    median = k; // B between A & C (ABC CBA)
                }
                else
                {
                    median = hi;   // C is all that's left!
                } 
                {int temp = partition[lo]; partition[lo] = partition[median]; partition[median] = temp;} // swap values in [lo] and [median]
            }

            int i=lo; 
            int j=hi+1; // Partition starts here: start at the two ends
            int X = partition[lo];    // pick up the 26 – Pivot value in X
            do {
                do  i++; while (partition[i] < X && i < hi);
                do  j--; while (partition[j] > X && j > lo);
                if (i < j) {int temp = partition[i]; partition[i] = partition[j]; partition[j] = temp;} // swap values in [i] and [j]
                else       break;
            } while (1);
            {int temp = partition[lo]; partition[lo] = partition[j]; partition[j] = temp;} // swap values in [lo] and [j]

            //Left side is smaller
            if((j-1)-lo +1 < hi-(j+1)+1)
            {
                Quicksort(partition, lo, j - 1);	// (recursively) QS the left side
                Quicksort(partition, j + 1, hi);	// (recursively) QS the right side
            }
            //Right side is smaller
            else
            {
                Quicksort(partition, j + 1, hi);	// (recursively) QS the right side
                Quicksort(partition, lo, j - 1);	// (recursively) QS the left side
            }
        }
    }
    else return;
}

/// @brief DEBUG - Checks if an array is sorted.
/// @param partition The array to check.
/// @param size The size of said array.
/// @return Whether or not the array is sorted.
bool isSorted(int* partition, int size)
{
    for(int i = size - 1; i > 0; i--)
    {
        //Inversion detected
        if(partition[i] < partition[i-1]) return false;
    }
    return true;
}

/// @brief DEBUG - Prints an arrays contents.
/// @param partition The array to be printed.
/// @param size The size of said array.
void PrintArray(int *partition, int size)
{
    for(int i = 0; i < size; i++)
    {
        printf("partition[%d]=%d\n",i, partition[i]);
    }
}

/// @brief DEBUG - Prints out the global flags.
static inline void VerifyFlags()
{
    printf("SIZE=%d\nALTERNATE=%c\nTHRESHOLD=%d\nSEED=%d\nMULTITHREAD=%c\nPIECES=%d\nMAXTHREADS=%d\nMEDIAN=%c\nEARLY=%c\n",
    SIZE, ALTERNATE, THRESHOLD, SEED,MULTITHREAD,PIECES,MAXTHREADS,MEDIAN,EARLY);
}