#include <stdio.h>
#include <string.h>

/*****************************************************
 * DEFINES
*****************************************************/
#define GEAR_COUNT_PEDAL 1
#define GEAR_COUNT_WHEEL 3


/*****************************************************
 * STATIC VARIABLES
*****************************************************/
static int desired_gear_pedal =0;
static int desired_gear_wheel =0;


/*****************************************************
 * STATIC FUNCTIONS
*****************************************************/
static void set_phisical_desired_gears (int gear_pedal_index, int gear_wheel_index){
    int phisical_desired_gear_pedal = gear_pedal_index +1;
    int phisical_desired_gear_wheel = gear_wheel_index +1;
    
    printf("The physical desired gear combination is: gear_pedal= %d, gear_wheel= %d\n", 
    phisical_desired_gear_pedal ,phisical_desired_gear_wheel);
}

int main()
{
	int current_rpm            = 0;
	int cadence_pm_set_point   = 0;
    int ratios[GEAR_COUNT_WHEEL] = {1,2,3};
	int cadence_at_gears [GEAR_COUNT_PEDAL];
	
	memset(cadence_at_gears, 0, sizeof((*cadence_at_gears)*GEAR_COUNT_WHEEL));
	
    //printf("Please enter the current rpm \n");
	//scanf("%d", &current_rpm);
	current_rpm            = 4;
	cadence_pm_set_point   = 8;
	
    
    for(int i = 0; i < 3; i++){
        cadence_at_gears[i] = ratios[i]*current_rpm;
		printf("if in gear[%d], cadence will be: %d\n", (i+1) ,cadence_at_gears[i]);
		
		//check if this gear will meet the cadence set point
		if (cadence_at_gears[i] == cadence_pm_set_point){
		    desired_gear_pedal = 0;
		    desired_gear_wheel = i;
		}
    }
    
    set_phisical_desired_gears(desired_gear_pedal, desired_gear_wheel);
    
    return 0;
}
