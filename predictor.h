/* Author: Mark Faust
 *
 * C version of predictor file
*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN
#define LOCAL_HISTORY_SIZE  1024
#define GLOBAL_HISTORY_SIZE  4096

#define GET_ADDR(PC) ((PC & 0XFFC) >> 2)
#define GET_LOCAL_PRED(PREDICTION) ((0x4 & PREDICTION) >>2) 

#include <cstddef>
#include <cstring>
#include <inttypes.h>
#include <vector>
#include "op_state.h"   // defines op_state_c (architectural state) class 
#include "tread.h"      // defines branch_record_c class


class PREDICTOR
{
public:
    //initialising data structures
    uint16_t path_history = 0; 
    uint16_t local_history_table[LOCAL_HISTORY_SIZE] = {0};
    uint16_t local_prediction_table[LOCAL_HISTORY_SIZE] = {0};
    uint16_t global_prediction_table[GLOBAL_HISTORY_SIZE] = {0};
    uint8_t choice_prediction_table[GLOBAL_HISTORY_SIZE] = {0};

    bool alpha_prediction = false;
    bool local_prediction = false;
    bool global_prediction = false;

    //prototypes
    bool get_prediction(const branch_record_c* br, const op_state_c* os);
    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken);
    
    //Functions for alpha predictor
    //GLOBAL PREDICTOR FUNCTIONS
    void update_global_prediction_table(uint16_t global_prediction_index,bool taken);
    void update_global_predictor(const branch_record_c* br,bool taken);

    //updating choice prediction table
    void update_choice_prediction_table(uint16_t global_prediction_index, bool taken);
    //LOCAL PREDICTOR FUNCTIONS
    void update_local_history_table(uint16_t addr, bool taken);
    void update_local_prediction_table(uint16_t local_prediction_index, bool taken);
    void update_local_predictor(const branch_record_c* br, bool taken);

};

#endif 

