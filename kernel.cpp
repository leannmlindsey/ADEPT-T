#include <kernel.hpp>

__inline__ __device__ short
gpu_bsw::warpReduceMax_with_index_reverse(short val, short& myIndex, short& myIndex2, unsigned lengthSeqB)
{
    int   warpSize = 32;
    short myMax    = 0;
    short newInd   = 0;
    short newInd2  = 0;
    short ind      = myIndex;
    short ind2     = myIndex2;
    myMax          = val;
    unsigned mask  = __ballot_sync(0xffffffff, threadIdx.x < lengthSeqB);  // blockDim.x
    // unsigned newmask;
    for(int offset = warpSize / 2; offset > 0; offset /= 2)
    {

        int tempVal = __shfl_down_sync(mask, val, offset);
        val     = max(val,tempVal);
        newInd  = __shfl_down_sync(mask, ind, offset);
        newInd2 = __shfl_down_sync(mask, ind2, offset);

      //  if(threadIdx.x == 0)printf("index1:%d, index2:%d, max:%d\n", newInd, newInd2, val);
        if(val != myMax)
        {
            ind   = newInd;
            ind2  = newInd2;
            myMax = val;
        }
        else if((val == tempVal) ) // this is kind of redundant and has been done purely to match the results
                                    // with SSW to get the smallest alignment with highest score. Theoreticaly
                                    // all the alignmnts with same score are same.
        {
          if(newInd2 > ind2){
            ind = newInd;
            ind2 = newInd2;

          }
        }
    }
    myIndex  = ind;
    myIndex2 = ind2;
    val      = myMax;
    return val;
}

__inline__ __device__ short
gpu_bsw::warpReduceMax_with_index(short val, short& myIndex, short& myIndex2, unsigned lengthSeqB)
{
    int   warpSize = 32;
    short myMax    = 0;
    short newInd   = 0;
    short newInd2  = 0;
    short ind      = myIndex;
    short ind2     = myIndex2;
    myMax          = val;
    unsigned mask  = __ballot_sync(0xffffffff, threadIdx.x < lengthSeqB);  // blockDim.x
    // unsigned newmask;
    for(int offset = warpSize / 2; offset > 0; offset /= 2)
    {

        int tempVal = __shfl_down_sync(mask, val, offset);
        val     = max(val,tempVal);
        newInd  = __shfl_down_sync(mask, ind, offset);
        newInd2 = __shfl_down_sync(mask, ind2, offset);
        if(val != myMax)
        {
            ind   = newInd;
            ind2  = newInd2;
            myMax = val;
        }
        else if((val == tempVal) ) // this is kind of redundant and has been done purely to match the results
                                    // with SSW to get the smallest alignment with highest score. Theoreticaly
                                    // all the alignmnts with same score are same.
        {
          if(newInd < ind){
            ind = newInd;
            ind2 = newInd2;
          }
        }
    }
    myIndex  = ind;
    myIndex2 = ind2;
    val      = myMax;
    return val;
}



__device__ short
gpu_bsw::blockShuffleReduce_with_index_reverse(short myVal, short& myIndex, short& myIndex2, unsigned lengthSeqB)
{
    int              laneId = threadIdx.x % 32;
    int              warpId = threadIdx.x / 32;
    __shared__ short locTots[32];
    __shared__ short locInds[32];
    __shared__ short locInds2[32];
    short            myInd  = myIndex;
    short            myInd2 = myIndex2;
    myVal                   = warpReduceMax_with_index_reverse(myVal, myInd, myInd2, lengthSeqB);

    __syncthreads();
    if(laneId == 0)
        locTots[warpId] = myVal;
    if(laneId == 0)
        locInds[warpId] = myInd;
    if(laneId == 0)
        locInds2[warpId] = myInd2;
    __syncthreads();
    unsigned check =
        ((32 + blockDim.x - 1) / 32);  // mimicing the ceil function for floats
                                       // float check = ((float)blockDim.x / 32);
    if(threadIdx.x < check)  /////******//////
    {
        myVal  = locTots[threadIdx.x];
        myInd  = locInds[threadIdx.x];
        myInd2 = locInds2[threadIdx.x];
    }
    else
    {
        myVal  = 0;
        myInd  = -1;
        myInd2 = -1;
    }
    __syncthreads();

    if(warpId == 0)
    {
        myVal    = warpReduceMax_with_index_reverse(myVal, myInd, myInd2, lengthSeqB);
        myIndex  = myInd;
        myIndex2 = myInd2;
    }
    __syncthreads();
    return myVal;
}

__device__ short
gpu_bsw::blockShuffleReduce_with_index(short myVal, short& myIndex, short& myIndex2, unsigned lengthSeqB)
{
    int              laneId = threadIdx.x % 32;
    int              warpId = threadIdx.x / 32;
    __shared__ short locTots[32];
    __shared__ short locInds[32];
    __shared__ short locInds2[32];
    short            myInd  = myIndex;
    short            myInd2 = myIndex2;
    myVal                   = warpReduceMax_with_index(myVal, myInd, myInd2, lengthSeqB);

    __syncthreads();
    if(laneId == 0)
        locTots[warpId] = myVal;
    if(laneId == 0)
        locInds[warpId] = myInd;
    if(laneId == 0)
        locInds2[warpId] = myInd2;
    __syncthreads();
    unsigned check =
        ((32 + blockDim.x - 1) / 32);  // mimicing the ceil function for floats
                                       // float check = ((float)blockDim.x / 32);
    if(threadIdx.x < check)  /////******//////
    {
        myVal  = locTots[threadIdx.x];
        myInd  = locInds[threadIdx.x];
        myInd2 = locInds2[threadIdx.x];
    }
    else
    {
        myVal  = 0;
        myInd  = -1;
        myInd2 = -1;
    }
    __syncthreads();

    if(warpId == 0)
    {
        myVal    = warpReduceMax_with_index(myVal, myInd, myInd2, lengthSeqB);
        myIndex  = myInd;
        myIndex2 = myInd2;
    }
    __syncthreads();
    return myVal;
}



__device__ __host__ short
           gpu_bsw::findMaxFour(short first, short second, short third, short fourth, int* ind)
{
    short array[4];
    array[0] = first; //diag_score
    array[1] = second; //_curr_F
    array[2] = third; //_curr_E
    array[3] = fourth; // 0 

    short max;

    // to make sure that if the H score is 0, then * will be put in the H_ptr matrix for correct termination of traceback 
    if (array[0] > 0 ) {
      max = array[0];
      *ind = 0;
    } else {
      max = 0;
      *ind = 3;
    }
    
    // //findMaxFour(diag_score, _curr_F, _curr_E, 0, 

    for (int i=1; i<4; i++){
      if (array[i] > max){
        max = array[i];
        *ind = i;
      }
    }

    return max;
}

__device__ short
gpu_bsw::intToCharPlusWrite(int num, char* CIGAR, short cigar_position)
{
    int last_digit = 0;
    int digit_length = 0;
    char digit_array[5];
    //printf("num = %d, cigar_position = %d\n", num, cigar_position);
    // convert the int num to ASCII digit by digit and record in a digit_array
    while (num != 0){
        last_digit = num%10;
        digit_array[digit_length] = char('0' + last_digit);
        num = num/10;
        digit_length++;
    }

    //write each char of the digit_array to the CIGAR string
    for (int q = 0; q < digit_length; q++){
        CIGAR[cigar_position]=digit_array[q];
        cigar_position++; 
    }
    //printf("cigar_position = %d\n", cigar_position);
    return cigar_position;
}

__device__ void
gpu_bsw::createCIGAR(char* longCIGAR, char* CIGAR, int maxCIGAR, 
        const char* seqA, const char* seqB, unsigned lengthShorterSeq, unsigned lengthLongerSeq, 
        bool seqBShorter, short first_j, short last_j, short first_i, short last_i) 
{
    int myId = blockIdx.x;
    int myTId = threadIdx.x;
    if(myId==0 && myTId ==0 && DEBUG_PRINT == 1) {
     printf("made it to CIGAR. lengthShorterSeq = %i, lengthLongerSeq = %i, first_j = %i, last_j = %i, first_i = %i, last_i = %i",
     lengthShorterSeq, lengthLongerSeq, first_j, last_j, first_i, last_i);
    }

    short cigar_position = 0;

    short beg_S;
    short end_S; 

    if (seqBShorter){
        
        beg_S = lengthShorterSeq - first_j-1;
        end_S = last_j; 
    } else {
        beg_S = lengthLongerSeq - first_i-1; 
        end_S = last_i;
    }
    
    if ( beg_S != 0){
        CIGAR[0]='S';
        cigar_position++ ; 
        cigar_position = intToCharPlusWrite(beg_S, CIGAR, cigar_position);
    }

    int p = 0;
    while(longCIGAR[p] != '\0'){
       
        int letter_count = 1;
        
        while (longCIGAR[p] == longCIGAR[p+1]){
            letter_count++; 
            p++; 
        }

        CIGAR[cigar_position]=longCIGAR[p];
        cigar_position++ ; 
       
        cigar_position = intToCharPlusWrite(letter_count, CIGAR, cigar_position); 
        p++;

        // if(myId==0 && myTId ==0&& DEBUG_PRINT == 1) {
        //      for (int i = 0; i <= cigar_position; i++){
        //          printf("%c",CIGAR[i]);
        //      }
        //      printf("\n");
        //  }  
        //  if(myId==0 && myTId ==0&& DEBUG_PRINT == 1) {
        //      for (int i = 0; i <= cigar_position; i++){
        //          printf("%c",longCIGAR[p]);
        //      }
        //      printf("\n");
        //  }  
    }
    
    if ( end_S != 0){
        CIGAR[cigar_position]='S';
        cigar_position++ ; 
        cigar_position = intToCharPlusWrite(end_S, CIGAR, cigar_position);
    }    
    cigar_position--;
    char temp;
    //code to reverse the cigar by swapping i and length of cigar - i
    for(int i = 0; i<(cigar_position)/2+1;i++){
        temp = CIGAR[i]; 
        CIGAR[i]=CIGAR[cigar_position-i]; 
        CIGAR[cigar_position-i] = temp; 
    }
    
    CIGAR[cigar_position+1]='\0';
    

}

__device__ void
gpu_bsw::printMatrix(char* H_ptr, short* I, char* seqA, char* seqB, int lengthSeqA, int lengthSeqB, unsigned long* diagOffset, unsigned maxSize){
    //printf("AFTER TRACEBACK: I=%d\n", I[diagOffset[27]+9]);
    //put smaller sequence across the top and label 
    char* topSeq;
    char* sideSeq;
    int topSeqLength;
    int sideSeqLength;
    //printf("lengthSeqA = %d, length SeqB = %d, maxSize = %u\n", lengthSeqA, lengthSeqB, maxSize);

    if (lengthSeqA < lengthSeqB){
        topSeq = seqA;
        sideSeq = seqB;
        topSeqLength = lengthSeqA;
        sideSeqLength = lengthSeqB;
    } else {
        topSeq = seqB;
        sideSeq = seqA;
        topSeqLength = lengthSeqB;
        sideSeqLength = lengthSeqA;
    }
   

                char temp_H;
                printf("FULL H POINTER MATRIX\n");
                 int S_current_diagId    = 0;
                 int S_current_locOffset = 0;
                printf("-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%d\t", sj); //indexes printed across the top 
                }
                printf("\n-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%c\t", topSeq[sj]); //query sequence printed across the top 
                }
                printf("\n");
                for (int si = 0; si < sideSeqLength + 1; si++){
                    printf("%d\t%c\t", si, sideSeq[si]); //reference sequence & indexes printed down the side 
                  
                    for (int sj =0; sj < topSeqLength + 1; sj++){
                       
                        S_current_diagId = si + sj;

                        if(S_current_diagId < maxSize + 1){
                            S_current_locOffset = sj;
                        } else {
                            unsigned short S_myOff = S_current_diagId - maxSize;
                            S_current_locOffset = sj - S_myOff;
                        }
                        //printf("%d\t",I_score[diagOffset[S_current_diagId] + S_current_locOffset]);
                        //printf("%d\t",H_ptr[diagOffset[S_current_diagId] + S_current_locOffset]);
                        temp_H = H_ptr[diagOffset[S_current_diagId] + S_current_locOffset];
                        switch (temp_H & 0b00001100){
                            case 0b00001100 :
                                printf("\\\t");
                                break;
                            case 0b00001000 :
                                printf("|\t");
                                break;
                            case 0b00000100 :
                                printf("-\t");
                                break;
                            case 0b00000000 :
                                printf("*\t");
                                break;
                      


                        }
                  
                }
                printf("\n");
            }

            printf("FULL E POINTER MATRIX\n");
                 S_current_diagId    = 0;
                 S_current_locOffset = 0;
                printf("-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%d\t", sj); //indexes printed across the top 
                }
                printf("\n-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%c\t", topSeq[sj]); //query sequence printed across the top 
                }
                printf("\n");
                for (int si = 0; si < sideSeqLength + 1; si++){
                    printf("%d\t%c\t", si, sideSeq[si]); //reference sequence & indexes printed down the side 
                  
                    for (int sj =0; sj < topSeqLength + 1; sj++){
                       
                        S_current_diagId = si + sj;

                        if(S_current_diagId < maxSize + 1){
                            S_current_locOffset = sj;
                        } else {
                            unsigned short S_myOff = S_current_diagId - maxSize;
                            S_current_locOffset = sj - S_myOff;
                        }
                        //printf("%d\t",I_score[diagOffset[S_current_diagId] + S_current_locOffset]);
                        //printf("%d\t",H_ptr[diagOffset[S_current_diagId] + S_current_locOffset]);
                        temp_H = H_ptr[diagOffset[S_current_diagId] + S_current_locOffset];
                        switch (temp_H & 0b00000010){ //values from E
                            case 0b00000010 :
                                printf("-\t");
                                break;
                            case 0b00000000 :
                                printf("*\t");
                                break;


                        }
                  
                }
                printf("\n");
            }

            printf("FULL F POINTER MATRIX\n");
                 S_current_diagId    = 0;
                 S_current_locOffset = 0;
                printf("-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%d\t", sj); //indexes printed across the top 
                }
                printf("\n-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%c\t", topSeq[sj]); //query sequence printed across the top 
                }
                printf("\n");
                for (int si = 0; si < sideSeqLength + 1; si++){
                    printf("%d\t%c\t", si, sideSeq[si]); //reference sequence & indexes printed down the side 
                  
                    for (int sj =0; sj < topSeqLength + 1; sj++){
                       
                        S_current_diagId = si + sj;

                        if(S_current_diagId < maxSize + 1){
                            S_current_locOffset = sj;
                        } else {
                            unsigned short S_myOff = S_current_diagId - maxSize;
                            S_current_locOffset = sj - S_myOff;
                        }
                        //printf("%d\t",I_score[diagOffset[S_current_diagId] + S_current_locOffset]);
                        //printf("%d\t",H_ptr[diagOffset[S_current_diagId] + S_current_locOffset]);
                        temp_H = H_ptr[diagOffset[S_current_diagId] + S_current_locOffset];
                        switch (temp_H & 0b00000001){ //values from E
                            case 0b00000001 :
                                printf("|\t");
                                break;
                            case 0b00000000 :
                                printf("*\t");
                                break;


                        }
                  
                }
                printf("\n");
            }


             int score;
                printf("FULL SCORING MATRIX\n");
                S_current_diagId    = 0;
                S_current_locOffset = 0;
                printf("-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%d\t", sj); //indexes printed across the top 
                }
                printf("\n-\t-\t");
                for (int sj = 0; sj < topSeqLength; sj++){
                    printf("%c\t", topSeq[sj]); //query sequence printed across the top 
                }
                printf("\n");
                for (int si = 0; si < sideSeqLength + 1; si++){
                    printf("%d\t%c\t", si, sideSeq[si]); //reference sequence & indexes printed down the side 
                  
                    for (int sj =0; sj < topSeqLength + 1; sj++){
                       
                        S_current_diagId = si + sj;
                        S_current_locOffset = 0;

                        if(S_current_diagId < maxSize + 1){
                            S_current_locOffset = sj;
                        } else {
                            unsigned short S_myOff = S_current_diagId - maxSize;
                            S_current_locOffset = sj - S_myOff;
                        }

    // current_diagId    = current_i + current_j;
    // current_locOffset = 0;
    // if(current_diagId < maxSize + 1)
    // {
    //     current_locOffset = current_j;
    // }
    // else
    // {
    //     unsigned short myOff = current_diagId - maxSize;
    //     current_locOffset    = current_j - myOff;
    // }



                        //printf("%d\t",I_score[diagOffset[S_current_diagId] + S_current_locOffset]);
                        //printf("%d\t",H_ptr[diagOffset[S_current_diagId] + S_current_locOffset]);
                        score = I[diagOffset[S_current_diagId] + S_current_locOffset];
                        printf("%d\t",score);

                    }
                  printf("\n");
                }
                

}

__device__ void
gpu_bsw::traceBack(short current_i, short current_j, char* seqA_array, char* seqB_array, unsigned* prefix_lengthA, 
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, unsigned const maxMatrixSize, int maxCIGAR,
                    char* longCIGAR, char* CIGAR, char* H_ptr, unsigned long* diagOffset)
{   //printf("current_i = %d, current_j = %d\n", current_i, current_j);
    
    int myId = blockIdx.x;
    int myTId = threadIdx.x;
    
    char*    seqA;
    char*    seqB;

    int lengthSeqA;
    int lengthSeqB;

    if(myId == 0)
    {
        lengthSeqA = prefix_lengthA[0];
        lengthSeqB = prefix_lengthB[0];
        seqA       = seqA_array;
        seqB       = seqB_array;
    
    }
    else
    {
        lengthSeqA = prefix_lengthA[myId] - prefix_lengthA[myId - 1];
        lengthSeqB = prefix_lengthB[myId] - prefix_lengthB[myId - 1];
        seqA       = seqA_array + prefix_lengthA[myId - 1];
        seqB       = seqB_array + prefix_lengthB[myId - 1];

    }

    unsigned short current_diagId;     // = current_i+current_j;
    unsigned short current_locOffset;  // = 0;
    unsigned maxSize = lengthSeqA > lengthSeqB ? lengthSeqA : lengthSeqB;
   
    const char* longerSeq = lengthSeqA < lengthSeqB ? seqB : seqA; 
    const char* shorterSeq = lengthSeqA < lengthSeqB ? seqA : seqB; 
    unsigned lengthShorterSeq = lengthSeqA < lengthSeqB ? lengthSeqA : lengthSeqB;  
    unsigned lengthLongerSeq = lengthSeqA < lengthSeqB ? lengthSeqB : lengthSeqA;    
    bool seqBShorter = lengthSeqA < lengthSeqB ? false : true;  //need to keep track of whether query or ref is shorter for CIGAR 

    current_diagId    = current_i + current_j;
    current_locOffset = 0;
    if(current_diagId < maxSize + 1)
    {
        current_locOffset = current_j;
    }
    else
    {
        unsigned short myOff = current_diagId - maxSize;
        current_locOffset    = current_j - myOff;
    }

    if(myId == 0 && myTId ==0 && DEBUG_PRINT == 1){
        printf("Beginning Traceback on Block #%d\n", myId);
        printf("diagID:%d locoffset:%d current_i:%d, current_j:%d, Block #%d\n\n",current_diagId, current_locOffset, current_i, current_j, myId);
        printf("BLOCK #%d, first H_ptr[] = %c, H_ptr[0] = %c\n", myId, H_ptr[diagOffset[current_diagId] + current_locOffset], H_ptr[0]);                
        
    }
    char temp_H;
    temp_H = H_ptr[diagOffset[current_diagId] + current_locOffset];
    short next_i;
    short next_j;
    

    short first_j = current_j; //recording the first_j, first_i values for use in calculating S
    short first_i = current_i;
    char matrix = 'H'; //initialize with H

    int counter = 0;
    short prev_i;
    short prev_j;
    bool continueTrace = true;
   
    while(continueTrace && (current_i != 0) && (current_j !=0))
    {   
       temp_H = H_ptr[diagOffset[current_diagId] + current_locOffset];
       
       
         if(myId==0 && myTId ==0 && DEBUG_PRINT == 1) {
             printf("current_i:%d, current_j:%d, %c , %c, %d\n",current_i, current_j, shorterSeq[current_j], longerSeq[current_i],counter);
         } 

        //write the current value into longCIGAR then assign next_i
        if (matrix == 'H') { 
            if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
              printf("in matrix H");
            }
            
            switch (temp_H & 0b00001100){    
                case 0b00001100 :
                    matrix = 'H';
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00001100\n");
                    }
                    longCIGAR[counter] = shorterSeq[current_j] == longerSeq[current_i] ? '=' : 'X';
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j - 1;
                    break;
                case 0b00001000 :
                    matrix = 'F';
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00001000\n");
                    }
                    next_i = current_i;
                    next_j = current_j;
                    break;
                case 0b00000100 :
                    matrix = 'E';
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000100\n");
                    }
                    next_i = current_i;
                    next_j = current_j;
                    break;
                 case 0b00000000 : 
                    continueTrace = false;
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000000\n");
                    }
                    break;
            }
        } else if (matrix == 'E'){
            if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
              printf("in matrix E");
            }
            switch (temp_H & 0b00000010){
                case 0b00000010 :
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000010\n");
                    }
		                longCIGAR[counter] = seqBShorter ? 'I' : 'D';
                    //if (longCIGAR[counter-1] == 'X' ) {
                      //  if (longCIGAR[counter] == 'I') {longCIGAR[counter-1] = 'I';}
                      //  else if (longCIGAR[counter] == 'D') {longCIGAR[counter-1] = 'D';}
                      //  }
                    counter++;
                    next_i = current_i;
                    next_j = current_j - 1;
                    break;
                case 0b00000000 :
                    matrix = 'H';
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000000\n");
                    }
                    longCIGAR[counter] = seqBShorter ? 'I' : 'D';
                    //if (longCIGAR[counter-1] == 'X' ) {
                      //  if (longCIGAR[counter] == 'I') {longCIGAR[counter-1] = 'I';}
                      //  else if (longCIGAR[counter] == 'D') {longCIGAR[counter-1] = 'D';}
                      //  }
                    counter++;
                    next_i = current_i;
                    next_j = current_j - 1;
                    break;
            }
        } else if (matrix == 'F'){
            if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
              printf("in matrix F");
            }
            switch (temp_H & 0b00000001){
                case 0b00000001 :
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000001\n");
                    }
		                longCIGAR[counter] = seqBShorter ? 'D' : 'I';
                    //if (longCIGAR[counter-1] == 'X' ) {
                      // if (longCIGAR[counter] == 'I') {longCIGAR[counter-1] = 'I';}
                      // else if (longCIGAR[counter] == 'D') {longCIGAR[counter-1] = 'D';}
                    //}
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j;
                    break;
                case 0b00000000 :
                    matrix = 'H';
                    if(myId==0 && myTId==0&& DEBUG_PRINT == 1){
                      printf("00000000\n");
                    }
                    longCIGAR[counter] = seqBShorter ? 'D' : 'I';
                    //if (longCIGAR[counter-1] == 'X' ) {
                      // if (longCIGAR[counter] == 'I') {longCIGAR[counter-1] = 'I';}
                      // else if (longCIGAR[counter] == 'D') {longCIGAR[counter-1] = 'D';}
                    //}
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j;
                    break;
            }
        }
        if(myId==0 && myTId ==0&& DEBUG_PRINT == 1) {
             for (int i = 0; i <= counter; i++){
                 printf("%c",longCIGAR[i]);
             }
             printf("\n");
         }  
        if(myId==0 && myTId ==0&& DEBUG_PRINT == 1) {
            printf("     %c\n",longCIGAR[counter]);
        }
        if (continueTrace != false){
          prev_i = current_i; //record current values in case this is the stop location
          prev_j = current_j;

          current_i = next_i;
          current_j = next_j;

          current_diagId    = current_i + current_j;
          current_locOffset = 0;

          if(current_diagId < maxSize + 1)
          {
            current_locOffset = current_j;
          } else {
            unsigned short myOff2 = current_diagId - maxSize;
            current_locOffset     = current_j - myOff2;
          }
        //counter++;   
        }
  }
   if ((current_i == 0) || (current_j == 0)) {
      if(myId==0 && myTId ==0&& DEBUG_PRINT == 1) {
            printf("counter = %i, current_j = %i, current_i = %i, shorterSeq[current_j] = %c, longerSeq[current_i] = %c ", 
            counter, current_j, current_i, shorterSeq[current_j],longerSeq[current_i] );
      }

      if (shorterSeq[current_j] == longerSeq[current_i] ){
        longCIGAR[counter] = '=';
        longCIGAR[counter+1] = '\0';
        prev_j = current_j;
        prev_i = current_i;
      }
      else {
        longCIGAR[counter]='\0';
      }
   } else {
    longCIGAR[counter] = '\0';
   }
   current_i ++; current_j++; next_i ++; next_j ++;


  //  if(myId==0 && myTId ==0) {
  //           for (int i = 0; i <= counter; i++){
  //               printf("%c",longCIGAR[i]);
  //           }
  //           printf("\n\n");
  //       }  
  //       if(myId==0 && myTId ==0) {
  //           printf("current_i:%d, current_j:%d, %c , %c, %d\n",current_i, current_j, shorterSeq[current_j], longerSeq[current_i], counter);
  //       } 
   
    if(lengthSeqA < lengthSeqB){  
        seqB_align_begin[myId] = prev_i;
        seqA_align_begin[myId] = prev_j;
    }else{
        seqA_align_begin[myId] = prev_i;
        seqB_align_begin[myId] = prev_j;
    }

    if (myTId == 0){
     gpu_bsw::createCIGAR(longCIGAR, CIGAR, maxCIGAR, seqA, seqB, lengthShorterSeq, lengthLongerSeq, seqBShorter, first_j, prev_j, first_i, prev_i);
    }

    
    // *tick_out = tick;
}

__global__ void
gpu_bsw::sequence_dna_kernel_traceback(char* seqA_array, char* seqB_array, unsigned* prefix_lengthA,
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, short* top_scores, 
                    char* longCIGAR_array, char* CIGAR_array, char* H_ptr_array, short* I_ptr_array,
                    int maxCIGAR, unsigned const maxMatrixSize, short matchScore, short misMatchScore, short startGap, short extendGap)
{
   
    int block_Id  = blockIdx.x;
    int thread_Id = threadIdx.x;
    short laneId = threadIdx.x%32;
    short warpId = threadIdx.x/32;

    unsigned lengthSeqA;
    unsigned lengthSeqB;
    // local pointers
    char*    seqA;
    char*    seqB;

    char* H_ptr;
    char* CIGAR, *longCIGAR;
    short* I;
     
    extern __shared__ char is_valid_array[];
    char*                  is_valid = &is_valid_array[0];

// setting up block local sequences and their lengths.
      
    if(block_Id == 0)
    {
        lengthSeqA = prefix_lengthA[0];
        lengthSeqB = prefix_lengthB[0];
        seqA       = seqA_array;
        seqB       = seqB_array;
    }
    else
    {
        lengthSeqA = prefix_lengthA[block_Id] - prefix_lengthA[block_Id - 1];
        lengthSeqB = prefix_lengthB[block_Id] - prefix_lengthB[block_Id - 1];
        seqA       = seqA_array + prefix_lengthA[block_Id - 1];
        seqB       = seqB_array + prefix_lengthB[block_Id - 1];
    }
    

    // what is the max length and what is the min length
    unsigned maxSize = lengthSeqA > lengthSeqB ? lengthSeqA : lengthSeqB;
    unsigned minSize = lengthSeqA < lengthSeqB ? lengthSeqA : lengthSeqB;

    H_ptr = H_ptr_array + (block_Id * maxMatrixSize);
    I = I_ptr_array + (block_Id * maxMatrixSize);

    longCIGAR = longCIGAR_array + (block_Id * maxCIGAR);
    CIGAR = CIGAR_array + (block_Id * maxCIGAR);

     
    char* longer_seq;
    unsigned long* diagOffset = (unsigned long*) (&is_valid_array[3 * (minSize + 1) * sizeof(long)]);


// shared memory space for storing longer of the two strings
      //if (block_Id == 0 && thread_Id == 0) {printf("before memset - is_valid = %p, diagOffset = %p\n",is_valid, diagOffset);}
    memset(is_valid, 0, minSize);
    is_valid += minSize;
      //if (block_Id == 0 && thread_Id == 0) {printf("after memset 1 - is_valid = %p, diagOffset = %p\n",is_valid, diagOffset);}
    memset(is_valid, 1, minSize);
      //if (block_Id == 0 && thread_Id == 0) {printf("after memset 2 - is_valid = %p, diagOffset = %p\n",is_valid, diagOffset);}
    is_valid += minSize;
    memset(is_valid, 0, minSize);
      //if (block_Id == 0 && thread_Id == 0) {printf("after memset 3 - is_valid = %p, diagOffset = %p \n",is_valid, diagOffset);}

    char myColumnChar;
    // the shorter of the two strings is stored in thread registers
    char H_temp = 0;  //set all bits to 0 initially

    if(lengthSeqA < lengthSeqB)
    {
      if(thread_Id < lengthSeqA)
        myColumnChar = seqA[thread_Id];  // read only once
      longer_seq = seqB;
    }
    else
    {
      if(thread_Id < lengthSeqB)
        myColumnChar = seqB[thread_Id];
      longer_seq = seqA;
    }

    __syncthreads(); // this is required here so that complete sequence has been copied to shared memory

    int   i            = 0;
    int   j            = thread_Id;
    short thread_max   = 0; // to maintain the thread max score
    short thread_max_i = 0; // to maintain the DP coordinate i for the longer string
    short thread_max_j = 0;// to maintain the DP cooirdinate j for the shorter string
    int ind;

    //set up the prefixSum for diagonal offset look up table for H_ptr, E_ptr, F_ptr
    int    locSum = 0;
     
    for(int diag = 0; diag < lengthSeqA + lengthSeqB-1; diag++) 
    {
        
        int locDiagId = diag;
        if(thread_Id == 0)
        {
            
            if(locDiagId <= minSize + 1)
            {
                locSum += locDiagId;
                diagOffset[locDiagId] = locSum;
            }
            else if(locDiagId > maxSize + 1)
            {
                locSum += (minSize + 1) - (locDiagId - (maxSize + 1));
                diagOffset[locDiagId] = locSum;
            }
            else
            {
                locSum += minSize + 1;
                diagOffset[locDiagId] = locSum;
            }
            diagOffset[lengthSeqA + lengthSeqB] = locSum + 2; //what is this for?
            //if (block_Id == 0 && thread_Id == 0) {printf("inside diagOffset loop - is_valid = %p, diagOffset = %p, writing at index = %d, locDiagId * 4 = %d\n",is_valid, diagOffset, locDiagId, locDiagId*4);}
            //printf("diag = %d, diagOffset = %d\n", locDiagId, diagOffset[locDiagId]);
        }
    }
    
    __syncthreads(); //to make sure prefixSum is calculated before the threads start calculations.    


  //initializing registers for storing diagonal values for three recent most diagonals (separate tables for
  //H, E and F)
    short _curr_H = 0, _curr_F = -100, _curr_E = -100; //-100 acts as neg infinity
    short _prev_H = 0, _prev_F = -100, _prev_E = -100;
    short _prev_prev_H = 0, _prev_prev_F = -100, _prev_prev_E = -100;
    short _temp_Val = 0;

   __shared__ short sh_prev_E[32]; // one such element is required per warp
   __shared__ short sh_prev_H[32];
   __shared__ short sh_prev_prev_H[32];

   __shared__ short local_spill_prev_E[1024];// each threads local spill,
   __shared__ short local_spill_prev_H[1024];
   __shared__ short local_spill_prev_prev_H[1024];

    //unsigned      alignmentPad = 4 + (4 - totBytes % 4);

    __syncthreads(); // to make sure all shmem allocations have been initialized

    
    for(int diag = 0; diag < lengthSeqA + lengthSeqB-1; diag++)
    {  // iterate for the number of anti-diagonals

        
        unsigned short diagId    = i + j;
        unsigned short locOffset = 0;
        if(diagId < maxSize + 1) 
        {
            locOffset = j;
        }
        else
        {
          unsigned short myOff = diagId - maxSize;
          locOffset            = j - myOff;
        }

        //if (thread_Id == 0 && block_Id == 0) {
          //printf("i = %d, j = %d, diagId = %d, locOffset = %d\n", i, j, diagId, locOffset);
        //}

        is_valid = is_valid - (diag < minSize || diag >= maxSize); //move the pointer to left by 1 if cnd true

	       _temp_Val = _prev_H; // value exchange happens here to setup registers for next iteration
	       _prev_H = _curr_H;
	       _curr_H = _prev_prev_H;
	       _prev_prev_H = _temp_Val;
         _curr_H = 0;

        _temp_Val = _prev_E;
        _prev_E = _curr_E;
        _curr_E = _prev_prev_E;
        _prev_prev_E = _temp_Val;
        _curr_E = -100;

        _temp_Val = _prev_F;
        _prev_F = _curr_F;
        _curr_F = _prev_prev_F;
        _prev_prev_F = _temp_Val;
        _curr_F = -100;


        if(laneId == 31)
        { // if you are the last thread in your warp then spill your values to shmem
          sh_prev_E[warpId] = _prev_E;
          sh_prev_H[warpId] = _prev_H;
		      sh_prev_prev_H[warpId] = _prev_prev_H;
        }

        if(diag >= maxSize)
        { // if you are invalid in this iteration, spill your values to shmem
          local_spill_prev_E[thread_Id] = _prev_E;
          local_spill_prev_H[thread_Id] = _prev_H;
          local_spill_prev_prev_H[thread_Id] = _prev_prev_H;
        }

        __syncthreads(); // this is needed so that all the shmem writes are completed.

        if(is_valid[thread_Id] && thread_Id < minSize)
        {
          unsigned mask  = __ballot_sync(__activemask(), (is_valid[thread_Id] &&( thread_Id < minSize)));
          short fVal = _prev_F + extendGap;
          short hfVal = _prev_H + startGap;
          short valeShfl = __shfl_sync(mask, _prev_E, laneId- 1, 32);
          short valheShfl = __shfl_sync(mask, _prev_H, laneId - 1, 32);
          short eVal=0, heVal = 0;

          if(diag >= maxSize) // when the previous thread has phased out, get value from shmem
          {
            eVal = local_spill_prev_E[thread_Id - 1] + extendGap;
            heVal = local_spill_prev_H[thread_Id - 1]+ startGap;
          }
          else
          {
            eVal =((warpId !=0 && laneId == 0)?sh_prev_E[warpId-1]: valeShfl) + extendGap;
            heVal =((warpId !=0 && laneId == 0)?sh_prev_H[warpId-1]:valheShfl) + startGap;
          }

           if(warpId == 0 && laneId == 0) // make sure that values for lane 0 in warp 0 is not undefined
           {
              eVal = 0;
              heVal = 0;
           }
      		_curr_F = (fVal > hfVal) ? fVal : hfVal;
          
          if (fVal > hfVal){ //record F value in H_temp 0b00000001
                H_temp = H_temp | 1;
          } else { //record F value in H_ptr 0b00000000
                H_temp = H_temp & (~1);
          }

      		_curr_E = (eVal > heVal) ? eVal : heVal;
          if (j!=0){
            if (eVal > heVal) { //record E value in H_temp 0b00000010
              H_temp = H_temp | 2;
            } else { //record E value in H_temp 0b00000000
              H_temp = H_temp & (~2);
            }
          }
          short testShufll = __shfl_sync(mask, _prev_prev_H, laneId - 1, 32);
          short final_prev_prev_H = 0;
          if(diag >= maxSize)
          {
            final_prev_prev_H = local_spill_prev_prev_H[thread_Id - 1];
          }
          else
          {
            final_prev_prev_H =(warpId !=0 && laneId == 0)?sh_prev_prev_H[warpId-1]:testShufll;
          }

          if(warpId == 0 && laneId == 0) final_prev_prev_H = 0;
          short diag_score = final_prev_prev_H + ((longer_seq[i] == myColumnChar) ? matchScore : misMatchScore);
          _curr_H = findMaxFour(diag_score, _curr_F, _curr_E, 0, &ind);
           //if (thread_Id == 117 ||thread_Id == 118 ||thread_Id == 119 ||thread_Id == 120 ) printf("j = %d, myColumnChar = %c, i = %d, comparChar = %c, _curr_H = %d, _curr_E = %d, _curr_F = %d, diag_score = %d\n",
                          //  j, myColumnChar, i, longer_seq[i], _curr_H, _curr_E, _curr_F, diag_score);
          
          if (ind == 0) { // diagonal cell is max, set bits to 0b00001100
                H_temp = H_temp | 4;     // set bit 0b00000100
                H_temp = H_temp | 8;     // set bit 0b00001000
                //printf("\\");
            } else if (ind == 1) {       // left cell is max, set bits to 0b00001000
                H_temp = H_temp & (~4);  // clear bit
                H_temp = H_temp | 8;     // set bit 0b00001000
                 //printf("-");
            } else if (ind == 2) {       // top cell is max, set bits to 0b00000100
                H_temp = H_temp & (~8);  //clear bit
                H_temp = H_temp | 4;     // set bit 0b00000100
                 //printf("|");
            } else {                     // score is 0, set bits to 0b00000000
                H_temp = H_temp & (~8);  //clear bit
                H_temp = H_temp & (~4);  //clear bit
                 //printf("*");
          }
          H_ptr[diagOffset[diagId] + locOffset] =  H_temp;
          I[diagOffset[diagId] + locOffset] = _curr_H;
          //if (block_Id == 0 && thread_Id == 122) {
            //printf("i = %d, j = %d, diag_score = %d, _curr_F = %d, _curr_E = %d, maxscore = _curr_H = %d, ind = %d\n",
                    //i, j, diag_score, _curr_F, _curr_E, _curr_H, ind);
            //printf("IN LOOP TRACEBACK: i = %d, j = %d, I=%d\n", i, j, I[diagOffset[27]+9]);
          //}
          //I[diagOffset[diagId] + locOffset] = _curr_E;
          //I[diagOffset[diagId] + locOffset] = _curr_F;
      
          thread_max_i = (thread_max >= _curr_H) ? thread_max_i : i;
          thread_max_j = (thread_max >= _curr_H) ? thread_max_j : thread_Id;
          thread_max   = (thread_max >= _curr_H) ? thread_max : _curr_H;
          //if (block_Id == 0)
            //printf("i = %d, j = %d, _curr_H = %d, thread_max_i = %d, thread_max_j = %d, thread_max = %d\n",i, thread_Id, _curr_H, thread_max_i, thread_max_j, thread_max);
          i++;
       }
      __syncthreads(); 
    }
    __syncthreads();

    thread_max = blockShuffleReduce_with_index(thread_max, thread_max_i, thread_max_j, minSize);  // thread 0 will have the correct values
    
    // if(thread_Id == 0) {
    //   printf("thread_max_j = %d, thread_max_i = %d, thread_max = %d\n",thread_max_i, thread_max_j, thread_max);
    // } 
    
    if(thread_Id == 0)
    {
        short current_i = thread_max_i;
        short current_j = thread_max_j;
        //if (block_Id == 0)
          //printf("thread_max_i = %d, thread_max_j = %d\n", thread_max_i, thread_max_j);
        if(lengthSeqA < lengthSeqB)
        {
          seqB_align_end[block_Id] = thread_max_i;
          seqA_align_end[block_Id] = thread_max_j;
          top_scores[block_Id] = thread_max;
        }
        else
        {
          seqA_align_end[block_Id] = thread_max_i;
          seqB_align_end[block_Id] = thread_max_j;
          top_scores[block_Id] = thread_max;
        }
        //printf("BEFORE TRACEBACK: I=%d\n", I[diagOffset[23]+9]);
        //printf("before calling TRACEBACK, BLOCK #%d, current_i = %d, current_j = %d\n", block_Id, current_i, current_j);
        if (DEBUG_PRINT == 1) {
          gpu_bsw::printMatrix(H_ptr, I, seqA, seqB, lengthSeqA, lengthSeqB, diagOffset,maxSize);
        }
        //unsigned short diagId    = current_i + current_j;
        //unsigned short locOffset = 0;

        //if(diagId < maxSize + 1)
        //{ 
            //locOffset = current_j;
        //} else {
            //unsigned short myOff2 = diagId - maxSize;
            //locOffset     = current_j - myOff2;
        //}
        
        gpu_bsw::traceBack(current_i, current_j, seqA_array, seqB_array, prefix_lengthA, 
                    prefix_lengthB, seqA_align_begin, seqA_align_end,
                    seqB_align_begin, seqB_align_end, maxMatrixSize, maxCIGAR,
                    longCIGAR, CIGAR, H_ptr, diagOffset);

    }
    __syncthreads();
}

__global__ void
gpu_bsw::sequence_aa_kernel_traceback(char* seqA_array, char* seqB_array, unsigned* prefix_lengthA,
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, short* top_scores, char* longCIGAR_array, 
                    char* CIGAR_array, char* H_ptr_array, short* I_ptr_array,int maxCIGAR, unsigned const maxMatrixSize, 
                    short startGap, short extendGap, short* scoring_matrix, short* encoding_matrix)
{
  int block_Id  = blockIdx.x;
  int thread_Id = threadIdx.x;
  short laneId = threadIdx.x%32;
  short warpId = threadIdx.x/32;

  unsigned lengthSeqA;
  unsigned lengthSeqB;
  // local pointers
  char*    seqA;
  char*    seqB;
  char* longer_seq;

  char* H_ptr;
  char* CIGAR, *longCIGAR;
  short* I;


  extern __shared__ char is_valid_array[];
  char*                  is_valid = &is_valid_array[0];

// setting up block local sequences and their lengths.
  if(block_Id == 0)
  {
      lengthSeqA = prefix_lengthA[0];
      lengthSeqB = prefix_lengthB[0];
      seqA       = seqA_array;
      seqB       = seqB_array;
  }
  else
  {
      lengthSeqA = prefix_lengthA[block_Id] - prefix_lengthA[block_Id - 1];
      lengthSeqB = prefix_lengthB[block_Id] - prefix_lengthB[block_Id - 1];
      seqA       = seqA_array + prefix_lengthA[block_Id - 1];
      seqB       = seqB_array + prefix_lengthB[block_Id - 1];
  }
  // what is the max length and what is the min length
  unsigned maxSize = lengthSeqA > lengthSeqB ? lengthSeqA : lengthSeqB;
  unsigned minSize = lengthSeqA < lengthSeqB ? lengthSeqA : lengthSeqB;

  H_ptr = H_ptr_array + (block_Id * maxMatrixSize);
  I = I_ptr_array + (block_Id * maxMatrixSize);

  longCIGAR = longCIGAR_array + (block_Id * maxCIGAR);
  CIGAR = CIGAR_array + (block_Id * maxCIGAR);

  unsigned long* diagOffset = (unsigned long*) (&is_valid_array[3 * (minSize + 1) * sizeof(long)]);

 
// shared memory space for storing longer of the two strings
  memset(is_valid, 0, minSize);
  is_valid += minSize;
 
  memset(is_valid, 1, minSize);
  is_valid += minSize;
  
  memset(is_valid, 0, minSize);
   

  char myColumnChar;
  char H_temp = 0;
  // the shorter of the two strings is stored in thread registers
  if(lengthSeqA < lengthSeqB)
  {
    if(thread_Id < lengthSeqA)
      myColumnChar = seqA[thread_Id];  // read only once
    longer_seq = seqB;
  }
  else
  {
    if(thread_Id < lengthSeqB)
      myColumnChar = seqB[thread_Id];
    longer_seq = seqA;
  }

  __syncthreads(); // this is required here so that complete sequence has been copied to shared memory

  int   i            = 0;
  int   j            = thread_Id;
  short thread_max   = 0; // to maintain the thread max score
  short thread_max_i = 0; // to maintain the DP coordinate i for the longer string
  short thread_max_j = 0;// to maintain the DP cooirdinate j for the shorter string
  int ind;

  //set up the prefixSum for diagonal offset look up table for H_ptr, E_ptr, F_ptr
    int    locSum = 0;
     
    for(int diag = 0; diag < lengthSeqA + lengthSeqB - 1; diag++) 
    {
        
        int locDiagId = diag;
        if(thread_Id == 0)
        {
            
            if(locDiagId <= minSize + 1)
            {
                locSum += locDiagId;
                diagOffset[locDiagId] = locSum;
            }
            else if(locDiagId > maxSize + 1)
            {
                locSum += (minSize + 1) - (locDiagId - (maxSize + 1));
                diagOffset[locDiagId] = locSum;
            }
            else
            {
                locSum += minSize + 1;
                diagOffset[locDiagId] = locSum;
            }
            diagOffset[lengthSeqA + lengthSeqB] = locSum + 2; //what is this for?
            
            //printf("diag = %d, diagOffset = %d\n", locDiagId, diagOffset[locDiagId]);
        }
    }
    
    __syncthreads(); //to make sure prefixSum is calculated before the threads start calculations.  


//initializing registers for storing diagonal values for three recent most diagonals (separate tables for
//H, E and F)
  short _curr_H = 0, _curr_F = -100, _curr_E = -100;
  short _prev_H = 0, _prev_F = -100, _prev_E = -100;
  short _prev_prev_H = 0, _prev_prev_F = -100, _prev_prev_E = -100;
  short _temp_Val = 0;

 __shared__ short sh_prev_E[32]; // one such element is required per warp
 __shared__ short sh_prev_H[32];
 __shared__ short sh_prev_prev_H[32];

 __shared__ short local_spill_prev_E[1024];// each threads local spill,
 __shared__ short local_spill_prev_H[1024];
 __shared__ short local_spill_prev_prev_H[1024];

 __shared__ short sh_aa_encoding[ENCOD_MAT_SIZE];// length = 91
 __shared__ short sh_aa_scoring[SCORE_MAT_SIZE];

 int max_threads = blockDim.x;
 for(int p = thread_Id; p < SCORE_MAT_SIZE; p+=max_threads){
    sh_aa_scoring[p] = scoring_matrix[p];
 }
 for(int p = thread_Id; p < ENCOD_MAT_SIZE; p+=max_threads){
   sh_aa_encoding[p] = encoding_matrix[p];
 }

  __syncthreads(); // to make sure all shmem allocations have been initialized

  for(int diag = 0; diag < lengthSeqA + lengthSeqB - 1; diag++)
  {  // iterate for the number of anti-diagonals

      unsigned short diagId    = i + j;
      unsigned short locOffset = 0;
      if(diagId < maxSize + 1) 
      {
          locOffset = j;
      }
      else
      {
        unsigned short myOff = diagId - maxSize;
        locOffset            = j - myOff;
      }

      is_valid = is_valid - (diag < minSize || diag >= maxSize); //move the pointer to left by 1 if cnd true

       _temp_Val = _prev_H; // value exchange happens here to setup registers for next iteration
       _prev_H = _curr_H;
       _curr_H = _prev_prev_H;
       _prev_prev_H = _temp_Val;
       _curr_H = 0;

      _temp_Val = _prev_E;
      _prev_E = _curr_E;
      _curr_E = _prev_prev_E;
      _prev_prev_E = _temp_Val;
      _curr_E = -100;

      _temp_Val = _prev_F;
      _prev_F = _curr_F;
      _curr_F = _prev_prev_F;
      _prev_prev_F = _temp_Val;
      _curr_F = -100;


      if(laneId == 31)
      { // if you are the last thread in your warp then spill your values to shmem
        sh_prev_E[warpId] = _prev_E;
        sh_prev_H[warpId] = _prev_H;
        sh_prev_prev_H[warpId] = _prev_prev_H;
      }

      if(diag >= maxSize)
      { // if you are invalid in this iteration, spill your values to shmem
        local_spill_prev_E[thread_Id] = _prev_E;
        local_spill_prev_H[thread_Id] = _prev_H;
        local_spill_prev_prev_H[thread_Id] = _prev_prev_H;
      }

      __syncthreads(); // this is needed so that all the shmem writes are completed.

      if(is_valid[thread_Id] && thread_Id < minSize)
      {
        unsigned mask  = __ballot_sync(__activemask(), (is_valid[thread_Id] &&( thread_Id < minSize)));

        short fVal = _prev_F + extendGap;
        short hfVal = _prev_H + startGap;
        short valeShfl = __shfl_sync(mask, _prev_E, laneId- 1, 32);
        short valheShfl = __shfl_sync(mask, _prev_H, laneId - 1, 32);

        short eVal=0, heVal = 0;

        if(diag >= maxSize) // when the previous thread has phased out, get value from shmem
        {
          eVal = local_spill_prev_E[thread_Id - 1] + extendGap;
          heVal = local_spill_prev_H[thread_Id - 1]+ startGap;
        }
        else
        {
          eVal =((warpId !=0 && laneId == 0)?sh_prev_E[warpId-1]: valeShfl) + extendGap;
          heVal =((warpId !=0 && laneId == 0)?sh_prev_H[warpId-1]:valheShfl) + startGap;
        }

         if(warpId == 0 && laneId == 0) // make sure that values for lane 0 in warp 0 is not undefined
         {
            eVal = 0;
            heVal = 0;
         }
        _curr_F = (fVal > hfVal) ? fVal : hfVal;

        if (fVal > hfVal){
                H_temp = H_temp | 1;
        } else {
                H_temp = H_temp & (~1);
        }

        _curr_E = (eVal > heVal) ? eVal : heVal;

        if (j!=0){
            if (eVal > heVal) {
              H_temp = H_temp | 2;
            } else {
              H_temp = H_temp & (~2);
            }
        }

        short testShufll = __shfl_sync(mask, _prev_prev_H, laneId - 1, 32);
        short final_prev_prev_H = 0;
        if(diag >= maxSize)
        {
          final_prev_prev_H = local_spill_prev_prev_H[thread_Id - 1];
        }
        else
        {
          final_prev_prev_H =(warpId !=0 && laneId == 0)?sh_prev_prev_H[warpId-1]:testShufll;
        }


        if(warpId == 0 && laneId == 0) final_prev_prev_H = 0;

        short mat_index_q = sh_aa_encoding[(int)longer_seq[i]];//encoding_matrix
        short mat_index_r = sh_aa_encoding[(int)myColumnChar];

        short add_score = sh_aa_scoring[mat_index_q*24 + mat_index_r]; // doesnt really matter in what order these indices are used, since the scoring table is symmetrical

         //if (block_Id == 0 && thread_Id == 6){
         // printf("myColumnChar = %c\n",myColumnChar);
         // printf("long_seq[i-1] = %c\n",longer_seq[i]);
         // printf("q = %d, r = %d, score = %d\n", mat_index_q, mat_index_r, add_score);
         //}

        short diag_score = final_prev_prev_H + add_score;

        _curr_H = findMaxFour(diag_score, _curr_F, _curr_E, 0, &ind);

        if (ind == 0) {
                H_temp = H_temp | 4;
                H_temp = H_temp | 8;
        } else if (ind == 1) {
                H_temp = H_temp & (~4);
                H_temp = H_temp | 8;           
        } else if (ind == 2) {
                H_temp = H_temp & (~8);
                H_temp = H_temp | 4;
        } else {
                H_temp = H_temp & (~8);
                H_temp = H_temp & (~4);
        }
        H_ptr[diagOffset[diagId] + locOffset] =  H_temp;
        I[diagOffset[diagId] + locOffset] = _curr_H;

        thread_max_i = (thread_max >= _curr_H) ? thread_max_i : i;
        thread_max_j = (thread_max >= _curr_H) ? thread_max_j : thread_Id;
        thread_max   = (thread_max >= _curr_H) ? thread_max : _curr_H;
        i++;
     }

    __syncthreads(); 

  }
  __syncthreads();

  thread_max = blockShuffleReduce_with_index(thread_max, thread_max_i, thread_max_j,
                                  minSize);  // thread 0 will have the correct values

  if(thread_Id == 0)
  {
      short current_i = thread_max_i;
      short current_j = thread_max_j;      

      if(lengthSeqA < lengthSeqB)
      {
        seqB_align_end[block_Id] = thread_max_i;
        seqA_align_end[block_Id] = thread_max_j;
        top_scores[block_Id] = thread_max;
      }
      else
      {
      seqA_align_end[block_Id] = thread_max_i;
      seqB_align_end[block_Id] = thread_max_j;
      top_scores[block_Id] = thread_max;
      }

      if (DEBUG_PRINT == 1) {
        gpu_bsw::printMatrix(H_ptr, I, seqA, seqB, lengthSeqA, lengthSeqB, diagOffset, maxSize);
      }

      // unsigned short diagId    = current_i + current_j;
      // unsigned short locOffset = 0;

      // if(diagId < maxSize + 1)
      // {
      //     locOffset = current_j;
      // } else {
      //     unsigned short myOff2 = diagId - maxSize;
      //     locOffset     = current_j - myOff2;
      // }
        //printf("diagId = %d, locOffset = %d, diagOffset[diagId] + locOffset = %d\n", diagId, locOffset,diagOffset[diagId] + locOffset );
        //printf("H_ptr[] = %c\n", H_ptr[diagOffset[diagId] + locOffset]);
      gpu_bsw::traceBack(current_i, current_j, seqA_array, seqB_array, prefix_lengthA, 
                    prefix_lengthB, seqA_align_begin, seqA_align_end,
                    seqB_align_begin, seqB_align_end, maxMatrixSize, maxCIGAR,
                    longCIGAR, CIGAR, H_ptr, diagOffset);


  }
  __syncthreads();
}

