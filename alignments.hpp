#ifndef ALIGNMENTS_HPP
#define ALIGNMENTS_HPP
#include"utils.hpp"

class gpu_alignments{
    public:
    short* ref_start_gpu;
    short* ref_end_gpu;
    short* query_start_gpu;
    short* query_end_gpu;
    short* scores_gpu;
    unsigned* offset_ref_gpu;
    unsigned* offset_query_gpu;

    gpu_alignments(int max_alignments);
    ~gpu_alignments();
};


#endif