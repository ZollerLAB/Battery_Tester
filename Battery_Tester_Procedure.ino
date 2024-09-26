//--- DEFINITIONS

//Definition of global parameters
int shunt_resistance = 10; // resistance or the shunt resistor connected in series to the tested battery in ohm
int max_voltage=4200; // maximum allowed voltage of the tested battery in mV
int min_voltage =2750; // minimum allowed voltage of the tested battery in mV
float max_charge_current = 70; //maximum charge current of the battery
float max_discharge_current = -70; //maximum allowed discharge current of the battery
int maxtemp = 55; // maximum allowed temperature in °C
float capacity = 70*3.6; // Charge of fully charged battery in As
int recording_time_period = 1000; // time period between sended measurement data points in ms (cannot simply be changed)
int sense_pin_battery_high = 2; // number of the sense pin connected to the high potential of the tested battery
int sense_pin_battery_low = 1; // number of the sense pin connected to the low potential of the tested battery
int sense_pin_shunt_high = 1; // number of the sense pin connected to the high potential of the shunt resistor
int sense_pin_shunt_low = 0; // number of the sense pin connected to the low potential of the shunt resistor
int sense_pin_NTC_low = 3; // number of the sense pin connected to the low potential of the NTC
int cycle_counter = 0; // variable to count number of charge discharge cycles

// Definition of global variables
int actual_voltage; // actual value of controlled voltage in mV
int battery_voltage; // actual value of battery voltage in mV
float shunt_current; // actual current trough shunt resistor in mA
long timecounter = millis()/1000; // start time of the program in ms
float coulomb_counter; // Counts the charge which flows into (positive) or out of (negative) the battery in As
int SoC; // State of Charge in %
float starttime; // start time of a program section in ms
int endtime; // end time of a program section in ms
int integ; // summed deviation of actual_voltage from set voltage
float set_voltage; // desired set voltage
int width_charge; // width of charge pulse send to the gate of the mosfet
int width_discharge; // width of the discharge pulse send to the gate of the mosfet
int tempReading; // sensor value for NTC voltage probe
double temp; // temperature in °C
float shunt_current_av; // summed up current through shunt resistor in mA for the recording_time_period
float shunt_current_av_last; // average current through the shunt resistor in mA for the last recording time period
float battery_voltage_av; // summed up battery voltage in mV for the recording_time_period
float battery_voltage_av_last; // average battery voltage in mV for the last recording time period
float actual_voltage_av; // summed up controlled voltage in mV for the recording_time_period
float actual_voltage_av_last; // average controlled voltage in mV for the last recording time period
int n; // variable which counts tthe loops
bool control_voltage_criteria; // continue loop when criteria is true
bool time_criteria; // continue loop when criteria is true
bool current_criteria; // continue loop when criteria is true
bool battery_voltage_criteria; // continue loop when criteria is true

//--- FUNCTIONS

// This is a multi-purpose controler function called by following simpler functions
// The voltage controler tries to reach a set voltage ramp from the startvoltage (mV) to the endvoltage (mV) for a given timeperiod (ms).
// The voltage which should controled is defined by the voltage probes connected to the analog inputs with the numbers sense_pin_voltage_high and sense_pin_voltage_low
// Different modes for stop criteria can be selected:
// mode == 0: Stop when runtime is longer than timeperiod (ms)
// mode ==1: Stop when absolute value current trough shunt resistor is smaller than stopcurrent (mA)
// mode ==2: Stop when battery voltage is lower than endvoltage (mV) during discharge or higher during charge
void voltage_controler (int startvoltage, int endvoltage, float timeperiod, int sense_pin_voltage_high, int sense_pin_voltage_low, int stopcurrent, int stopvoltage, int mode){
  
  // Initialize variables
  n = 1;
  control_voltage_criteria = 1;
  time_criteria = 1;
  current_criteria = 1;
  battery_voltage_criteria = 1;
  starttime = millis();
  endtime = starttime + timeperiod;
  integ=0;
  actual_voltage_av=0;
  shunt_current_av=0;
  battery_voltage_av=0;
  
  // Measure actual voltages and current
  actual_voltage=(analogRead(sense_pin_voltage_high)-analogRead(sense_pin_voltage_low))*5/1.023;
  battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
  shunt_current=(analogRead(sense_pin_shunt_high)-analogRead(sense_pin_shunt_low))*5/(1.023*shunt_resistance);
  battery_voltage_av_last=battery_voltage;

  // Loop for the voltage ramp, which continues when the criterias are true
  while((time_criteria&&((mode==0)||current_criteria||battery_voltage_criteria))){
    
    // Calculate set voltage for current ramp
    set_voltage=startvoltage+((endvoltage-startvoltage)*(millis()-starttime))/timeperiod;
    // limit ramp to endvoltage
    if(millis()>(starttime+timeperiod)){
      set_voltage=endvoltage;
    }
    
    // sum up the errors for the integral part of the controler
    integ=integ+(set_voltage-actual_voltage);
    // prevent overflow of variable
    if(integ>26000){
        integ=26000;
    }
    if(integ<(-26000)){
        integ=-26000;
    }
    
    // calculate width of pulse width modulated output signal
    if(set_voltage>actual_voltage){
        width_charge=(2.55/50)*set_voltage+(2.55/50)*(set_voltage-actual_voltage)+(1.0/50)*integ;
        width_discharge=0;
        // limit pulse width to valid values
        if(width_charge>255){
           width_charge=255;
        }
        if(width_charge<0){
           width_charge=0;
        }
    }
    if(set_voltage<actual_voltage){
      width_charge=0;
      width_discharge=-(255/battery_voltage)*(set_voltage-actual_voltage)-(1.3/50)*integ;
      // limit pulse width to valid values
      if(width_discharge>255){
           width_discharge=255;
      }
      if(width_discharge<0){
         width_discharge=0;
      }
    }
    
    // Perform safety checks for voltage, current and temperature limits
    battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
    shunt_current=(analogRead(sense_pin_shunt_high)-analogRead(sense_pin_shunt_low))*5/(1.023*shunt_resistance);
    // Slowly discharge if voltage is higher than allowed
    if (battery_voltage>max_voltage){
      width_charge = 0;
      width_discharge = 127;
    }
    // Slowly charge if voltage is lower than allowed
    if (battery_voltage<min_voltage){
      width_charge = 127;
      width_discharge = 0;
    }
    // Limit discharge current
    if (shunt_current < max_discharge_current){
      width_charge = 0;
      width_discharge = width_discharge*max_discharge_current/shunt_current;
    }
    // Limit charge current
    if (shunt_current > max_charge_current){
      width_charge = width_charge*max_charge_current/shunt_current;
      width_discharge = 0;
    }
    // Pause and cool if temperature is higher than allowed
    tempReading = analogRead(sense_pin_NTC_low);
    temp = log(10000.0 * ((1024.0 / tempReading - 1)));
    temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp ))* temp )-273.15;
    if (temp>maxtemp){
      width_charge = 0;
      width_discharge = 0;
    }

    // Finally output the calculated pulses
    analogWrite(6, width_charge);
    analogWrite(5, width_discharge);

    // Measure actual values
    actual_voltage=(analogRead(sense_pin_voltage_high)-analogRead(sense_pin_voltage_low))*5/1.023;
    battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
    // Sum values for average calculation
    shunt_current_av+=(analogRead(sense_pin_shunt_high)-analogRead(sense_pin_shunt_low))*5/(1.023*shunt_resistance);
    battery_voltage_av+=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
    actual_voltage_av+=(analogRead(sense_pin_voltage_high)-analogRead(sense_pin_voltage_low))*5/1.023;
    n += 1;

    // Output data if recording_time_period is reached
    if ((millis()/recording_time_period)>timecounter){
      timecounter=millis()/1000;
      // Calculate average values
      actual_voltage_av_last=actual_voltage_av/n;
      shunt_current_av_last=shunt_current_av/n;
      battery_voltage_av_last=battery_voltage_av/n;
      //Coulomb counting
      coulomb_counter+=shunt_current_av_last*recording_time_period/(1000*1000);
      SoC=100*coulomb_counter/capacity;
      // Reset the summed up values
      actual_voltage_av=0;
      shunt_current_av=0;
      battery_voltage_av=0;
      n = 1;
      //Output the data for arduino ide serial monitor used for bugfixing
      /*
      Serial.print(timecounter);
      Serial.print(" Cycle");
      Serial.print(cycle_counter);
      Serial.print(" Soll ");
      Serial.print(set_voltage);
      Serial.print(", Ist ");
      Serial.print(actual_voltage_av_last);
      Serial.print(", Batterie ");
      Serial.print(battery_voltage_av_last);
      Serial.print(", Strom");
      Serial.print(shunt_current_av_last);
      Serial.print(", Laden");
      Serial.print(width_charge);
      Serial.print(", Entladen");
      Serial.print(width_discharge);
      Serial.print(", Power ");
      Serial.print(shunt_resistance/1000.0*pow(shunt_current_av_last,2));
      Serial.print(", Coulomb ");
      Serial.print(coulomb_counter);
      Serial.print(", Temp ");
      Serial.println(temp);
      */
      // Output data send to python for plotting and saving
      Serial.print(timecounter);
      Serial.print(", ");
      Serial.print(cycle_counter);
      Serial.print(", ");
      Serial.print(battery_voltage_av_last);
      Serial.print(", ");
      Serial.print(shunt_current_av_last);
      Serial.print(", ");
      Serial.print(coulomb_counter);
      Serial.print(", ");
      Serial.println(temp);
      
    }
  
  // Control the stop criteria for the loop
  control_voltage_criteria = (abs(actual_voltage_av_last-endvoltage)>=10);
  time_criteria = (millis()-starttime)<timeperiod;
  current_criteria = ((abs(shunt_current_av_last)>stopcurrent)||control_voltage_criteria)&&(mode==1);
  if (endvoltage<0){
    battery_voltage_criteria = ((battery_voltage_av_last-stopvoltage)>0)&&(mode==2);
  }
  if (endvoltage>0){
    battery_voltage_criteria = ((battery_voltage_av_last-stopvoltage)<0)&&(mode==2);
  }
  }
  //Output the data for arduino ide serial monitor used for bugfixing
  /*
  analogWrite(6, 0);
  analogWrite(5, 0);
  Serial.println("Abbruch:");
  Serial.print("Endvoltage = ");
  Serial.print(endvoltage);
  Serial.print(" mV; Actual = ");
  Serial.print(actual_voltage_av_last);
  Serial.print(" mV; Criteria = ");
  Serial.println(control_voltage_criteria);
  Serial.print("Timeperiod = ");
  Serial.print(timeperiod);
  Serial.print(" ms; Actual = ");
  Serial.print(millis()-starttime);
  Serial.print(" ms; Criteria = ");
  Serial.println(time_criteria);
  Serial.print("Stopcurrent = ");
  Serial.print(stopcurrent);
  Serial.print(" mA; Actual = ");
  Serial.print(shunt_current_av_last);
  Serial.print(" mA; Criteria = ");
  Serial.println(current_criteria);
  Serial.print("Stopvoltage = ");
  Serial.print(stopvoltage);
  Serial.print(" mV; Actual = ");
  Serial.print(battery_voltage_av_last);
  Serial.print(" mV; Criteria = ");
  Serial.println(battery_voltage_criteria);
  */
}


// This function can be used in the test plan section for constructing a voltage ramp starting with the startvoltage (mV)
// and leading linearly to the endvoltage (mV) of the battery in a given timeperiod (ms)
void voltage_ramp (int startvoltage, int endvoltage, float timeperiod){
  voltage_controler(startvoltage, endvoltage, timeperiod, sense_pin_battery_high, sense_pin_battery_low, 0, 0, 0);
}


// This function can be used in the test plan section for holding the battery on a voltage (mV) until a timeperiod (ms) is
// surpassed or the current decreased below a stopcurrent (mA)
void constant_voltage (int voltage, float timeperiod, int stopcurrent){
  voltage_controler(voltage, voltage, timeperiod, sense_pin_battery_high, sense_pin_battery_low, stopcurrent, 0, 1);
}


// This function can be used in the test plan section for constructing a current ramp starting with the startcurrent (mA)
// and leading linearly to the endcurrent (mA) of the battery in a given timeperiod (ms)
void current_ramp (int startcurrent, int endcurrent, float timeperiod){
  int startvoltage=shunt_resistance*startcurrent;
  int endvoltage=shunt_resistance*endcurrent;
  voltage_controler(startvoltage, endvoltage, timeperiod, sense_pin_shunt_high, sense_pin_shunt_low, 0, 0, 0);
}

// This function can be used in the test plan section for holding a current (mA) until a timeperiod (ms) is
// surpassed or the battery voltage decreased below a stopvoltage (mV)
void constant_current (int current, float timeperiod, int stopvoltage){
  int voltage=shunt_resistance*current;
  voltage_controler(voltage, voltage, timeperiod, sense_pin_shunt_high, sense_pin_shunt_low, 0, stopvoltage, 2);
}


// This function can be used in the test plan section for constructing a discharge power ramp starting with the startpower (mW)
// and leading linearly to the endpower (mW) dissipated by the shunt resistor in a given timeperiod (ms)
void discharge_power_ramp (float startpower, float endpower, float timeperiod){
  float startcurrent=-1000*sqrt(startpower/(1000*shunt_resistance));
  float endcurrent=-1000.0*sqrt(endpower/(1000.0*shunt_resistance));
  current_ramp(startcurrent,endcurrent,timeperiod);
}


// This function can be used in the test plan section to pause charge or discharge and only output data for a given timeperiod (ms)
void pause (int timeperiod){
  // Initialize values
  starttime = millis();
  actual_voltage_av=0;
  shunt_current_av=0;
  battery_voltage_av=0;
  n = 1;

  // Time loop
  while((millis()-starttime) < timeperiod){
      
      // Sum values for average calculation
      shunt_current_av+=(analogRead(sense_pin_shunt_high)-analogRead(sense_pin_shunt_low))*5/(1.023*shunt_resistance);
      battery_voltage_av+=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
      n += 1;
      
      // Output data if recording_time_period is reached
      if ((millis()/recording_time_period)>timecounter){
         tempReading = analogRead(sense_pin_NTC_low);
         temp = log(10000.0 * ((1024.0 / tempReading - 1)));
         temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp ))* temp )-273.15;
         timecounter=millis()/1000;
         // Calculate average values
         shunt_current_av_last=shunt_current_av/n;
         battery_voltage_av_last=battery_voltage_av/n;
         //Coulomb counting
         coulomb_counter+=shunt_current_av_last*recording_time_period/(1000*1000);
         SoC=100*coulomb_counter/capacity;
         // Reset the summed up values
         actual_voltage_av=0;
         shunt_current_av=0;
         battery_voltage_av=0;
         n = 1;
         //Output the data for arduino ide serial monitor used for bugfixing
         /*
         Serial.print(timecounter);
         Serial.print(", Batterie ");
         Serial.print(battery_voltage_av_last);
         Serial.print(", Strom");
         Serial.print(shunt_current_av_last);
         Serial.print(", Laden");
         Serial.print(width_charge);
         Serial.print(", Entladen");
         Serial.print(width_discharge);
         Serial.print(", Power ");
         Serial.print(shunt_resistance*pow(shunt_current_av_last,2));
         Serial.print(", Coulomb ");
         Serial.print(coulomb_counter);
         Serial.print(", Temp ");
         Serial.println(temp);
         */
         // Output data send to python for plotting and saving
         Serial.print(timecounter);
         Serial.print(", ");
         Serial.print(cycle_counter);
         Serial.print(", ");
         Serial.print(battery_voltage_av_last);
         Serial.print(", ");
         Serial.print(shunt_current_av_last);
         Serial.print(", ");
         Serial.print(coulomb_counter);
         Serial.print(", ");
         Serial.println(temp);
    }
  }
}


//--- SETUP SYSTEM
// This code runs once the system is started and defines baud-rate and I/O-pins
void setup() {
  Serial.begin(9600);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
}


//--- MAIN RUNNING LOOP

void loop() {
  
   // !!!!!! PUT YOUR TEST PROCEDURE HERE !!!!!
   
   // in this section the test program can be defined by calling following functions
   // voltage_ramp (int startvoltage, int endvoltage, float timeperiod)
   // constant_voltage (int voltage, float timeperiod, int stopcurrent)
   // current_ramp (int startcurrent, int endcurrent, float timeperiod)
   // constant_current (int current, float timeperiod, int stopvoltage)
   // discharge_power_ramp (int startpower, int endpower, float timeperiod)
   // pause (int timeperiod)

   // Set coulomb_counting = 0 if the battery is at the end of discharge voltage or coulomb_counting = capacity at the end of charge voltage
   // to get a reference point for coulomb counting and SoC calculation
   
   coulomb_counter = 0;
   constant_current(50,9000000,max_voltage-100);
   constant_voltage(max_voltage-100,9000000,5);
   constant_current(-50,9000000,min_voltage+100);
   constant_voltage(min_voltage+100,9000000,5);
   cycle_counter+=1;
   
   /*
   Serial.println("Discharge power ramp");
   discharge_power_ramp(0, 1 , 10000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   Serial.println("Charge Voltage Ramp");
   voltage_ramp(battery_voltage,battery_voltage+50,10000);
   Serial.println("Pause");
   pause(5000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   Serial.println("Discharge Voltage Ramp");
   voltage_ramp(battery_voltage,battery_voltage-50,10000);
   Serial.println("Pause");
   pause(5000);
   Serial.println("Charge Current Ramp");
   current_ramp(0,40,10000);
   Serial.println("Discharge Current Ramp");
   current_ramp(0,-10,10000);
   Serial.println("pause");
   pause(5000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   delay(1000);
   Serial.println("Charge CC");
   constant_current(20,600000,battery_voltage+60);
   Serial.println("pause");
   pause(5000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   Serial.println("Charge CV");
   constant_voltage(battery_voltage+15,600000,5);
   Serial.println("pause");
   pause(5000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   Serial.println("Discharge CC");
   constant_current(-20,600000,battery_voltage-60);
   Serial.println("pause");
   pause(5000);
   battery_voltage=(analogRead(sense_pin_battery_high)-analogRead(sense_pin_battery_low))*5/1.023;
   Serial.println("Disharge CV");
   constant_voltage(battery_voltage-5,600000,5);
   Serial.println("Discharge power ramp");
   discharge_power_ramp(0, 2 , 10000);
   */
   /*
   constant_current(50,9000000,max_voltage-100);
   constant_voltage(max_voltage-100,9000000,5);
   constant_current(-50,9000000,min_voltage+100);
   constant_voltage(min_voltage+100,9000000,5);
   cycle_counter+=1;
   */
   /*
   if(cycle_counter==0){
      constant_current(50,9000000,max_voltage-100);
      constant_voltage(max_voltage-100,9000000,5);
   }
   
   pause(8900);
   current_ramp (-0,-70,89000);
   current_ramp (-70,-64,507000);
   current_ramp (-64,-18,72000);
   current_ramp (-18,-65,67000);
   current_ramp (-65,-58,228000);
   current_ramp (-58,-0,39000);
   current_ramp (-0,-0,77000);
   current_ramp (-0,-67,15000);
   current_ramp (-67,-56,400000);
   current_ramp (-56,-0,33000);
   current_ramp (-0,-0,156000);
   current_ramp (-0,-52,18000);
   current_ramp (-52,-37,78000);
   current_ramp (-37,-0,13000);
   cycle_counter+=1;
   */
   /*
   constant_current(-12,9000000,min_voltage);
   constant_voltage(min_voltage,9000000,5);
   constant_current(12,14400000,max_voltage);
   cycle_counter+=1;
   */
   /*
   constant_current(40,600000,4500);
   constant_current(-40,600000,2500);
   cycle_counter+=1;
   */
}
