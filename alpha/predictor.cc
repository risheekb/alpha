/* Author: Risheek Bharadwaj,Shreekara Murthy;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"

    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {
		/* replace this code with your own */
            if(!br->is_conditional)
            {
                alpha_prediction = true;
                return alpha_prediction;
            }
            
            uint16_t addr;
            uint16_t local_prediction_index;
            uint8_t local_prediction_value;
            uint16_t global_prediction_index;
            uint8_t choice_prediction;

            //LOCAL PREDICTOR
            addr = GET_ADDR(br->instruction_addr); //get the [11:2] bits from the program counter
            local_prediction_index = 0x3FF & local_history_table[addr];
            local_prediction_value = local_prediction_table[local_prediction_index];
            local_prediction = GET_LOCAL_PRED(local_prediction_value);

            //GLOBAL PREDICTOR
            global_prediction_index = (0xFFF & path_history);
            global_prediction = (global_prediction_table[global_prediction_index] & 0x3) >> 1;
            choice_prediction = choice_prediction_table[global_prediction_index];

            if(choice_prediction < 2) 
                alpha_prediction = global_prediction;
            else
                alpha_prediction = local_prediction;

            return alpha_prediction;   // true for taken, false for not taken
        }


    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
	 	/* replace this code with your own */
            update_local_predictor(br, taken);
            update_global_predictor(br,taken);
	    }
    

    void PREDICTOR::update_local_predictor(const branch_record_c* br,bool taken)
    {
        uint16_t addr = GET_ADDR(br->instruction_addr);
        uint16_t local_prediction_index = 0x3FF & local_history_table[addr]; 
        update_local_prediction_table(local_prediction_index, taken);
        update_local_history_table(addr,taken);
    }

    void PREDICTOR::update_local_prediction_table(uint16_t local_prediction_index, bool taken)
    {
        if(taken && local_prediction_table[local_prediction_index] < 7)
            local_prediction_table[local_prediction_index]++;

        else if(local_prediction_table[local_prediction_index] > 0)
            local_prediction_table[local_prediction_index]--;

    }
    void PREDICTOR::update_local_history_table(uint16_t addr, bool taken)
    {
        if(taken)
            local_history_table[addr] = (local_history_table[addr] << 1) | 0x1;
        else
            local_history_table[addr] = local_history_table[addr] << 1;
    }

    void PREDICTOR::update_global_predictor(const branch_record_c* br, bool taken)
    {
        uint16_t global_prediction_index = (0xFFF & path_history);
        update_global_prediction_table(global_prediction_index,taken);
        update_choice_prediction_table(global_prediction_index,taken);
        path_history = taken ? (path_history << 1) | 0x1 : (path_history << 1); //update path history
        
    }

    void PREDICTOR::update_global_prediction_table(uint16_t global_prediction_index, bool taken)
    {
        if(taken && (global_prediction_table[global_prediction_index] < 3))
        {
            global_prediction_table[global_prediction_index]++;
        }
        else if(global_prediction_table[global_prediction_index] > 0)
        {
            global_prediction_table[global_prediction_index]--;
        }
    }
    void PREDICTOR::update_choice_prediction_table(uint16_t global_prediction_index,bool taken)
    {
        /*
            choice prediction
            00 -> Strongly Use GLOBAL (SG)
            01 -> Weakly Use GLOBAL   (WG)
            10 -> Weakly Use LOCAL    (WL)
            11 -> Strongly Use LOCAL  (SL)

            00  ->  01  -> 10 -> 11 
            SG -- WG -- WL -- SL
            00  <- 01  <- 10  <- 11                
        */
        switch(choice_prediction_table[global_prediction_index])
        {
            case 0: // STRONGLY USE GLOBAL
                if(global_prediction != taken && local_prediction == taken)
                {
                    choice_prediction_table[global_prediction_index] = 1; 
                }
                break;
            case 1: // WEAKLY USE GLOBAL
                if(global_prediction == taken && local_prediction != taken)
                {
                    choice_prediction_table[global_prediction_index] = 0;
                }
                else if(global_prediction != taken && local_prediction == taken)
                {
                    choice_prediction_table[global_prediction_index] = 2;
                }
                break;
            case 2: //WEAKLY USE LOCAL
                if(global_prediction == taken && local_prediction != taken)
                {
                    choice_prediction_table[global_prediction_index] = 1;
                }
                else if(global_prediction != taken && local_prediction == taken)
                {
                    choice_prediction_table[global_prediction_index] = 3;
                }
                break;

            case 3: //STRONGLY USE LOCAL
            if(global_prediction == taken && local_prediction != taken)
            {
                choice_prediction_table[global_prediction_index] = 2;
            }
            break;

            default: printf("INVALID ENTRY IN THE CHOICE TABLE");

        }
    }
    