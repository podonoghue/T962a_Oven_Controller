package testingChart;

import java.awt.Color;

import javax.swing.JOptionPane;

import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.annotations.XYTextAnnotation;
import org.jfree.chart.axis.AxisLocation;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.plot.CombinedDomainXYPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.ValueMarker;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.RectangleAnchor;
import org.jfree.ui.TextAnchor;

import com.serialpundit.core.SerialComException;

import testingChart.OvenCommunication.OvenCommunicationException;

/**
 * SMT Oven Application using JFreechart library.
 * 
 * @author Peter O'Donoghue
 */
public class ProfileChartPanel extends ChartPanel {

   /** SerialNumber */
   private static final long serialVersionUID = 6840754021687208492L;

   //   final String COM_PORT = "com23";
   final static String COM_PORT = "com29";

   /** Title for main chart */
   final String chartTitle = "Temperature Information";

   /** Y-Axis title */
   final String yAxisTemperatureLabel = "Temperature (Celsius)";

   /** X-Axis title */
   final String xAxisLabel = "Time (seconds)";

   /** Series for selected profile */
   XYSeries profileSeries = new XYSeries("Profile");

   /** Marker for liquidus temperature */
   ValueMarker liquidusMarker = new ValueMarker(0.0);

   /** Name of profile on chart */
   XYTextAnnotation profileName = new XYTextAnnotation("No Profile", 10, 100);

   /**
    * Create data set to hold the data for the plot
    * 
    * @return Created data set
    */
   private XYDataset createTemperatureDataset() {
      XYSeriesCollection dataset = new XYSeriesCollection();

      dataset.addSeries(profileSeries);
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
   ProfileChartPanel() {
      super(null);

      final XYDataset temperatureDataset = createTemperatureDataset();
      // Create renderer with line but no shapes (too many points)
      XYLineAndShapeRenderer temperatureRenderer = new XYLineAndShapeRenderer(true,false);
      // Have points for profile only
      temperatureRenderer.setSeriesShapesVisible(0, true);
      final NumberAxis temperatureAxis = new NumberAxis(yAxisTemperatureLabel);
      final XYPlot temperaturePlot = new XYPlot(temperatureDataset, null, temperatureAxis, temperatureRenderer);
      temperatureAxis.setAutoRange(true);
      temperaturePlot.setRangeAxisLocation(AxisLocation.BOTTOM_OR_LEFT);

      final CombinedDomainXYPlot plot = new CombinedDomainXYPlot(new NumberAxis(xAxisLabel));
      plot.setGap(10.0);

      plot.add(temperaturePlot, 4);
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

      this.setChart(chart);
   }

   /**
    * Reads the profile from the oven and plots it to the graph 
    * 
    * @param scm     Serial communication manager
    * @param handle  Handle for com port
    * @throws OvenCommunicationException 
    * 
    * @throws SerialComException
    */
   void updateProfileChart(OvenCommunication oven) throws OvenCommunicationException {

      // Read current profile from oven
      SolderProfile profile = oven.getProfile();

      // Update plot
      profile.plotProfile(profileSeries);
      liquidusMarker.setValue(profile.liquidus);
      profileName.setText("Profile: \n" + profile.description);
      profileName.setY(profileSeries.getMaxY());
   }

   /**
    * Updates the graph from the Oven.
    */
   public void update() {
      OvenCommunication oven = new OvenCommunication();

      try {
         updateProfileChart(oven);
      } catch (OvenCommunicationException e) {
         JOptionPane.showMessageDialog(this, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
      } finally {
         try {
            oven.close();
         } catch (OvenCommunicationException e) {
            // Ignore
         }
      }
   }

}
