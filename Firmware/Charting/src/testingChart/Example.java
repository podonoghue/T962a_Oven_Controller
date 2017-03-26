/**
 * 
 */
package testingChart;
import com.serialpundit.serial.SerialComManager;
import com.serialpundit.serial.SerialComManager.BAUDRATE;
import com.serialpundit.serial.SerialComManager.DATABITS;
import com.serialpundit.serial.SerialComManager.FLOWCONTROL;
import com.serialpundit.serial.SerialComManager.PARITY;
import com.serialpundit.serial.SerialComManager.STOPBITS;

public class Example {
   public static void main(String[] args) {
      try {
         SerialComManager scm = new SerialComManager();
         long handle = scm.openComPort("com29", true, true, true);
         scm.configureComPortData(handle, DATABITS.DB_8, STOPBITS.SB_1, PARITY.P_NONE, BAUDRATE.B115200, 0);
         scm.configureComPortControl(handle, FLOWCONTROL.NONE, 'x', 'x', false, false);
         scm.writeString(handle, "idn?\n\r", 0);
         String data = scm.readString(handle);
         System.out.println("data read is :" + data);
         scm.writeString(handle, "plot?\n\r", 0);
         data = scm.readString(handle);
         System.out.println("data read is :" + data);
         scm.closeComPort(handle);
      } catch (Exception e) {
         e.printStackTrace();
      }
   }
}