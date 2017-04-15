package net.sourceforge.usbdm.oven;

import java.util.ArrayList;

import com.fazecast.jSerialComm.SerialPort;

/**
 * Class for communication with oven
 */
public class OvenCommunicationjSerialComm extends OvenCommunication {

   /**
    * Time to wait for serial response
    */
   private static final int SERIAL_READ_TIME_OUT = 1;
   
   /**
    * Open serial port
    */
   private static SerialPort comPort = null;

   /**
    * Select default port name
    * 
    * @throws OvenCommunicationException 
    */
   protected void selectDefaultPortName() {
      String[] portNames = getPortNames();
      comPortName = null;
      for (String s:portNames) {
         if (s.startsWith(OVEN_COMM_NAME)) {
            comPortName = s;
            break;
         }
      }
      if ((comPortName == null) && (portNames.length>0)) {
         comPortName = portNames[0];
      }
   }

   /**
    * Get list of port names
    * 
    * @return Array of currently available serial ports
	*
    * @throws OvenCommunicationException
    */
   protected String[] getPortNames() {
      ArrayList<String> portNames = new ArrayList<>();
      SerialPort[] comPorts = SerialPort.getCommPorts();
      for (SerialPort port:comPorts) {
         System.err.println(port.getDescriptivePortName());
         portNames.add(port.getDescriptivePortName());
      }
      return portNames.toArray(new String[portNames.size()]);
   }

   /**
    * Opens the communication to the Oven<br>
    * The Oven should be closed after use.
    * 
    * @throws OvenCommunicationException 
    */
   void open() throws OvenCommunicationException {
      if (comPort != null) {
         throw new OvenCommunicationException("Oven already open"); 
      }
      try {
         if (comPortName == null) {
            // No name selected - look for oven
            selectDefaultPortName();
         }
         // Look for port to open based on description
         comPort = null;
         SerialPort[] comPorts = SerialPort.getCommPorts();
         for (SerialPort port:comPorts) {
            if (port.getDescriptivePortName().equals(comPortName)) {
               comPort = port;
               break;
            }
         }
         if (comPort == null) {
            throw new OvenCommunicationException("Failed to find oven serial port");
         }
         // Open and configure serial port
         comPort.openPort();
         comPort.setBaudRate(115200);
         comPort.setParity(SerialPort.NO_PARITY);
         comPort.setNumStopBits(SerialPort.ONE_STOP_BIT);
         comPort.setNumDataBits(8);
         comPort.setNumStopBits(SerialPort.ONE_STOP_BIT);
         comPort.setFlowControl(SerialPort.FLOW_CONTROL_DISABLED);
         comPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 100, 0);

         // Check for correct oven response
         writeString(OVEN_ID_COMMAND);
         String response = readString();
         System.out.println("data read is :" + response);
         if (!response.startsWith("SMT-Oven")) {
            close();
            throw new OvenCommunicationException("Oven failed to respond"); 
         }
      } catch (Exception e) {
         close();
         throw new OvenCommunicationException("Failed to open Oven serial port\nCheck serial port name", e); 
      }
   }

   /**
    * Closes communication with the oven
    * 
    * @throws OvenCommunicationException 
    */
   void close() throws OvenCommunicationException {
      if (comPort != null) {
         comPort.closePort();
         comPort = null;
      }
   }

   /**
    * Write string to the Oven
    * 
    * @param string
    * 
    * @throws OvenCommunicationException
    */
   protected void writeString(String string) throws OvenCommunicationException {
      if (comPort == null) {
         throw new OvenCommunicationException("Oven communication not open"); 
      }
      byte oBuffer[] = string.getBytes();
      comPort.writeBytes(oBuffer, oBuffer.length);
   }

   /**
    * Read string from the Oven
    * 
    * @return String read
    * 
    * @throws OvenCommunicationException
    */
   protected String readString() throws OvenCommunicationException {
      StringBuilder sb = new StringBuilder();
      comPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, SERIAL_READ_TIME_OUT, 0);
      try {
         while (true) {
            byte[] readBuffer = new byte[1024];
            int numRead = comPort.readBytes(readBuffer, readBuffer.length);
            if (numRead == 0) {
               break;
            }
            for (int index=0; index<numRead; index++) {
               sb.append((char)readBuffer[index]);
            }
         }
      } catch (Exception e) { 
         e.printStackTrace(); 
         throw new OvenCommunicationException("Failed read from Oven", e); 
      }
      System.out.println("Read " + sb.toString());
      return sb.toString();
   }

}
