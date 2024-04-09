/* Author: Mark Faust;
 * Description: This file defines the two required functions for the branch predictor.
 */

#include "predictor.h"

// Create bitmask at position X
#define BIT(X)          (1<<(X))

// Set bit in X to 1 at position Y
#define BIT_SET(X,Y)    (X |= BIT(Y))

// Get bit at position Y in bitfield X
#define BIT_GET(X,Y)    ((X >> Y) & 1)

// Shift X left by Y
#define BIT_SL(X,Y)     (X << Y)

// Hashing function to convert program counter into index of local hist. table
#define LHT_HASH(pc)    (pc & (BIT(10) - 1))

// Hashing function to convert LHT entry to index of local prediction table
#define LPT_HASH(lh)    (lh & (BIT(10) - 1))

// Get 12 lowest bits of phist to use as an index for the global/chooser tables
#define GPT_BITS(phist) (phist & 0xFFF)

// path history (12 bit history) initialized to not taken
uint16_t phist = 0;

// local history table (10 bit histories) all initialized to not taken
// C/C++ initializer lists, see: https://stackoverflow.com/a/629063
uint16_t lht[1024] = {0};

// global predictor table (2 bit saturating counters) initialized to strongly not taken
// C/C++ initializer lists, see: https://stackoverflow.com/a/629063
uint8_t gpt[4096] = {0};

// local prediction table (3 bit saturating counters) initialized to strongly not taken
// C/C++ initializer lists, see: https://stackoverflow.com/a/629063
uint8_t lpt[1024] = {0};

// choice prediction or chooser (2 bit saturating counters) initialized to strongly global
// C/C++ initializer lists, see: https://stackoverflow.com/a/629063
uint8_t chooser[4096] = {0};

// check global predictor table for how likely to take branch
bool check_gpt() {
    if (gpt[GPT_BITS(phist)] >= 2) {
        return true;
    }
    return false;
}

// check local predictor table for how likely to take branch
bool check_lpt(uint16_t lh) {
    if (lpt[LPT_HASH(lh)] >= 4) {
        return true;
    }
    return false;
}

// check chooser table for which predictor, if true then use local predictor
// if false, then use the global predictor
bool choose() {
    if (chooser[GPT_BITS(phist)] >= 2) {
        return true;
    }
    return false;
}

// check local predictor
bool check_local(unsigned int instr_addr) {
    // Get the history for this instruction address
    return check_lpt(lht[LHT_HASH(instr_addr)]);
}

// update local prediction table saturating counter for local history index
void update_lpt(uint16_t lh, bool taken) {
    // saturating counter, if <3 increment until 3 if taken
    // otherwise, decrement until 0 if not taken
    if (lpt[LPT_HASH(lh)] < 7 && taken) {
        lpt[LPT_HASH(lh)]++;
    } else if (lpt[LPT_HASH(lh)] > 0 && !taken) {
        lpt[LPT_HASH(lh)]--;
    }
}

// update local history table for most recent choice wrt instruction
void update_lht(unsigned int instr_addr, bool taken) {
    // first update the local predictor table
    update_lpt(lht[LHT_HASH(instr_addr)], taken);
    // shift left the lht entry by one
    lht[LHT_HASH(instr_addr)] = BIT_SL(lht[LHT_HASH(instr_addr)], 1);
    // if taken, update bitfield's LSB
    if (taken) {
        BIT_SET(lht[LHT_HASH(instr_addr)], 0);
    }
}

// update path history for most recent choice wrt instruction
void update_phist(bool taken) {
    // shift left the lht entry by one
    phist = BIT_SL(phist, 1);
    // if taken, update bitfield's LSB
    if (taken) {
        BIT_SET(phist, 0);
    }
}

// update global predictor table entry for path history index
void update_gpt(bool taken) {
    // saturating counter, if <3 increment until 3 if taken
    // otherwise, decrement until 0 if not taken
    if (gpt[GPT_BITS(phist)] < 3 && taken) {
        gpt[GPT_BITS(phist)]++;
    } else if (gpt[GPT_BITS(phist)] > 0 && !taken) {
        gpt[GPT_BITS(phist)]--;
    }
}

// update the chooser table based on the result
void update_chooser(unsigned int instr_addr, bool taken) {
    // call >=2 local used, <2 global used
    uint8_t choice = chooser[GPT_BITS(phist)];
    // global prediction
    bool gpredict = check_gpt();
    // local prediction
    bool lpredict = check_local(instr_addr);
    // if our local is rigpt, and our global is wrong
    if (lpredict == taken && gpredict != taken && choice < 3) {
        chooser[GPT_BITS(phist)]++;
    }
    // if our local is wrong, and our global is rigpt
    else if (lpredict != taken && gpredict == taken && choice > 0) {
        chooser[GPT_BITS(phist)]--;
    }
}


// Make a prediction on whether a branch is taken or not
bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os) {
    bool prediction = false;

    if (choose()) {
        // use local history table to make prediction
        if (check_local(br->instruction_addr)) {
            prediction = true;
        }
    } else {
        // use global history table to make prediction
        if (check_gpt()) {
            prediction = true;
        }
    }

    // Our predictor

    return prediction;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken) {

    // update the chooser table
    update_chooser(br->instruction_addr, taken);
    // update local history table and local predictor table
    update_lht(br->instruction_addr, taken);
    if (br->is_conditional) {
        // update global predictor table if the branch is conditional
        update_gpt(taken);
    }
    // update path history
    update_phist(taken);

}