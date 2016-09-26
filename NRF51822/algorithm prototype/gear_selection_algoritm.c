int main()
{
    
    
    int ratios[3] = {1,2,3};
    int current_rpm = 5; 
    //scanf("%d", &current_rpm);
    
    int current_cadence[3]; 
    
    for(int i = 0; i < 3; i++){
        current_cadence[i] = ratios[i]*current_rpm;
        
        printf("%d\n",current_cadence[i]);
    }
    
    
    
    return 0;
}