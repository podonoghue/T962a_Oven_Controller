package net.sourceforge.usbdm.oven;

import java.awt.Component;
import java.io.IOException;

import javax.swing.JOptionPane;

import net.sourceforge.usbdm.oven.PlotData.PlotDataPoint;

/**
 * Base class for communication with oven
 */
public abstract class OvenCommunication {

   /**
    * Name of serial port
    */
   static String comPortName = null;

   /**
    * Oven ID command
    */
   static final String OVEN_ID_COMMAND = "idn?\n\r";

   /**
    * Expected response from oven for ID command
    */
   static final String OVEN_ID_RESPONSE = "SMT-Oven";

   /**
    * Oven serial port description will start with this
    */
   static final String OVEN_COMM_NAME = "T962a";

   /**
    * Exceptions from communication with oven
    */
   @SuppressWarnings("serial")
   static class OvenCommunicationException extends IOException {

      public OvenCommunicationException(String message) {
         super(message);
      }

      public OvenCommunicationException(String message, Throwable e) {
         super(message, e);
      }
   }

   /**
    * Select default port name
    * 
    * @throws OvenCommunicationException 
    * @throws IOException 
    */
   protected void selectDefaultPortName() throws OvenCommunicationException {
      String[] portNames = getPortNames();
      comPortName = null;
      for (String s:portNames) {
         if (s.startsWith(OVEN_COMM_NAME)) {
            comPortName = s;
            break;
         }
      }
   }

   /**
    * Provides dialogue to select communication port for oven
    */
   void selectPortDialogue(Component parent) {
      try {
         if (comPortName == null) {
            // Check for default name
            selectDefaultPortName();
         }
         String[] portNames = getPortNames();
         if (portNames.length != 0) {
            comPortName = (String) JOptionPane.showInputDialog(
                  parent, 
                  "Choose port...",
                  "Communication port", 
                  JOptionPane.QUESTION_MESSAGE, 
                  null, 
                  portNames, 
                  comPortName); 
            System.out.println(comPortName);
         }
      } catch (OvenCommunicationException e) {
         e.printStackTrace();
      }
   }

   /**
    * Get list of port names
    * 
    * @return Array of currently available serial ports
    * 
    * @throws IOException
    */
   protected abstract String[] getPortNames() throws OvenCommunicationException;

   /**
    * Opens the communication to the Oven<br>
    * The Oven should be closed after use.
    * 
    * @throws OvenCommunicationException 
    */
   abstract void open() throws OvenCommunicationException;

   /**
    * Closes communication with the oven
    * 
    * @throws OvenCommunicationException 
    */
   abstract void close() throws OvenCommunicationException;

   /**
    * Write string to the Oven
    * 
    * @param string
    * 
    * @throws OvenCommunicationException
    */
   protected abstract void writeString(String string) throws OvenCommunicationException;

   /**
    * Read string from the Oven
    * 
    * @return String read
    * 
    * @throws OvenCommunicationException
    */
   protected abstract String readString() throws OvenCommunicationException;

   /**
    * Send command and get response from Oven
    * 
    * @param command Command to send
    * 
    * @return Response from oven
    * 
    * @throws OvenCommunicationException
    */
   protected String commandResponse(String command) throws OvenCommunicationException {
      String data;
      try {
         open();
         // Write command
         writeString(command);
         // Get response
         data = readString();
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
   void selectProfile(Number index) throws OvenCommunicationException {
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
   protected PlotData getPlotValues() throws OvenCommunicationException {
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
      //TODO
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
   class PidParameters {
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
