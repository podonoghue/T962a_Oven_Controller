package testingChart;

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
import org.jfree.chart.axis.NumberAxis;
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
 * SMT Oven Application using JFreechart library.
 * 
 * @author Peter O'Donoghue
 *
 */
public class OvenApp extends JFrame {

   /**
    * Describes a solder profile
    */
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
      
      /**
       * 
       * @param description     Description of the profile     
       * @param ramp1Slope      Slope up to soakTemp1          
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
            float    ramp1Slope,       // Slope up to soakTemp1
            float    soakTemp1,        // Temperature for start of soak
            float    soakTemp2,        // Temperature for end of soak
            float    soakTime,         // Length of soak
            float    ramp2Slope,       // Slope up to peakTemp
            float    peakTemp,         // Peak reflow temperature
            float    peakDwell,        // How long to remain at peakTemp
            float    rampDownSlope) {  // Slow down after peak (cooling)

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

   /** Title for main chart */
   final String chartTitle = "Temperature Information";

   /** Y-Axis title */
   final String yAxisTemperatureLabel = "Temperature (Celsius)";
   
   /** Y-Axis title */
   final String yAxisPercentageLabel = "Percentage";
   
   /** X-Axis title */
   final String xAxisLabel = "Time (seconds)";

   /** Main Panel */
   private JPanel chartPanel;
   
   /** Series for selected profile */
   XYSeries profileSeries = new XYSeries("Profile");

   /** Series for target profile as executed on oven */
   XYSeries targetSeries  = new XYSeries("Target");

   /** Series for average oven temperature */
   XYSeries averageSeries = new XYSeries("Average");
   
   /** Series for fan speed (percent) */
   XYSeries fanSeries     = new XYSeries("Fan"); 
   
   /** Series for heater duty cycle (percent) */
   XYSeries heaterSeries  = new XYSeries("Heater");
   
   /**
    * Plots a profile as a line graph connecting profile control points 
    * 
    * @param profileSeries Profile points are added to this series
    * @param profile       Profile to plot
    */
   void plotProfile(XYSeries profileSeries, final SolderProfile profile) {

      // Clear existing data
      profileSeries.clear();
      
      // Assume starting from ambient of 25 celsius
      float ambient = 25.0f;
      
      // Step through profile points
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

//   SolderProfile profile = new SolderProfile(
//         /* description   */ "4300 63SN/37PB-a",
//         /* ramp1Slope    */ 1.0f,
//         /* soakTemp1     */ 140.0f,
//         /* soakTemp2     */ 183.0f,
//         /* soakTime      */ 90.0f,
//         /* ramp2Slope    */ 1.4f,
//         /* peakTemp      */ 210.0f,
//         /* peakDwell     */ 15.0f,
//         /* rampDownSlope */ -3.0f
//         );

   /**
    * 
    */
   private static final long serialVersionUID = 1L;

   /**
    * Create data set to hold the data for the plot
    * 
    * @return Created data set
    */
   private XYDataset createTemperatureDataset() {
      XYSeriesCollection dataset = new XYSeriesCollection();

      dataset.addSeries(profileSeries);
      dataset.addSeries(targetSeries);
      dataset.addSeries(averageSeries);

      return dataset;
   }

   /**
    * Create data set to hold the data for the plot
    * 
    * @return Created data set
    */
   private XYDataset createPercentageDataset() {
      XYSeriesCollection dataset = new XYSeriesCollection();

      dataset.addSeries(fanSeries);
      dataset.addSeries(heaterSeries);

      return dataset;
   }

   /**
    * Create chart panel. This includes creating<br>
    * <li>The panel</li>
    * <li>The XY chart</li>
    * <li>The various series for the chart</li>
    * 
    * @return Chart panel created
    */
   private JPanel createChartPanel() {

      // Create chart
      JFreeChart chart = ChartFactory.createXYLineChart(
            chartTitle, 
            xAxisLabel, 
            yAxisTemperatureLabel, 
            createTemperatureDataset());
      
      // Create renderer with line but no shapes (too many points)
      XYLineAndShapeRenderer renderer = new XYLineAndShapeRenderer(true,false);
      // Have points for profile only
      renderer.setSeriesShapesVisible(0, true);
      //      // sets paint color for each series
      //      renderer.setSeriesPaint(0, Color.RED);
      //      renderer.setSeriesPaint(1, Color.GREEN);
      //
      //      // sets thickness for series (using strokes)
      //      renderer.setSeriesStroke(0, new BasicStroke(4.0f));
      //      renderer.setSeriesStroke(1, new BasicStroke(3.0f));

      XYPlot plot = chart.getXYPlot();
      plot.setBackgroundPaint(Color.DARK_GRAY);
      
      final NumberAxis axis2 = new NumberAxis(yAxisPercentageLabel);
//      axis2.setAutoRangeIncludesZero(false);
      plot.setRangeAxis(1, axis2);
      plot.setDataset(1, createPercentageDataset());
      plot.mapDatasetToRangeAxis(1, 1);
      plot.setRangeAxis(1, axis2);
      
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

   /**
    * 
    * @param scm
    * @param handle
    * @throws SerialComException
    */
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

   /**
    * 
    * @param scm
    * @param handle
    * @throws SerialComException
    */
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
//      System.out.println("data read is :" + data);
      String[] points = data.split(";");
      targetSeries.clear();
      averageSeries.clear();
      heaterSeries.clear();
      fanSeries.clear();
      for (int index=1; index< points.length; index++) {
         String[] values = points[index].split(",");
         if (values.length == 10) {
//            String state         = values[0];
            int    time          = Integer.parseInt(values[1]);
            float  targetTemp    = Float.parseFloat(values[2]);
            float  averageTemp   = Float.parseFloat(values[3]);
            int    heaterPercent = Integer.parseInt(values[4]);
            int    fanPercent    = Integer.parseInt(values[5]);
//            float  thermocouple1 = Float.parseFloat(values[6]);
//            float  thermocouple2 = Float.parseFloat(values[7]);
//            float  thermocouple3 = Float.parseFloat(values[8]);
//            float  thermocouple4 = Float.parseFloat(values[9]);
            targetSeries.add((float)time, targetTemp);
            averageSeries.add((float)time, averageTemp);
            heaterSeries.add((float)time, (float)heaterPercent);
            fanSeries.add((float)time, (float)fanPercent);
//            System.out.println(
//                  String.format("%3d %6.1f %6.1f %3d %3d ", 
//                        time, targetTemp, averageTemp, heaterPercent, fanPercent));
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
//         System.out.println("data read is :" + data);
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

   /**
    * 
    */
   public OvenApp() {
      super("SMT Oven");

      chartPanel = createChartPanel();

      final JButton button = new JButton("Update");
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
