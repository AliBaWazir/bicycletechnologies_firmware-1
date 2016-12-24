//#include <iostream>
//using namespace std;

//code can be executed in https://www.codechef.com/ide

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*****************************************************
 * DEFINES
*****************************************************/
#define GEAR_COUNT_PEDAL 1
#define GEAR_COUNT_WHEEL 8
#define WHEEL_RPM_SIM_ARRAY_LENGTH 29
#define MAX_WHEEL_RPM 200

/*****************************************************
 * STATIC VARIABLES
*****************************************************/

double cadence_pm_set_point_rpm   = 80.0;
static double gear_ratios_array [GEAR_COUNT_WHEEL] = {2.91, 2.46, 2.13, 1.78, 1.45, 1.23, 1.07, 0.94};
static double wheel_rpm_sim_array [] = {60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 190, 180, 170, 160, 150, 140, 130, 120, 110, 100, 90, 80, 70, 60};
//static double wheel_rpm_sim_array [] = {167,81,15,14,14,106,148,72,139,86,96,3,14,52,101,88,163,196,19,33,49,32,120,55,61,130,93,188,93};
static int current_wheel_gear_index  = 3; //initially cassete gear is at the middle
double current_wheel_rpm             = 0.0;
double cadence_after_shifting        = 0.0;

//Arrays to store result vectors
static double cadence_after_shifting_array [WHEEL_RPM_SIM_ARRAY_LENGTH];
static int gear_index_after_shifting_array [WHEEL_RPM_SIM_ARRAY_LENGTH];
/*****************************************************
 * STATIC FUNCTIONS
*****************************************************/
static void set_phisical_desired_gears (int gear_pedal_index, int gear_wheel_index){
    int phisical_desired_gear_pedal = gear_pedal_index +1;
    int phisical_desired_gear_wheel = gear_wheel_index +1;
    
	printf("for wheel RPM= %3.1f ==> ", current_wheel_rpm);
    printf("physical desired gears: crankset_gear= %d, cassette_gear= %d\n", 
    phisical_desired_gear_pedal ,phisical_desired_gear_wheel);
}

static double evaluate_current_cycling_cadence(){
	
	return (current_wheel_rpm/gear_ratios_array[current_wheel_gear_index]);
}

static void print_sim_result_vectors(){
	int i = 0;
	
	printf("wheel RPM,      gear index after shifting,      cadence after shifting\n");
	for (i= 0; i<WHEEL_RPM_SIM_ARRAY_LENGTH; i++){
		printf("%.2f,          %d,           %.2f\n",
		wheel_rpm_sim_array[i], gear_index_after_shifting_array[i], cadence_after_shifting_array[i]);
	}
}

static void wheel_rpm_sim_array_init(){
	
	int i = 0;
	time_t t;
	   
	/* Intializes random number generator */
	srand((unsigned) time(&t));

	/* Print 5 random numbers from 0 to 49 */
	for(i = 0 ; i < WHEEL_RPM_SIM_ARRAY_LENGTH; i++) {
	    //generate random value between 60-200
		wheel_rpm_sim_array[i]= (double)((rand()%(MAX_WHEEL_RPM-60))+60);
	}
	 
}

int main()
{
	int i                                = 0;
	int j                                = 0;
	static int desired_gear_pedal_index  = 2; //never change
	static int desired_gear_wheel_index  = 0;
	double cadence_at_gears [GEAR_COUNT_PEDAL];

	
	//memset(cadence_at_gears, 0, sizeof((*cadence_at_gears)*GEAR_COUNT_WHEEL));
	
	//initialize the wheel_rpm_sim_array
	wheel_rpm_sim_array_init();
	
	for (i = 0; i< WHEEL_RPM_SIM_ARRAY_LENGTH; i++){
		double current_cycling_cadence;
		double current_cycling_cadence_error;
		double expected_cycling_cadence;
		double expected_cycling_cadence_error;
		double expected_cycling_cadence_error_previous;
		bool looping_backword_in_ratios_array;
		
		//reset all i iteration vriables
		current_cycling_cadence                  = 0.0;
		current_cycling_cadence_error            = 0.0;
		expected_cycling_cadence                 = 0.0;
		expected_cycling_cadence_error           = 0.0;
		expected_cycling_cadence_error_previous  = 0.0;
		looping_backword_in_ratios_array         = false;
			
		current_wheel_rpm = wheel_rpm_sim_array[i];
		//printf("current wheel rpm= %f \n", current_wheel_rpm);
		
		current_cycling_cadence = current_wheel_rpm/gear_ratios_array[current_wheel_gear_index];
		//printf("current_cycling_cadence= %.2f \n", current_cycling_cadence);
		
		//calculate current cycling cadence relative error
		current_cycling_cadence_error = (double) ((current_cycling_cadence - cadence_pm_set_point_rpm));
		//printf("current_cycling_cadence_error= %.2f \n", current_cycling_cadence_error);
		
		if(current_cycling_cadence_error == 0){
		    //keep current gear index
			desired_gear_wheel_index = current_wheel_gear_index;
		} else{
		    
		    if (current_cycling_cadence_error >= 0){
				//loop backword throgh all wheel gears to select optimum gear
				looping_backword_in_ratios_array= true;
				//printf("loop backword ...\n"); 
    		} else{
    			//loop forward throgh all wheel gears to select optimum gear
    			looping_backword_in_ratios_array= false;
    			//printf("loop forward ...\n");
    		}
    		
    		for (j= current_wheel_gear_index; (looping_backword_in_ratios_array&&(j>= 0)) || ((!looping_backword_in_ratios_array)&&(j< GEAR_COUNT_WHEEL)); (looping_backword_in_ratios_array?j--:j++)){
				
				//calculate expected cadence at gear index j
				expected_cycling_cadence = current_wheel_rpm/gear_ratios_array[j];
				//printf("expected_cycling_cadence for gear [%d]= %.2f\n", j, expected_cycling_cadence); 
				//store previous expected cycling cadence error
				expected_cycling_cadence_error_previous= expected_cycling_cadence_error;
				//calculate expected cycling cadence relative error
				expected_cycling_cadence_error = (double) (expected_cycling_cadence - cadence_pm_set_point_rpm);
				//check for sign change in expected relative error
				if ((expected_cycling_cadence_error>0 && current_cycling_cadence_error<0) || 
					(expected_cycling_cadence_error<0 && current_cycling_cadence_error>0))
				{
					break;
				}
			
			}
    		
    		//printf("expected_cycling_cadence_error= %f. current_cycling_cadence_error= %f\n", expected_cycling_cadence_error, current_cycling_cadence_error);
    		if (abs(expected_cycling_cadence_error) < abs(current_cycling_cadence_error)){
    			//check boundary conditions
    			if ( j>= GEAR_COUNT_WHEEL){
    			    desired_gear_wheel_index = GEAR_COUNT_WHEEL-1;
    			} else if(j< 0){
    			    desired_gear_wheel_index = 0;
    			} else{
    			    //loop break due to sign change in cadence error
    			    if (abs(expected_cycling_cadence_error_previous) < abs(expected_cycling_cadence_error)){
    			        if(looping_backword_in_ratios_array){
    			            desired_gear_wheel_index = j+1;
    			        } else{
    			            desired_gear_wheel_index = j-1;
    			        }
    			    } else{
    			        desired_gear_wheel_index = j;
    			    }
    			    
    			}
    		} else {
    			//keep current gear index
    			desired_gear_wheel_index = current_wheel_gear_index;
    		}
		}

		//set current wheel gear with the desired gear
		current_wheel_gear_index = desired_gear_wheel_index;
		//printf("current_wheel_gear_index is set to gear with index[%d] \n", current_wheel_gear_index);
		//set_phisical_desired_gears(desired_gear_pedal_index, desired_gear_wheel_index);
		
		cadence_after_shifting = evaluate_current_cycling_cadence();
		//printf("current_cadence after shifting= %2.1f \n", cadence_after_shifting);
		
		//store results in corresponding vector
		gear_index_after_shifting_array[i]= current_wheel_gear_index;
		cadence_after_shifting_array[i]= cadence_after_shifting;
		
	}
	
	//print result vectors
	print_sim_result_vectors();
    
    
    return 0;
}


