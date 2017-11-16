#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv)
{
    int size, rank;
   
    MPI_Init(&argc, &argv);
         
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                  
    void datread(char *, void *, int , int );
    void pgmwrite(char *, void *, int , int );
    
    int MAX_ITERATIONS = 100, iteration = 0;
    int m = 400;
    int n = 300;
    float xLeft = -2, xRight = 2, yBottom = -1.5, yUpper = 1.5;

    int i = 0, j = 0, iLocal = 0, jLocal = 0;
    float buffer[m][n];

    int quit = 0, slavesCount = size - 1;
    int LOCAL_BUFF_SIZE = 50; 
    int MSG_QUIT = -1, MSG_READY = 1, MSG_PROCESS = 2, msg;
    float localBuffer[LOCAL_BUFF_SIZE][LOCAL_BUFF_SIZE];
    
    int currentPoint[2] = { 0, 0 };
       
    if (0 == slavesCount) {
        printf("Server:: This is parallel algorithm :@! Give me some slaves :)!\n");
        MPI_Finalize();
        return 0;
    }   
    
    if (0 == rank) {
        // Server...
        int iterator[2] = { 0, 0 };  
        MPI_Status status;
        printf("Server:: Server is starting...\n");
        while(slavesCount) {
            
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            //printf("Server:: Message received, msg is from %d %d\n", msg, status.MPI_SOURCE);

            if (MSG_READY == msg) {

                if (1 == quit) {
                    iterator[0] = MSG_QUIT;
                    MPI_Send(&iterator[0], 2, MPI_INT, status.MPI_SOURCE, status.MPI_SOURCE, MPI_COMM_WORLD);
                    slavesCount--;
                    continue;
                }
                // Send work to worker
                //printf("Server:: Message to worker will be sent, destination is %d\n", status.MPI_SOURCE);

                MPI_Send(&iterator[0], 2, MPI_INT, status.MPI_SOURCE, status.MPI_SOURCE, MPI_COMM_WORLD);
                // Move iterator 
                if (m == (iterator[0] + LOCAL_BUFF_SIZE)) {
                    iterator[0] = 0;
                    iterator[1] +=LOCAL_BUFF_SIZE;
                } else {
                    iterator[0] += LOCAL_BUFF_SIZE;
                };

                if (iterator[1] == n) {
                    printf("Server:: Server is goint to exit. Iterator is at position %d %d\n", iterator[0], iterator[1]);                    
                    quit = 1;                   
                }
            }

            if (MSG_PROCESS == msg) {
                // Receive result from worker
                MPI_Recv(&localBuffer[0][0], LOCAL_BUFF_SIZE * LOCAL_BUFF_SIZE, MPI_FLOAT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                MPI_Recv(&currentPoint[0], 2, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                
                //printf("Server:: Message PROCESS done received, source process is %d , current point is %d %d\n", status.MPI_SOURCE, currentPoint[0], currentPoint[1]);

                // Copy the result to the result buffer
                for(i = currentPoint[0], iLocal=0; i < currentPoint[0] + LOCAL_BUFF_SIZE; i++, iLocal++) {
                    for(j = currentPoint[1], jLocal=0; j < currentPoint[1] + LOCAL_BUFF_SIZE; j++, jLocal++) { 
                        buffer[i][j] = localBuffer[iLocal][jLocal];
                    }
                }
               
            }
         
        }
		// For DEBUG:
        /*for(i = 0; i < m; i++) {
            printf("\n");
             for(j = 0; j < n; j++) {
                printf("%f", buffer[i][j]);
            }
        }*/ 
        pgmwrite("image.pgm", buffer, m, n);
        printf("Server:: BYE BYE!\n");
    } else {
        // Worker...
        float x0, y0, x, y, xTemp, dX, dY;
        dX = (xRight - xLeft) / (m-1);
        dY = (yUpper - yBottom) / (n-1);      
        while(!quit) {
           
            MPI_Send(&MSG_READY, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Recv(&currentPoint[0], 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (currentPoint[0] == MSG_QUIT) {
                printf("Client: %d BYE BYE!\n", rank);
                quit = 1;
                break;
            }

            //printf("Client:: Message received, worker %d will process starting points %d %d\n", rank, currentPoint[0], currentPoint[1]);

            for(i = currentPoint[0], iLocal=0 ; i < currentPoint[0] + LOCAL_BUFF_SIZE; i++, iLocal++) {
             
                x0 = i*dX + xLeft;
        
                for(j = currentPoint[1], jLocal=0; j < currentPoint[1] + LOCAL_BUFF_SIZE; j++,jLocal++) {
            
                    y0 = j*dY + yBottom;
                    x = 0.0;
                    y = 0.0;
                    iteration = 0;
                    while (iteration < MAX_ITERATIONS && (x*x + y*y) < 4) {
                        xTemp = x*x - y*y + x0;
                        y = 2*x*y + y0;
                        x = xTemp;
                        iteration++;
                        //printf("Iterations count: %d %f %f\n", iteration, x, y);
                    }
                    //printf("Iterations count: %d %d %d\n", iteration, i, j);
                    if ((x*x + y*y) < 4) { 
                        localBuffer[iLocal][jLocal] = 255;
                    } else {
                        localBuffer[iLocal][jLocal] = 0.0;
                    }
                }
            }
            //printf("Client:: Calculations finnished, msg is %d\n", msg);
            MPI_Send(&MSG_PROCESS, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&localBuffer[0][0], LOCAL_BUFF_SIZE * LOCAL_BUFF_SIZE, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&currentPoint[0], 2, MPI_INT, 0, 0, MPI_COMM_WORLD);           
        }
    }
    
    MPI_Finalize();

    return 0;
}
