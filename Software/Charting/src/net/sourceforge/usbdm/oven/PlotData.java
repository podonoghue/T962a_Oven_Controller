package net.sourceforge.usbdm.oven;

/**
 * Represents a set of plot points
 * 
 * @author podonoghue
 */
public class PlotData {
   public static class PlotDataPoint {
      String state;
      int    time; 
      float  targetTemp; 
      float  averageTemp; 
      int    heaterPercent; 
      int    fanPercent; 
      float  thermocouple1; 
      float  thermocouple2; 
      float  thermocouple3; 
      float  thermocouple4; 

      /**
       * Title for printing
       * 
       * @return Title as string
       */
      public static String title() {
         return "   State   Time target average heat fan    TC1    TC2    TC3    TC4";
      }
      
      /**
       * Plot point as string
       * 
       * @return String representing the plot point
       */
      public String toString() {
         return 
               String.format("%10s %3d %6.1f %6.1f   %3d  %3d %6.1f %6.1f %6.1f %6.1f ", 
                     state, time, targetTemp, averageTemp, heaterPercent, fanPercent, 
                     thermocouple1, thermocouple2, thermocouple3, thermocouple4 );

      }
   };

   /**
    * The points making up the plot
    */
   PlotDataPoint[] points;

   /**
    * Constructor
    * 
    * @param size Number of plot points
    */
   PlotData(int size) {
      points = new PlotDataPoint[size];
   }
}
