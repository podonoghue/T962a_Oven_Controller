package testingChart;

import org.jfree.data.xy.XYSeries;

/**
 * Describes a solder profile
 */
class SolderProfile {
   final static int  UNLOCKED      = 1<<0;
   final static int  LEAD_FREE_SOLDER = 1<<1;
   
   String   description;      // Description of the profile
   int      flags;            // Flags
   int      liquidus;         // Liquidus temperature
   int      preheatTime;      // Time to reach soakTemp1
   int      soakTemp1;        // Temperature for start of soak
   int      soakTemp2;        // Temperature for end of soak
   int      soakTime;         // Length of soak
   float    rampUpSlope;      // Slope up to peakTemp
   int      peakTemp;         // Peak reflow temperature
   int      peakDwell;        // How long to remain at peakTemp
   float    rampDownSlope;    // Slope down after peakTemp

   /**
    * Creates a solder profile
    */
   public SolderProfile() {
   }
   
   /**
    * Creates a solder profile
    * 
    * @param description     Description of the profile 
    * @param flags           Flags
    * @param liquidus        Liquidus temperature
    * @param preheatTime     Preheat time
    * @param soakTemp1       Temperature for start of soak 
    * @param soakTemp2       Temperature for end of soak   
    * @param soakTime        Length of soak                
    * @param ramp2Slope      Slope up to peakTemp          
    * @param peakTemp        Peak reflow temperature       
    * @param peakDwell       How long to remain at peakTemp
    * @param rampDownSlope   Slow down after peak (cooling)
    */
   public SolderProfile(
         String   description,      // Description of the profile
         int      flags,            // Flags
         int      liquidus,         // Liquidus temperature
         int      preheatTime,      // Time to reach soakTemp1 from ambient
         int      soakTemp1,        // Temperature for start of soak
         int      soakTemp2,        // Temperature for end of soak
         int      soakTime,         // Length of soak
         float    ramp2Slope,       // Slope up to peakTemp
         int      peakTemp,         // Peak reflow temperature
         int      peakDwell,        // How long to remain at peakTemp
         float    rampDownSlope) {  // Slow down after peak (cooling)

      this.description   = description;
      this.flags         = flags;
      this.liquidus      = liquidus;
      this.preheatTime   = preheatTime;
      this.soakTemp1     = soakTemp1;
      this.soakTemp2     = soakTemp2;
      this.soakTime      = soakTime;
      this.rampUpSlope   = ramp2Slope;
      this.peakTemp      = peakTemp;
      this.peakDwell     = peakDwell;
      this.rampDownSlope = rampDownSlope;
   }
   
   public String toString() {
     return String.format("PROF [%s,%2X,%d,%d,%d,%d,%d,%f,%d,%d,%f]", 
            description,
            flags,
            liquidus,
            preheatTime,
            soakTemp1,
            soakTemp2,
            soakTime,
            rampUpSlope,
            peakTemp,
            peakDwell,
            rampDownSlope);
      }
   /**
    * Plots a profile as a line graph connecting profile control points 
    * 
    * @param profileSeries   Profile points are added to this series
    * @param liquidusMarker  Modified to reflect profile liquidus temperature
    * @param profile         Profile to plot
    */
   void plotProfile(XYSeries profileSeries) {

      // Clear existing data
      profileSeries.clear();
      
      // Assume starting from ambient of 25 celsius
      float ambient = 25.0f;
      
      // Step through profile points
      float time = 0.0f;
      profileSeries.add(time, ambient);
      time += preheatTime;
      profileSeries.add(time, soakTemp1);
      time += soakTime;
      profileSeries.add(time, soakTemp2);
      time += (peakTemp-soakTemp2)/rampUpSlope;
      profileSeries.add(time, peakTemp);
      time += peakDwell;
      profileSeries.add(time, peakTemp);
      time += (25.0f-peakTemp)/rampDownSlope;
      profileSeries.add(time, ambient);
   }
}