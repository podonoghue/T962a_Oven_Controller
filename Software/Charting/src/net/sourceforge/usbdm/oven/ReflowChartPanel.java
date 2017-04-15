package net.sourceforge.usbdm.oven;

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
import org.jfree.chart.renderer.xy.StandardXYItemRenderer;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.RectangleAnchor;
import org.jfree.ui.TextAnchor;

import com.serialpundit.core.SerialComException;

import net.sourceforge.usbdm.oven.OvenCommunication.OvenCommunicationException;

/**
 * Panel displaying reflow chart
 */
public class ReflowChartPanel extends ChartPanel {

   /** SerialNumber */
   private static final long serialVersionUID = 6840754021687208492L;

   /** Title for main chart */
   private static final String CHART_TITLE = "Temperature Information";

   /** Y-Axis title */
   private static final String Y_AXIS_TEMPERATURE_LABEL = "Temperature (Celsius)";

   /** Y-Axis title */
   private static final String Y_AXIS_PERCENTAGE_LABEL = "Percentage";

   /** X-Axis title */
   private static final String X_AXIS_LABEL = "Time (seconds)";

   /** Series for selected profile */
   private final XYSeries profileSeries = new XYSeries("Reference Profile");

   /** Series for target profile as executed on oven */
   private final XYSeries targetSeries  = new XYSeries("Target Profile");

   /** Series for average oven temperature */
   private final XYSeries averageSeries = new XYSeries("Average");

   /** Series for fan speed (percent) */
   private final XYSeries fanSeries     = new XYSeries("Fan"); 

   /** Series for heater duty cycle (percent) */
   private final XYSeries heaterSeries  = new XYSeries("Heater");

   /** Series for thermocouple 1 */
   private final XYSeries thermocoupleSeries1 = new XYSeries("TC1");

   /** Series for thermocouple 2 */
   private final XYSeries thermocoupleSeries2 = new XYSeries("TC2");

   /** Series for thermocouple 3 */
   private final XYSeries thermocoupleSeries3 = new XYSeries("TC3");

   /** Series for thermocouple 4 */
   private final XYSeries thermocoupleSeries4 = new XYSeries("TC4");

   /** Marker for liquidus temperature */
   private final ValueMarker liquidusMarker = new ValueMarker(0.0);

   /** Name of profile on chart */
   private final XYTextAnnotation profileName = new XYTextAnnotation("No Profile", 10, 100);

   /** The actual chart */
   private final JFreeChart chart;

   /**
    * Create data set to hold the temperature data for the plot
    * 
    * <li>Profile</li>
    * <li>Target temperature</li>
    * <li>Average temperature</li>
    * <li>Thermocouple 1 temperature</li>
    * <li>Thermocouple 2 temperature</li>
    * <li>Thermocouple 3 temperature</li>
    * <li>Thermocouple 4 temperature</li>
    * 
    * @return Created data set
    */
   private XYDataset createTemperatureDataset() {
      XYSeriesCollection dataset = new XYSeriesCollection();

      dataset.addSeries(profileSeries);
      dataset.addSeries(targetSeries);
      dataset.addSeries(averageSeries);
      dataset.addSeries(thermocoupleSeries1);
      dataset.addSeries(thermocoupleSeries2);
      dataset.addSeries(thermocoupleSeries3);
      dataset.addSeries(thermocoupleSeries4);
      return dataset;
   }

   /**
    * Create data set to hold the percentage data for the plot
    * <li>Fan percentage</li>
    * <li>Heater percentage</li>
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
   ReflowChartPanel() {
      super(null);

      final XYDataset temperatureDataset = createTemperatureDataset();
      // Create renderer with line but no shapes (too many points)
      XYLineAndShapeRenderer temperatureRenderer = new XYLineAndShapeRenderer(true,false);
      // Have points for profile only
      temperatureRenderer.setSeriesShapesVisible(0, true);
      final NumberAxis temperatureAxis = new NumberAxis(Y_AXIS_TEMPERATURE_LABEL);
      final XYPlot temperaturePlot = new XYPlot(temperatureDataset, null, temperatureAxis, temperatureRenderer);
      temperatureAxis.setAutoRange(true);
      temperaturePlot.setRangeAxisLocation(AxisLocation.BOTTOM_OR_LEFT);

      final XYDataset percentageDataset = createPercentageDataset();
      final XYItemRenderer percentageRenderer = new StandardXYItemRenderer();
      final NumberAxis percentageAxis = new NumberAxis(Y_AXIS_PERCENTAGE_LABEL);
      percentageAxis.setAutoRange(false);
      percentageAxis.setRange(0.0, 105.0);
      final XYPlot percentagePlot = new XYPlot(percentageDataset, null, percentageAxis, percentageRenderer);
      percentagePlot.setRangeAxisLocation(AxisLocation.TOP_OR_LEFT);

      final CombinedDomainXYPlot plot = new CombinedDomainXYPlot(new NumberAxis(X_AXIS_LABEL));
      plot.setGap(10.0);

      plot.add(temperaturePlot, 4);
      plot.add(percentagePlot,  1);
      plot.setOrientation(PlotOrientation.VERTICAL);

      chart = new JFreeChart(CHART_TITLE, JFreeChart.DEFAULT_TITLE_FONT, plot, true);

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
    * @param oven Oven communication
    * 
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
    * Reads the plot from the oven and plots it to the chart.<br>
    * Plots Target, Average, Thermocouples, Fan and Heater.
    * 
    * @param oven Oven communication
    * 
    * @throws OvenCommunicationException 
    * 
    * @throws SerialComException
    */
   void updateOvenChart(OvenCommunication oven) throws OvenCommunicationException {
      PlotData plot = oven.getPlotValues();
      targetSeries.clear();
      averageSeries.clear();
      heaterSeries.clear();
      fanSeries.clear();
      thermocoupleSeries1.clear();
      thermocoupleSeries2.clear();
      thermocoupleSeries3.clear();
      thermocoupleSeries4.clear();
      for (int index=1; index<plot.points.length; index++) {
         targetSeries.add(       plot.points[index].time, plot.points[index].targetTemp);
         averageSeries.add(      plot.points[index].time, plot.points[index].averageTemp);
         heaterSeries.add(       plot.points[index].time, plot.points[index].heaterPercent);
         fanSeries.add(          plot.points[index].time, plot.points[index].fanPercent);
         thermocoupleSeries1.add(plot.points[index].time, plot.points[index].thermocouple1);
         thermocoupleSeries2.add(plot.points[index].time, plot.points[index].thermocouple2);
         thermocoupleSeries3.add(plot.points[index].time, plot.points[index].thermocouple3);
         thermocoupleSeries4.add(plot.points[index].time, plot.points[index].thermocouple4);
      }
   }

   /**
    * Updates the graph from the Oven.
    */
   public void update(OvenCommunication oven) {
      try {
         chart.setNotify(false);
         updateOvenChart(oven);
         updateProfileChart(oven);
         chart.setNotify(true);
      } catch (OvenCommunicationException e) {
         JOptionPane.showMessageDialog(this, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
      } finally {
      }
   }

}
