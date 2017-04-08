package testingChart;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.annotations.XYTextAnnotation;
import org.jfree.chart.axis.AxisLocation;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.plot.CombinedDomainXYPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.ValueMarker;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.StandardXYItemRenderer;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.RectangleAnchor;
import org.jfree.ui.TextAnchor;

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
 */
public class OvenApp extends JFrame {

//   final String COM_PORT = "com23";
   final static String COM_PORT = "com29";

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

   /** Series for heater duty cycle (percent) */
   XYSeries thermocoupleSeries[]  = {new XYSeries("TC1"), new XYSeries("TC2"), new XYSeries("TC3"), new XYSeries("TC4")};

   /** Marker for liquidus temperature */
   ValueMarker liquidusMarker = new ValueMarker(0.0);

   /** Name of profile on chart */
   XYTextAnnotation profileName = new XYTextAnnotation("No Profile", 10, 100);

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

      for (XYSeries series:thermocoupleSeries) {
         dataset.addSeries(series);
      }
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
    * Create chart panel.<br> 
    * This includes creating:
    * <li>The panel</li>
    * <li>The XY chart</li>
    * <li>The various series for the chart</li>
    * 
    * @return Chart panel created
    */
   private JPanel createChartPanel() {
      final XYDataset temperatureDataset = createTemperatureDataset();
      // Create renderer with line but no shapes (too many points)
      XYLineAndShapeRenderer temperatureRenderer = new XYLineAndShapeRenderer(true,false);
      // Have points for profile only
      temperatureRenderer.setSeriesShapesVisible(0, true);
      final NumberAxis temperatureAxis = new NumberAxis(yAxisTemperatureLabel);
      final XYPlot temperaturePlot = new XYPlot(temperatureDataset, null, temperatureAxis, temperatureRenderer);
      temperatureAxis.setAutoRange(true);
      temperaturePlot.setRangeAxisLocation(AxisLocation.BOTTOM_OR_LEFT);

      final XYDataset percentageDataset = createPercentageDataset();
      final XYItemRenderer percentageRenderer = new StandardXYItemRenderer();
      final NumberAxis percentageAxis = new NumberAxis(yAxisPercentageLabel);
      percentageAxis.setAutoRange(false);
      percentageAxis.setRange(0.0, 105.0);
      final XYPlot percentagePlot = new XYPlot(percentageDataset, null, percentageAxis, percentageRenderer);
      percentagePlot.setRangeAxisLocation(AxisLocation.TOP_OR_LEFT);
     
      final CombinedDomainXYPlot plot = new CombinedDomainXYPlot(new NumberAxis(xAxisLabel));
      plot.setGap(10.0);

      plot.add(temperaturePlot, 4);
      plot.add(percentagePlot,  1);
      plot.setOrientation(PlotOrientation.VERTICAL);

      // return a new chart containing the overlaid plot...
      JFreeChart chart = new JFreeChart(chartTitle, JFreeChart.DEFAULT_TITLE_FONT, plot, true);

      temperaturePlot.addRangeMarker(liquidusMarker);
      liquidusMarker.setPaint(Color.black);
      liquidusMarker.setLabel("Liquidus"); 
      liquidusMarker.setLabelBackgroundColor(Color.white);
      liquidusMarker.setLabelAnchor(RectangleAnchor.BOTTOM_RIGHT);
      liquidusMarker.setLabelTextAnchor(TextAnchor.TOP_RIGHT);

      temperaturePlot.addAnnotation(profileName);
      profileName.setTextAnchor(TextAnchor.TOP_LEFT);
      
      return new ChartPanel(chart);
   }

   /**
    * Reads the profile from the oven and plots it to the graph 
    * 
    * @param scm     Serial communication manager
    * @param handle  Handle for com port
    * 
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
      if (data == null) {
         System.out.println("No data");
         return;
      }
      System.out.println("data read is :" + data);
      String[] values = data.split(",");
      if (values.length == 12) {
         values[10] = values[10].split(";")[0];
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
         values[i] = values[i].replaceAll("[;\n\r]", "");
         profile.rampDownSlope = Float.parseFloat(values[i++]);
         
         profile.plotProfile(profileSeries);
         liquidusMarker.setValue(profile.liquidus);
         profileName.setText("Profile: \n" + profile.description);
         profileName.setY(profileSeries.getMaxY());
      }
   }

   /**
    * Reads the plot from the oven and plots it to the graph.<br>
    * Plots Target, Average, Fan and Heater.
    * 
    * @param scm     Serial communication manager
    * @param handle  Handle for com port
    * 
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
      for (XYSeries series:thermocoupleSeries) {
         series.clear();
      }
      for (int index=1; index<points.length; index++) {
         String[] values = points[index].split(",");
         if (values.length == 10) {
//            String state         = values[0];
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
            thermocoupleSeries[0].add((float)time, (float)thermocouple1);
            thermocoupleSeries[1].add((float)time, (float)thermocouple2);
            thermocoupleSeries[2].add((float)time, (float)thermocouple3);
            thermocoupleSeries[3].add((float)time, (float)thermocouple4);
//            System.out.println(
//                  String.format("%3d %6.1f %6.1f %3d %3d ", 
//                        time, targetTemp, averageTemp, heaterPercent, fanPercent));
         }
      }
   }
   
   /**
    * Updates the graph from the Oven.
    */
   public void update() {
      SerialComManager scm = null;
      long handle = -1;
      try {
         scm = new SerialComManager();
         handle = scm.openComPort(COM_PORT, true, true, true);
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
    * The main application
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

   /**
    * Main application
    * 
    * @param args
    */
   public static void main(String[] args) {
      SwingUtilities.invokeLater(new Runnable() {
         @Override
         public void run() {
            new OvenApp().setVisible(true);
         }
      });
   }

}
