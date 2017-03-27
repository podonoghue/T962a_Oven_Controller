package testingChart;

import java.awt.BasicStroke;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;

import com.serialpundit.core.SerialComException;
import com.serialpundit.serial.SerialComManager;
import com.serialpundit.serial.SerialComManager.BAUDRATE;
import com.serialpundit.serial.SerialComManager.DATABITS;
import com.serialpundit.serial.SerialComManager.FLOWCONTROL;
import com.serialpundit.serial.SerialComManager.PARITY;
import com.serialpundit.serial.SerialComManager.STOPBITS;

/**
 * This program demonstrates how to draw XY line chart with XYDataset
 * using JFreechart library.
 * @author www.codejava.net
 *
 */
public class OvenApp extends JFrame {

   static class SolderProfile {
      String   description;      // Description of the profile
      int      flags;
      float    ramp1Slope;       // Slope up to soakTemp1
      float    soakTemp1;        // Temperature for start of soak
      float    soakTemp2;        // Temperature for end of soak
      float    soakTime;         // Length of soak
      float    ramp2Slope;       // Slope up to peakTemp
      float    peakTemp;         // Peak reflow temperature
      float    peakDwell;        // How long to remain at peakTemp
      float    rampDownSlope;    // Slope down after peakTemp

      public SolderProfile() {

      }
      public SolderProfile(
            String   description,      // Description of the profile
            float    ramp1Slope,       // Slope up to soakTemp1
            float    soakTemp1,        // Temperature for start of soak
            float    soakTemp2,        // Temperature for end of soak
            float    soakTime,         // Length of soak
            float    ramp2Slope,       // Slope up to peakTemp
            float    peakTemp,         // Peak reflow temperature
            float    peakDwell,        // How long to remain at peakTemp
            float    rampDownSlope) {

         this.description   = description;
         this.ramp1Slope    = ramp1Slope;
         this.soakTemp1     = soakTemp1;
         this.soakTemp2     = soakTemp2;
         this.soakTime      = soakTime;
         this.ramp2Slope    = ramp2Slope;
         this.peakTemp      = peakTemp;
         this.peakDwell     = peakDwell;
         this.rampDownSlope = rampDownSlope;
      }
   };

   final String chartTitle = "Temperature Information";
   final String yAxisLabel = "Temperature (Celsius)";
   final String xAxisLabel = "Time (seconds)";

   private JPanel chartPanel;
   XYSeries profileSeries = new XYSeries("Profile");;
   XYSeries targetSeries  = new XYSeries("Target");;
   XYSeries averageSeries = new XYSeries("Average");;
   XYSeries fanSeries     = new XYSeries("Fan");;
   XYSeries heaterSeries  = new XYSeries("Heater");;

   enum State {
      s_off,
      s_fail,
      s_init,
      s_preheat,
      s_soak,
      s_ramp_up,
      s_dwell,
      s_ramp_down,
      s_complete,
      s_manual,
   };

   /**
    * Plots a profile
    * 
    * @param profileSeries
    * @param profile
    */
   void plotProfile(XYSeries profileSeries, final SolderProfile profile) {

      float ambient = 25.0f;
      float time = 0.0f;
      profileSeries.add(time, ambient);
      time += (profile.soakTemp1-ambient)/profile.ramp1Slope;
      profileSeries.add(time, profile.soakTemp1);
      time += profile.soakTime;
      profileSeries.add(time, profile.soakTemp2);
      time += (profile.peakTemp-profile.soakTemp2)/profile.ramp2Slope;
      profileSeries.add(time, profile.peakTemp);
      time += profile.peakDwell;
      profileSeries.add(time, profile.peakTemp);
      time += (25.0f-profile.peakTemp)/profile.rampDownSlope;
      profileSeries.add(time, ambient);
   }

   SolderProfile profile = new SolderProfile(
         /* description   */ "4300 63SN/37PB-a",
         /* ramp1Slope    */ 1.0f,
         /* soakTemp1     */ 140.0f,
         /* soakTemp2     */ 183.0f,
         /* soakTime      */ 90.0f,
         /* ramp2Slope    */ 1.4f,
         /* peakTemp      */ 210.0f,
         /* peakDwell     */ 15.0f,
         /* rampDownSlope */ -3.0f
         );

   /**
    * 
    */
   private static final long serialVersionUID = 1L;

   private XYDataset createDataset() {
      XYSeriesCollection dataset = new XYSeriesCollection();

      dataset.addSeries(profileSeries);
      dataset.addSeries(targetSeries);
      dataset.addSeries(averageSeries);
      dataset.addSeries(fanSeries);
      dataset.addSeries(heaterSeries);

      return dataset;
   }

   private JPanel createChartPanel() {

      XYDataset dataset = createDataset();

      JFreeChart chart = ChartFactory.createXYLineChart(chartTitle,
            xAxisLabel, yAxisLabel, dataset);

      XYLineAndShapeRenderer renderer = new XYLineAndShapeRenderer();

      //      // sets paint color for each series
      //      renderer.setSeriesPaint(0, Color.RED);
      //      renderer.setSeriesPaint(1, Color.GREEN);
      //
      //      // sets thickness for series (using strokes)
      //      renderer.setSeriesStroke(0, new BasicStroke(4.0f));
      //      renderer.setSeriesStroke(1, new BasicStroke(3.0f));

      XYPlot plot = chart.getXYPlot();

      plot.setBackgroundPaint(Color.DARK_GRAY);

      plot.setRangeGridlinesVisible(true);
      plot.setRangeGridlinePaint(Color.WHITE);

      plot.setDomainGridlinesVisible(true);
      plot.setDomainGridlinePaint(Color.WHITE);
      plot.setRenderer(renderer);

      ValueAxis axis = plot.getDomainAxis();
      axis.setAutoRange(true);
      //		axis.setRange(0.0, 600.0); // 600 seconds
      axis = plot.getRangeAxis();
      axis.setAutoRange(true);
      //      axis.setRange(0.0, 400.0); 	// 0-400 C
      return new ChartPanel(chart);
   }

   int count = 100;
   void updateProfileChart(SerialComManager scm, long handle) throws SerialComException {
      scm.writeString(handle, "prof?\n\r", 0);
      String data = scm.readString(handle);
      String moreData;
      do {
         moreData = scm.readString(handle);
         if (moreData == null) {
            break;
         }
         data = data + moreData;
      } while (true);
      System.out.println("data read is :" + data);
      String[] values = data.split(",");
      if (values.length == 11) {
         values[10] = values[10].split(";")[0];
         SolderProfile profile = new SolderProfile();
         profile.description   = values[1];
         profile.flags         = Integer.valueOf(values[2], 16);
         profile.ramp1Slope    = Float.parseFloat(values[3]);
         profile.soakTemp1     = Integer.parseInt(values[4]);
         profile.soakTemp2     = Integer.parseInt(values[5]);
         profile.soakTime      = Integer.parseInt(values[6]);
         profile.ramp2Slope    = Float.parseFloat(values[7]);
         profile.peakTemp      = Integer.parseInt(values[8]);
         profile.peakDwell     = Integer.parseInt(values[9]);
         profile.rampDownSlope = Float.parseFloat(values[10]);
         plotProfile(profileSeries, profile);
      }
   }

   void updateOvenChart(SerialComManager scm, long handle) throws SerialComException {
      scm.writeString(handle, "plot?\n\r", 0);
      String data = scm.readString(handle);
      String moreData;
      do {
         moreData = scm.readString(handle);
         if (moreData == null) {
            break;
         }
         data = data + moreData;
      } while (true);
      System.out.println("data read is :" + data);
      String[] points = data.split(";");
      averageSeries.clear();
      heaterSeries.clear();
      fanSeries.clear();
      for (int index=1; index< points.length; index++) {
         String[] values = points[index].split(",");
         if (values.length == 10) {
            String state         = values[0];
            int    time          = Integer.parseInt(values[1]);
            float  targetTemp    = Float.parseFloat(values[2]);
            float  averageTemp   = Float.parseFloat(values[3]);
            int    heaterPercent = Integer.parseInt(values[4]);
            int    fanPercent    = Integer.parseInt(values[5]);
            float  thermocouple1 = Float.parseFloat(values[6]);
            float  thermocouple2 = Float.parseFloat(values[7]);
            float  thermocouple3 = Float.parseFloat(values[8]);
            float  thermocouple4 = Float.parseFloat(values[9]);
            targetSeries.add((float)time, targetTemp);
            averageSeries.add((float)time, averageTemp);
            heaterSeries.add((float)time, (float)heaterPercent);
            fanSeries.add((float)time, (float)fanPercent);
            System.out.println(
                  String.format("%3d %6.1f %6.1f %3d %3d ", 
                        time, targetTemp, averageTemp, heaterPercent, fanPercent));
         }
      }
   }
   /**
    * Handles a click on the button by adding new (random) data.
    *
    * @param e  the action event.
    */
   public void update() {
      SerialComManager scm = null;
      long handle = -1;
      try {
         scm = new SerialComManager();
         handle = scm.openComPort("com29", true, true, true);
         scm.configureComPortData(handle, DATABITS.DB_8, STOPBITS.SB_1, PARITY.P_NONE, BAUDRATE.B115200, 0);
         scm.configureComPortControl(handle, FLOWCONTROL.NONE, 'x', 'x', false, false);
         scm.writeString(handle, "idn?\n\r", 0);
         String data = scm.readString(handle);
         System.out.println("data read is :" + data);
         if (data.startsWith("SMT-Oven")) {
            updateOvenChart(scm, handle);
            updateProfileChart(scm, handle);
         }
         scm.closeComPort(handle);
      } catch (Exception e) {
         if ((scm != null) && (handle != -1)) {
            try {
               scm.closeComPort(handle);
            } catch (SerialComException e1) {
            }
         }
         e.printStackTrace();
      }
   }

   public OvenApp() {
      super("SMT Oven");

      chartPanel = createChartPanel();

      final JButton button = new JButton("Add New Data Item");
      button.addActionListener(new ActionListener() {
         @Override
         public void actionPerformed(ActionEvent e) {
            update();
         }
      });

      final JPanel content = new JPanel(new BorderLayout());
      content.add(chartPanel);
      content.add(button, BorderLayout.SOUTH);
      setContentPane(content);

      setSize(640, 480);
      setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      setLocationRelativeTo(null);
   }

   public static void main(String[] args) {
      SwingUtilities.invokeLater(new Runnable() {
         @Override
         public void run() {
            new OvenApp().setVisible(true);
         }
      });
   }

}
