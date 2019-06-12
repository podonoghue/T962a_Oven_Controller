package net.sourceforge.usbdm.oven;

import java.io.IOException;

import com.serialpundit.core.SerialComException;
import com.serialpundit.serial.SerialComManager;
import com.serialpundit.serial.SerialComManager.BAUDRATE;
import com.serialpundit.serial.SerialComManager.DATABITS;
import com.serialpundit.serial.SerialComManager.FLOWCONTROL;
import com.serialpundit.serial.SerialComManager.PARITY;
import com.serialpundit.serial.SerialComManager.STOPBITS;

/**
 * Class for communication with oven
 */
public class OvenCommunicationSerialPundit extends OvenCommunication {

   /**
    * Serial communication manager
    */
   SerialComManager scm = null;

   /**
    * Handle for open serial port
    */
   private long handle = -1;

   /**
    * Constructor
    * 
    * @throws IOException
    */
   OvenCommunicationSerialPundit() throws IOException {
      scm = new SerialComManager();
   }

   /**
    * Select default port name
    * 
    * @throws OvenCommunicationException 
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
   protected String[] getPortNames() throws OvenCommunicationException {
      try {
         return scm.listAvailableComPorts();
      } catch (SerialComException e) {
         e.printStackTrace();
         throw new OvenCommunicationException(e.getMessage());
      }
   }

   /**
    * Opens the communication to the Oven<br>
    * The Oven should be closed after use.
    * 
    * @throws OvenCommunicationException 
    */
   void open() throws OvenCommunicationException {
      if (handle != -1) {
         throw new OvenCommunicationException("Oven already open"); 
      }
      try {
         if (comPortName == null) {
            // No name selected - look for oven
            selectDefaultPortName();
         }
         // Open com port
         handle = scm.openComPort(comPortName, true, true, true);
//         System.err.println("Opening port = "+scm.getPortName(handle));

         scm.configureComPortData(handle, DATABITS.DB_8, STOPBITS.SB_1, PARITY.P_NONE, BAUDRATE.B115200, 0);
         scm.configureComPortControl(handle, FLOWCONTROL.NONE, 'x', 'x', false, false);

         // Check for correct oven response
         scm.writeString(handle, "idn?\n\r", 0);
         String data = scm.readString(handle);
         //         System.out.println("data read is :" + data);
         if ((data==null) || !data.startsWith("SMT-Oven")) {
            close();
            throw new OvenCommunicationException("Oven failed to respond"); 
         }
      } catch (SerialComException e) {
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
      try {
         if ((scm != null) && (handle != -1)) {
            scm.closeComPort(handle);
         }
      } catch (Exception e) {
         throw new OvenCommunicationException("Failed to close Oven communication", e); 
      } finally {
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
   protected void writeString(String string) throws OvenCommunicationException {
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
    * Read string from the Oven
    * 
    * @return String read
    * 
    * @throws OvenCommunicationException
    */
   protected String readString() throws OvenCommunicationException {
      if ((scm == null) && (handle == -1)) {
         throw new OvenCommunicationException("Oven communication not open"); 
      }
      try {
         String data = scm.readString(handle);
         do {
            String moreData = scm.readString(handle);
            if (moreData == null) {
               break;
            }
            data = data + moreData;
         } while (true);
         return data;
      } catch (SerialComException e) {
         throw new OvenCommunicationException("Failed read from Oven", e); 
      }
   }

}
