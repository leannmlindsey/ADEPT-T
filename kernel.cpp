#include <kernel.hpp>

__inline__ __device__ short
gpu_bsw::warpReduceMax_with_index_reverse(short val, short& myIndex, short& myIndex2, unsigned lengthSeqB)
{
    short   warpSize = 32;
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
    int              laneId = threadIdx.x % 32; //index of the lane within the warp
    int              warpId = threadIdx.x / 32; //index of the warp
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
  
    return cigar_position;
}

__device__ void
gpu_bsw::createCIGAR(char* longCIGAR, char* CIGAR, int maxCIGAR, 
        const char* seqA, const char* seqB, unsigned lengthShorterSeq, unsigned lengthLongerSeq, 
        bool seqBShorter, short first_j, short last_j, short first_i, short last_i) 
{
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
gpu_bsw::traceBack(short current_i, short current_j, char* seqA_array, char* seqB_array, unsigned* prefix_lengthA, 
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, unsigned const maxMatrixSize, int maxCIGAR,
                    char* longCIGAR, char* CIGAR, char* H_ptr, uint32_t* diagOffset)
{     
    
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
    bool seqBShorter = lengthSeqA < lengthSeqB ? false : true;  //need to keep track of whether query or ref is shorter for I or D in CIGAR 

    current_diagId    = current_i + current_j;
    current_locOffset = 0;
    if(current_diagId < maxSize)
    {
        current_locOffset = current_j;
    }
    else
    {
        unsigned short myOff = current_diagId - maxSize+1;
        current_locOffset    = current_j - myOff;
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
       //if (myId == 0){
        //printf("current_i:%d, current_j:%d, %c , %c, %d\n",current_i, current_j, shorterSeq[current_j], longerSeq[current_i],counter);
       //}

        //write the current value into longCIGAR then assign next_i
        if (matrix == 'H') { 
            
            switch (temp_H & 0b00001100){    
                case 0b00001100 :
                    matrix = 'H';
                    longCIGAR[counter] = shorterSeq[current_j] == longerSeq[current_i] ? '=' : 'X';
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j - 1;
                    break;
                case 0b00001000 :
                    matrix = 'F';
                    next_i = current_i;
                    next_j = current_j;
                    break;
                case 0b00000100 :
                    matrix = 'E';
                    next_i = current_i;
                    next_j = current_j;
                    break;
                 case 0b00000000 : 
                    continueTrace = false;
                    break;
            }
        } else if (matrix == 'E'){
            switch (temp_H & 0b00000010){
                case 0b00000010 :
		                longCIGAR[counter] = seqBShorter ? 'I' : 'D';
                    counter++;
                    next_i = current_i;
                    next_j = current_j - 1;
                    break;
                case 0b00000000 :
                    matrix = 'H';
                    longCIGAR[counter] = seqBShorter ? 'I' : 'D';
                    counter++;
                    next_i = current_i;
                    next_j = current_j - 1;
                    break;
            }
        } else if (matrix == 'F'){
            switch (temp_H & 0b00000001){
                case 0b00000001 :
		                longCIGAR[counter] = seqBShorter ? 'D' : 'I';
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j;
                    break;
                case 0b00000000 :
                    matrix = 'H';
                    longCIGAR[counter] = seqBShorter ? 'D' : 'I';
                    counter++;
                    next_i = current_i - 1;
                    next_j = current_j;
                    break;
            }
        }

        //  if(myId==0 && myTId ==0) {
        //      for (int i = 0; i <= counter; i++){
        //          printf("%c",longCIGAR[i]);
        //      }
        //      printf("\n");
        //  }  
        // if(myId==0 && myTId ==0) {
        //     printf("     %c\n",longCIGAR[counter]);
        // }
        
        if (continueTrace != false){
          prev_i = current_i; //record current values in case this is the stop location
          prev_j = current_j;

          current_i = next_i;
          current_j = next_j;

          current_diagId    = current_i + current_j;
          current_locOffset = 0;

          if(current_diagId < maxSize)
          {
            current_locOffset = current_j;
          } else {
            unsigned short myOff2 = current_diagId - maxSize+1;
            current_locOffset     = current_j - myOff2;
          }
        //counter++;   
        }
  }
   //handle edge cases
   if ((current_i == 0) || (current_j == 0)) {

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
    //          for (int i = 0; i <= counter; i++){
    //              printf("%c",longCIGAR[i]);
    //          }
    //          printf("\n\n");
    //      }  
    //      if(myId==0 && myTId ==0) {
    //          printf("current_i:%d, current_j:%d, %c , %c, %d\n",current_i, current_j, shorterSeq[current_j], longerSeq[current_i], counter);
    //      } 
   
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
}

__global__ void
gpu_bsw::sequence_dna_kernel_traceback(char* seqA_array, char* seqB_array, unsigned* prefix_lengthA,
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, short* top_scores, 
                    char* longCIGAR_array, char* CIGAR_array, char* H_ptr_array, 
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

    longCIGAR = longCIGAR_array + (block_Id * maxCIGAR);
    CIGAR = CIGAR_array + (block_Id * maxCIGAR);

     
    char* longer_seq;
    uint32_t* diagOffset = (uint32_t*) (&is_valid_array[3 * (minSize + 1) * sizeof(uint32_t)]); //FIXME: this is too far ahead of the is_valid array but using char gives misaligned address error
    

// shared memory space for storing longer of the two strings
    memset(is_valid, 0, minSize/STRIDE+1);
    is_valid += minSize/STRIDE+1;
    memset(is_valid, 1, minSize/STRIDE+1);
    is_valid += minSize/STRIDE+1;
    memset(is_valid, 0, minSize/STRIDE+1);

    //if(thread_Id == 0)printf("minSize:%d, STRIDE:%d, minSize+1/STRIDE:%d\n", minSize, STRIDE, (minSize+1)/STRIDE);    
    char myColumnChar[STRIDE];

    // the shorter of the two strings is stored in thread registers
    char H_temp = 0;  //temp value of H for entire STRIDE is stored in register until H, E and F are set then written to global; set all bits to 0 initially

    //for(int q = 0; q<lengthSeqA; q++){
    // if(thread_Id == 0) printf("%c",seqA[q]);
      
    //}
    //if(thread_Id == 0)printf("\n");
    //for(int q = 0; q<lengthSeqB; q++){
    //  if(thread_Id == 0) printf("%c",seqB[q]);
      
    //}
    //if(thread_Id == 0)printf("\n");
    if(lengthSeqA < lengthSeqB) {
      if(thread_Id < (lengthSeqA/STRIDE)) {
        for(int k=0; k<STRIDE; k++) {
          myColumnChar[k] = seqA[thread_Id*STRIDE+k];
        }
        if(lengthSeqA%STRIDE != 0 && thread_Id == (lengthSeqA/STRIDE)) {//in the last thread
            for(int k=0; k<lengthSeqA%STRIDE;k++){
              myColumnChar[k] = seqA[thread_Id*STRIDE+k];
            }
        }
        longer_seq = seqB;
      }
    }
    else
    {
        if(thread_Id < (lengthSeqB/STRIDE)) {
          for(int k=0; k<STRIDE; k++) {
            myColumnChar[k] = seqB[thread_Id*STRIDE+k]; 
          } 
        }
          //zero out chars not in this sequence
        if(lengthSeqB%STRIDE != 0 && thread_Id == (lengthSeqB/STRIDE)) {//in the last thread
            for(int k=0; k<lengthSeqB%STRIDE;k++){
              myColumnChar[k] = seqB[thread_Id*STRIDE+k];
            }
        }
          longer_seq = seqA;
        
    }
    __syncthreads(); // this is required here so that complete sequence has been copied to shared memory

    int   i            = 0;
    int   j            = thread_Id;
    short thread_max   = 0; // to maintain the thread max score
    short thread_max_i = 0; // to maintain the DP coordinate i for the longer string
    short thread_max_j = 0;// to maintain the DP cooirdinate j for the shorter string
    int ind;

//set up the prefixSum for diagonal offset look up table for H_ptr
    int    locSum = 0;
    
    //create prefixSum table by cycling through the threads in batches

    for (int cyc = 0; cyc <= (lengthSeqA + lengthSeqB+1)/minSize + 1; cyc++){
      
      int locDiagId = thread_Id+cyc*minSize;
      if (locDiagId < lengthSeqA + lengthSeqB ){
        if(locDiagId <= minSize){
          locSum = (locDiagId) * (locDiagId + 1)/2;
          diagOffset[locDiagId]= locSum;
          //printf("LEFT CORNER inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
        else if (locDiagId > maxSize + 1){
          int n = (maxSize+minSize) - locDiagId-1;
          int finalcell = (maxSize) * (minSize)+1;
          locSum = finalcell - n*(n+1)/2;
          diagOffset[locDiagId] = locSum;
          //printf("RIGHT CORNER inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
        else {
          locSum = ((minSize)*(minSize+1)/2) +(minSize)*(locDiagId-minSize);
          diagOffset[locDiagId] = locSum;
          //printf("MIDDLE SECTION inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
      }
    }
     __syncthreads(); //to make sure prefixSum is calculated before the threads start calculations.    

  //initializing registers for storing diagonal values for three recent most diagonals (separate tables for H, E, F
    short _curr_H[STRIDE], _curr_F[STRIDE], _curr_E = -100; //-100 acts as neg infinity
    short _prev_H[STRIDE], _prev_F[STRIDE], _prev_E = -100;
    short _prev_prev_H = 0;
    
   //if(thread_Id == 0)printf("_curr_H:%d, _curr_F:%d, _curr_E:%d",_curr_H, _curr_F[1], _curr_E);
   //if(thread_Id == 0)printf("_prev_H:%d, _prev_F:%d, _prev_E:%d",_prev_H, _prev_F[1], _prev_E);
   //if(thread_Id == 0)printf("_prev_prev_H:%d",_prev_prev_H);
   __shared__ short sh_prev_E[32]; // one such element is required per warp 
   __shared__ short sh_prev_H[32];
   __shared__ short sh_prev_prev_H[32];

   __shared__ short local_spill_prev_E[1024];// each threads local spill,
   __shared__ short local_spill_prev_H[1024];
   __shared__ short local_spill_prev_prev_H[1024];

  //initialize arrays 
  if(thread_Id == 0){
      for (int k = 0; k < STRIDE;k++){
        _curr_F[k] = -100;
        _curr_H[k] = 0;

        _prev_F[k] = -100;
        _prev_H[k] = 0;
      }
        sh_prev_E[0] = 0;
        sh_prev_H[0] = 0;
        sh_prev_prev_H[0] = 0;

        local_spill_prev_E[0] = 0;
        local_spill_prev_H[0] = 0;
        local_spill_prev_prev_H[0] = 0;
    }
   


    __syncthreads(); // to make sure all shmem allocations have been initialized

    
    for(int diag = 0; diag < minSize/STRIDE + maxSize; diag++)
    {  // iterate for the number of anti-diagonals
        //if(thread_Id==0)printf("DIAG = %d\n", diag); 
        //if(thread_Id==0)printf("\n");
        unsigned short diagId    = i + j;
        unsigned short locOffset = 0;
        if(diagId < maxSize) 
        {
            locOffset = j;
        }
        else
        {
          unsigned short myOff = diagId - maxSize+1;
          locOffset            = j - myOff;
        }

        //if (thread_Id == 0)printf("diag:%d, diagId:%d, locOffset:%d, i:%d, j:%d\n",diag, diagId, locOffset, i, j);

        is_valid = is_valid - (diag < (minSize/STRIDE)+1 || diag >= maxSize); //move the pointer to left by 1 if cnd true       
      
        __syncthreads(); // this is needed so that all the shmem writes are completed.

        unsigned mask  = __ballot_sync(__activemask(), (is_valid[thread_Id] &&( thread_Id < minSize)));
        int fVal=0, hfVal=0, eVal=0, heVal=0, final_prev_prev_H = 0;
        int diag_score = 0; //if this is not an int, the addition fails (with short declaration), even with 3+0, I don't know why

        if(is_valid[thread_Id] && thread_Id < minSize)
        {
          //printf("thread_Id = %d, PREV_E = %d\n",thread_Id,_prev_E);
          int valeShfl = __shfl_sync(mask, _prev_E, laneId- 1, 32);
          int valheShfl = __shfl_sync(mask, _prev_H[STRIDE-1], laneId - 1, 32);
          int valhShfl = __shfl_sync(mask, _prev_prev_H, laneId - 1, 32);
          
          //Iterate over the STRIDE
          for(int k=0; k<STRIDE; k++ )  {
            fVal = _prev_F[k] + extendGap;
            hfVal = _prev_H[k] + startGap;
            
            //get the prev_E, prev_H, and prev_prev_H values from the previous thread needed to calculate curr_E and curr_H
            if(diag >= maxSize) // when the previous thread has phased out, get value from shmem
            {
              //printf("diag >=maxsize, thread_Id = %d, getting eVal = %d from local spill\n",thread_Id, eVal);
              if(k==0){
                eVal = local_spill_prev_E[thread_Id - 1] + extendGap;
                heVal = local_spill_prev_H[thread_Id - 1]+ startGap;
                final_prev_prev_H = local_spill_prev_prev_H[thread_Id - 1];
              }
              else {
                eVal = _prev_E + extendGap;
                heVal = _curr_H[k-1] + startGap;
                final_prev_prev_H = _prev_H[k-1];
              }
            }
            else
            { 
              if(k==0){
                
                eVal =((warpId !=0 && laneId == 0)?sh_prev_E[warpId-1]:valeShfl) + extendGap;
                heVal =((warpId !=0 && laneId == 0)?sh_prev_H[warpId-1]:valheShfl) + startGap;
                final_prev_prev_H =(warpId !=0 && laneId == 0)?sh_prev_prev_H[warpId-1]:valhShfl;
              } else {
              
                eVal =((warpId !=0 && laneId == 0)?sh_prev_E[warpId-1]: _prev_E) + extendGap;
                heVal =((warpId !=0 && laneId == 0)?sh_prev_H[warpId-1]:_curr_H[k-1]) + startGap;
                final_prev_prev_H =(warpId !=0 && laneId == 0)?sh_prev_prev_H[warpId-1]:_prev_H[k-1];
              }
            }
           
            //calculate the values for curr_F
            fVal = _prev_F[k] + extendGap;
            hfVal = _prev_H[k] + startGap;
            //if(thread_Id == 0)printf("fVal = %d\n",fVal);
            if (fVal > hfVal){ //record F value in H_temp 0b00000001
                  H_temp = H_temp | 1;
                  _curr_F[k] = fVal;
            } else { //record F value in H_temp 0b00000000
                  H_temp = H_temp & (~1);
                  _curr_F[k] = hfVal;
            }

            if (eVal > heVal) { //record E value in H_temp 0b00000010
                H_temp = H_temp | 2;
                _curr_E = eVal;
            } else { //record E value in H_temp 0b00000000
                H_temp = H_temp & (~2);
                _curr_E = heVal;
            }
            if(thread_Id == 0 && k == 0) eVal = -101;
            //calculate the values for curr_H
            diag_score = ((longer_seq[i] == myColumnChar[k]) ? matchScore : misMatchScore) + final_prev_prev_H;
            _curr_H[k] = findMaxFour(diag_score, _curr_F[k], _curr_E, 0, &ind);
           
            //if(thread_Id == 1)printf("%c, %c, eVal = %d, heVal = %d, _curr_E = %d, fVal = %d, hfVal = %d, _curr_F = %d, final_prev_prev_H = %d, diag_score = %d, MAX--> _curr_H = %d\n", 
                         //longer_seq[i],myColumnChar[k],eVal, heVal, _curr_E, fVal, hfVal, _curr_F[k], final_prev_prev_H, diag_score, _curr_H[k]);
            //if(thread_Id == 0)printf("%d\t", _curr_H[k]);
          
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

            // value exchanges happens here to set up registers for the next iteration
            
            //shuffle the reg values to prepare for next iteration 
            _prev_E = _curr_E;

            if(laneId == 31)
            { // if you are the last thread in your warp then spill your values to shmem
              sh_prev_E[warpId] = _prev_E;
              sh_prev_H[warpId] = _prev_H[STRIDE-1];
		          sh_prev_prev_H[warpId] = _prev_prev_H;
            }

            if(diag >= maxSize)
            { // if you are invalid in this iteration, spill your values to shmem
              //printf("spilling values to shared memory, thread_Id = %d, _prev_E = %d\n", thread_Id, _prev_E);
              local_spill_prev_E[thread_Id] = _prev_E;
              local_spill_prev_H[thread_Id] = _prev_H[STRIDE-1];
              local_spill_prev_prev_H[thread_Id] = _prev_prev_H;
            }

            //write H values to global memory
            //if loop for if thread_Id is even or odd
            if(thread_Id%2 == 0)
            {
              //bit shift H_temp[k] by 4 bits to the left so that even threads can write the upper 4 bits
               H_temp = H_temp << 4;
            }
            else
            {
              //H_temp is complete so write it to global memory
               //H_ptr[diagOffset[diagId] + locOffset + k] =  H_temp;
            }
            
            thread_max_i = (thread_max >= _curr_H[k]) ? thread_max_i : i;
            thread_max_j = (thread_max >= _curr_H[k]) ? thread_max_j : thread_Id*STRIDE+k; 
            thread_max   = (thread_max >= _curr_H[k]) ? thread_max : _curr_H[k];
           
            //if(thread_Id==0)printf("thread_max_i: %d, thread_max_j: %d, thread_max: %d\n", thread_max_i, thread_max_j, thread_max);
          }
          
          _prev_prev_H = _prev_H[STRIDE-1];
          _prev_E = _curr_E;
          for (int k=0; k< STRIDE; k++){
            _prev_H[k] = _curr_H[k];
            _prev_F[k] = _curr_F[k];
          }
          i++;
       }
       //if(thread_Id == 0)printf("END OF LOOP: thread_Id = %d, i = %d, j = %d, thread_max = %d, thread_max_i = %d, thread_max_j = %d\n", thread_Id, i, j, thread_max, thread_max_i, thread_max_j);
      __syncthreads(); 
    }
    __syncthreads();

    thread_max = blockShuffleReduce_with_index(thread_max, thread_max_i, thread_max_j, minSize);  // thread 0 will have the correct values
    //if(thread_Id==0)printf("thread_max_i: %d, thread_max_j: %d, thread_max: %d\n", thread_max_i, thread_max_j, thread_max);
    //write the max score and end positions to results array and call traceback to get CIGAR and start positions
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
        
        //gpu_bsw::traceBack(current_i, current_j, seqA_array, seqB_array, prefix_lengthA, 
        //            prefix_lengthB, seqA_align_begin, seqA_align_end,
        //            seqB_align_begin, seqB_align_end, maxMatrixSize, maxCIGAR,
        //            longCIGAR, CIGAR, H_ptr, diagOffset);

    }
    __syncthreads();
}

__global__ void
gpu_bsw::sequence_aa_kernel_traceback(char* seqA_array, char* seqB_array, unsigned* prefix_lengthA,
                    unsigned* prefix_lengthB, short* seqA_align_begin, short* seqA_align_end,
                    short* seqB_align_begin, short* seqB_align_end, short* top_scores, char* longCIGAR_array, 
                    char* CIGAR_array, char* H_ptr_array, int maxCIGAR, unsigned const maxMatrixSize, 
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

  longCIGAR = longCIGAR_array + (block_Id * maxCIGAR);
  CIGAR = CIGAR_array + (block_Id * maxCIGAR);

  uint32_t* diagOffset = (uint32_t*) (&is_valid_array[3 * (minSize + 1) * sizeof(uint32_t)]);

 
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
  
  for (int cyc = 0; cyc <= (lengthSeqA + lengthSeqB+1)/minSize + 1; cyc++){
      
      int locDiagId = thread_Id+cyc*minSize;
      if (locDiagId < lengthSeqA + lengthSeqB ){
        if(locDiagId <= minSize){
          locSum = (locDiagId) * (locDiagId + 1)/2;
          diagOffset[locDiagId]= locSum;
          //printf("LEFT CORNER inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
        else if (locDiagId > maxSize + 1){
          int n = (maxSize+minSize) - locDiagId-1;
          int finalcell = (maxSize) * (minSize)+1;
          locSum = finalcell - n*(n+1)/2;
          diagOffset[locDiagId] = locSum;
          //printf("RIGHT CORNER inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
        else {
          locSum = ((minSize)*(minSize+1)/2) +(minSize)*(locDiagId-minSize);
          diagOffset[locDiagId] = locSum;
          //printf("MIDDLE SECTION inside loop thread_Id = %d cyc = %d locSum = %d locDiagId = %d\n", thread_Id, cyc, locSum, locDiagId);
        }
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
      if(diagId < maxSize) 
      {
          locOffset = j;
      }
      else
      {
        unsigned short myOff = diagId - maxSize+1;
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

      gpu_bsw::traceBack(current_i, current_j, seqA_array, seqB_array, prefix_lengthA, 
                    prefix_lengthB, seqA_align_begin, seqA_align_end,
                    seqB_align_begin, seqB_align_end, maxMatrixSize, maxCIGAR,
                    longCIGAR, CIGAR, H_ptr, diagOffset);


  }
  __syncthreads();
}


