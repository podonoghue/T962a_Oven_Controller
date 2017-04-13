package testingChart;

import java.io.IOException;

import com.serialpundit.core.SerialComException;
import com.serialpundit.serial.SerialComManager;
import com.serialpundit.serial.SerialComManager.BAUDRATE;
import com.serialpundit.serial.SerialComManager.DATABITS;
import com.serialpundit.serial.SerialComManager.FLOWCONTROL;
import com.serialpundit.serial.SerialComManager.PARITY;
import com.serialpundit.serial.SerialComManager.STOPBITS;

import testingChart.PlotData.PlotDataPoint;

public class OvenCommunication {

      final String COM_PORT = "com23";
//   final static String COM_PORT = "com29";

   SerialComManager scm = null;
   long handle = -1;

   @SuppressWarnings("serial")
   static class OvenCommunicationException extends IOException {

      public OvenCommunicationException(String message) {
         super(message);
      }

      public OvenCommunicationException(String message, Throwable e) {
         super(message, e);
      }
   }

   public OvenCommunication() {
   }

   /**
    * Opens the communication to the Oven<br>
    * The Oven should be closed after use.
    * 
    * @throws OvenCommunicationException 
    */
   public void open() throws OvenCommunicationException {
      if ((scm != null) && (handle != -1)) {
         // Already open - ignore
         return;
      }
      try {
         // Open com port
         scm = new SerialComManager();
         handle = scm.openComPort(COM_PORT, true, true, true);
         scm.configureComPortData(handle, DATABITS.DB_8, STOPBITS.SB_1, PARITY.P_NONE, BAUDRATE.B115200, 0);
         scm.configureComPortControl(handle, FLOWCONTROL.NONE, 'x', 'x', false, false);

         // Check for correct oven response
         scm.writeString(handle, "idn?\n\r", 0);
         String data = scm.readString(handle);
         //         System.out.println("data read is :" + data);
         if (!data.startsWith("SMT-Oven")) {
            close();
            throw new OvenCommunicationException("Oven failed to respond"); 
         }
      } catch (SerialComException e) {
         close();
         throw new OvenCommunicationException("Failed to open Oven serial port\nCheck serial port name", e); 
      } catch (IOException e) {
         close();
         throw new OvenCommunicationException("Creation of Serial Communication Manager failed", e); 
      }
   }

   /**
    * Closes communication with the oven
    * 
    * @throws OvenCommunicationException 
    */
   public void close() throws OvenCommunicationException {
      try {
         if ((scm != null) && (handle != -1)) {
            scm.closeComPort(handle);
         }
      } catch (Exception e) {
         throw new OvenCommunicationException("Failed to close Oven communication", e); 
      } finally {
         scm    = null;
         handle = -1;
      }
   }

   /**
    * Write string to the Oven
    * 
    * @param string
    * 
    * @throws OvenCommunicationException
    */
   void writeString(String string) throws OvenCommunicationException {
      if ((scm == null) && (handle == -1)) {
         throw new OvenCommunicationException("Oven communication not open"); 
      }
      try {
         scm.writeString(handle, string, 0);
      } catch (SerialComException e) {
         throw new OvenCommunicationException("Failed write to Oven", e); 
      }
   }

   /**
    * Write string to the Oven
    * 
    * @param string
    * 
    * @throws OvenCommunicationException
    */
   String readString() throws OvenCommunicationException {
      if ((scm == null) && (handle == -1)) {
         throw new OvenCommunicationException("Oven communication not open"); 
      }
      try {
         return scm.readString(handle);
      } catch (SerialComException e) {
         throw new OvenCommunicationException("Failed read from Oven", e); 
      }
   }

   /**
    * Send command and get response from Oven
    * 
    * @param command Command to send
    * 
    * @return Response from oven
    * 
    * @throws OvenCommunicationException
    */
   String commandResponse(String command) throws OvenCommunicationException {
      String data;
      try {
         open();
         // Write command
         writeString(command);
         // Get response
         data = readString();
         String moreData;
         do {
            moreData = readString();
            if (moreData == null) {
               break;
            }
            data = data + moreData;
         } while (true);
         if (data != null) {
            // Remove line breaks
            data = data.replaceAll("[\n\r]", "");
         }
      } catch (Exception e) {
         throw e;
      } finally {
         close();
      }
      return data;
   }

   /**
    * Read current profile from oven
    * 
    * @return Profile
    * 
    * @throws OvenCommunicationException
    */
   SolderProfile getProfile() throws OvenCommunicationException {
      String data = commandResponse("prof?\n\r");
      if (data == null) {
         throw new OvenCommunicationException("Failed to read profile");
      }
//      System.out.println("data read is :" + data);
      String[] values = data.split(",");
      if (values.length != 12) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data);
      }
      SolderProfile profile = new SolderProfile();
      int i = 1;
      profile.description   = values[i++];
      profile.flags         = Integer.valueOf(values[i++], 16);
      profile.liquidus      = Integer.parseInt(values[i++]);
      profile.preheatTime   = Integer.parseInt(values[i++]);
      profile.soakTemp1     = Integer.parseInt(values[i++]);
      profile.soakTemp2     = Integer.parseInt(values[i++]);
      profile.soakTime      = Integer.parseInt(values[i++]);
      profile.rampUpSlope    = Float.parseFloat(values[i++]);
      profile.peakTemp      = Integer.parseInt(values[i++]);
      profile.peakDwell     = Integer.parseInt(values[i++]);
      // Strip extra terminator from last item
      values[i] = values[i].replaceAll("[;\n\r]", "");
      profile.rampDownSlope = Float.parseFloat(values[i++]);

      return profile;
   }

   /**
    * Update profile in Oven
    * 
    * @param index   Index of profile to modify
    * @param profile New profile values
    * 
    * @throws OvenCommunicationException
    * 
    * @note This change will be persistent in the oven
    */
   void setProfile(int index, SolderProfile profile) throws OvenCommunicationException {
      String command = String.format("PROF %d,%s,%2X,%d,%d,%d,%d,%d,%f,%d,%d,%f\n\r", 
            index,
            profile.description,
            profile.flags,
            profile.liquidus,
            profile.preheatTime,
            profile.soakTemp1,
            profile.soakTemp2,
            profile.soakTime,
            profile.rampUpSlope,
            profile.peakTemp,
            profile.peakDwell,
            profile.rampDownSlope);
      String data = commandResponse(command);
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Select the a profile on the oven
    * 
    * @param number Number of profile to select
    * @throws OvenCommunicationException 
    */
   public void selectProfile(Number index) throws OvenCommunicationException {
      String command = String.format("PROF %d\n\r", index);
      String data = commandResponse(command);
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Reads plot data from Oven
    * 
    * @return Plot data
    * 
    * @throws OvenCommunicationException
    */
   PlotData getPlotValues() throws OvenCommunicationException {
      String data = commandResponse("plot?\n\r");
      if (data == null) {
         throw new OvenCommunicationException("Failed to read plot");
      }
      // Split in to points
      String[] points = data.split(";");

      // Parse into PlotData
      PlotData plotData = new PlotData(points.length);
//      System.out.println(PlotData.PlotDataPoint.title());
      for (int index=1; index<points.length; index++) {
         String[] values = points[index].split(",");
         if (values.length == 10) {
            //            String state         = values[0];
            plotData.points[index] = new PlotDataPoint(); 
            plotData.points[index].state         = values[0];
            plotData.points[index].time          = Integer.parseInt(values[1]);
            plotData.points[index].targetTemp    = Float.parseFloat(values[2]);
            plotData.points[index].averageTemp   = Float.parseFloat(values[3]);
            plotData.points[index].heaterPercent = Integer.parseInt(values[4]);
            plotData.points[index].fanPercent    = Integer.parseInt(values[5]);
            plotData.points[index].thermocouple1 = Float.parseFloat(values[6]);
            plotData.points[index].thermocouple2 = Float.parseFloat(values[7]);
            plotData.points[index].thermocouple3 = Float.parseFloat(values[8]);
            plotData.points[index].thermocouple4 = Float.parseFloat(values[9]);
         }
         else {
            System.err.println("Opps");
         }
//         System.out.println(plotData.points[index].toString());
      }
      return plotData;
   }

   /**
    * Start reflow run on oven
    * 
    * @throws OvenCommunicationException
    */
   void startReflow() throws OvenCommunicationException {
      String data = commandResponse("run\n\r");
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Abort reflow run on target
    * 
    * @throws OvenCommunicationException
    */
   void abortReflow() throws OvenCommunicationException {
      String data = commandResponse("abort\n\r");
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Class representing thermocouple setting
    */
   static class Thermocouple {
      boolean enabled = false;
      float   offset  = 0;

      Thermocouple() {
      }

      Thermocouple(boolean enabled, float offset) {
         this.enabled = enabled;
         this.offset  = offset;
      }
   };

   /**
    * Get thermocouple settings
    * 
    * @return Array of 4 thermocouples
    */
   Thermocouple[] getThermocouples() {
      Thermocouple result[] = new Thermocouple[4];
      return result;

   }

   /**
    * Set thermocouple values
    * 
    * @param values Array of 4 thermocouples settings
    * 
    * @throws OvenCommunicationException 
    * 
    * @note This change will be persistent in the oven
    */
   void setThermocouples(Thermocouple values[]) throws OvenCommunicationException {
      StringBuilder command = new StringBuilder();
      command.append("THERM ");
      boolean needSeparator = false;
      for (int index=0; index<4; index++) {
         if (needSeparator) {
            command.append(",");
         }
         needSeparator = true;
         command.append(String.format("%s,%f", values[index].enabled?"1":"0", values[index].offset));
      }
      command.append(";\n\r");
      System.err.println("Sending "+command.toString());
      String data = commandResponse(command.toString());
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Class representing parameters for PID controller
    */
   static class PidParameters {
      float kp = 0.0f;
      float ki = 0.0f;
      float kd = 0.0f;
      
      PidParameters() {
      }
      PidParameters(float kp, float ki, float kd) {
         this.kp = kp;
         this.ki = ki;
         this.kd = kd;
      }
      
      public String toString() {
         return String.format("[p=%f, i=%f, d=%f]", kp, ki, kd);
      }
   }
   
   /**
    * Set PID parameters
    *  
    * @param value PID parameters
    * 
    * @throws OvenCommunicationException
    * 
    * @note This change will be persistent in the oven
    */
   void setPidParameters(PidParameters value) throws OvenCommunicationException {
      String command = String.format("PID %f, %f, %f;\n\r", value.kp, value.ki, value.kd);
      System.err.println("Sending "+command.toString());
      String data = commandResponse(command.toString());
      // Check for correct oven response
      if ((data == null) || !data.startsWith("OK")) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data); 
      }
   }

   /**
    * Get PID parameters
    *  
    * @return PID parameters
    *  
    * @throws OvenCommunicationException
    */
   PidParameters getPidParameters() throws OvenCommunicationException {
      String data = commandResponse("pid?\n\r");
      if (data == null) {
         throw new OvenCommunicationException("Failed to read PID parameters");
      }
      String[] values = data.split(",");
      if (values.length != 3) {
         throw new OvenCommunicationException("Invalid oven response to query\n\r"+data);
      }
      PidParameters parameters = new PidParameters();
      int i = 0;
      parameters.kp         = Float.valueOf(values[i++]);
      parameters.ki         = Float.valueOf(values[i++]);
      // Strip extra terminator from last item
      values[i] = values[i].replaceAll("[;\n\r]", "");
      parameters.kd         = Float.valueOf(values[i++]);

      return parameters;
   }

}
