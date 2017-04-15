package net.sourceforge.usbdm.oven;

import java.awt.Color;
import java.awt.Cursor;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;

import javax.swing.JOptionPane;
import javax.swing.UIManager;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartMouseEvent;
import org.jfree.chart.ChartMouseListener;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.ChartRenderingInfo;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.annotations.XYTextAnnotation;
import org.jfree.chart.axis.AxisLocation;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.entity.ChartEntity;
import org.jfree.chart.entity.EntityCollection;
import org.jfree.chart.entity.XYItemEntity;
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

import net.sourceforge.usbdm.oven.OvenCommunication.OvenCommunicationException;

/**
 * 
 */
public class ProfileChartPanel extends ChartPanel implements
ChartMouseListener, MouseListener, MouseMotionListener {

   /** SerialNumber */
   private static final long serialVersionUID = 6840754021687208492L;

   /** Title for main chart */
   private final String chartTitle = "Temperature Information";

   /** Y-Axis title */
   private final String yAxisTemperatureLabel = "Temperature (Celsius)";

   /** X-Axis title */
   private final String xAxisLabel = "Time (seconds)";

   /** Series for selected profile */
   private XYSeries profileSeries = new XYSeries("Profile");

   /** Marker for liquidus temperature */
   private ValueMarker liquidusMarker = new ValueMarker(100.0);

   /** Name of profile on chart */
   private XYTextAnnotation profileName = new XYTextAnnotation("No Profile", 10, 100);

   private boolean canMove;

   double initialMovePointY = 0.0;

   private XYItemEntity xyItemEntity;

   private JFreeChart chart;

   private ChartRenderingInfo info = null;

   private double finalMovePointY = 0;

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

      final NumberAxis temperatureAxis = new NumberAxis(yAxisTemperatureLabel);
      temperatureAxis.setAutoRange(true);

      chart = ChartFactory.createXYLineChart(chartTitle, xAxisLabel, yAxisTemperatureLabel, temperatureDataset, PlotOrientation.VERTICAL, true, true, false );

      chart.setBackgroundPaint(UIManager.getColor ( "Panel.background" ));
      
      info = getChartRenderingInfo();

      XYPlot plot = chart.getXYPlot();
      plot.setBackgroundPaint(Color.white);
      plot.setDomainGridlinesVisible(true);
      plot.setDomainGridlinePaint(Color.BLACK);
      plot.setRangeGridlinesVisible(true);
      plot.setRangeGridlinePaint(Color.BLACK);
      
      final XYPlot temperaturePlot = (XYPlot) chart.getPlot();
      
      temperaturePlot.setRangeAxisLocation(AxisLocation.BOTTOM_OR_LEFT);

      // Make points visible
      XYLineAndShapeRenderer temperatureRenderer = new XYLineAndShapeRenderer(true,false);
      temperatureRenderer.setSeriesShapesVisible(0, true);
      temperaturePlot.setRenderer(temperatureRenderer);

      temperaturePlot.addRangeMarker(liquidusMarker);
      liquidusMarker.setPaint(Color.black);
      liquidusMarker.setLabel("Liquidus"); 
      liquidusMarker.setLabelBackgroundColor(Color.white);
      liquidusMarker.setLabelAnchor(RectangleAnchor.BOTTOM_RIGHT);
      liquidusMarker.setLabelTextAnchor(TextAnchor.TOP_RIGHT);

      temperaturePlot.addAnnotation(profileName);
      profileName.setTextAnchor(TextAnchor.TOP_LEFT);

      addChartMouseListener(this);
      addMouseMotionListener(this);
      addMouseListener(this);

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
   private void updateProfileChart(OvenCommunication oven) throws OvenCommunicationException {

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
   public void update(OvenCommunication oven) {
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

   @Override
   public void chartMouseClicked(ChartMouseEvent event) {
   }

   @Override
   public void chartMouseMoved(ChartMouseEvent event) {
   }

//   @Override
//   public void mouseClicked(MouseEvent event) {
//   }

   @Override
   public void mouseDragged(MouseEvent event) {
      try {
         if (canMove) {
            int itemIndex = xyItemEntity.getItem();
            XYPlot xy = chart.getXYPlot();
            ChartPanel localChartPanel = this;
            Rectangle2D dataArea = localChartPanel.getChartRenderingInfo().getPlotInfo().getDataArea();
            Point2D p = localChartPanel.translateScreenToJava2D(event.getPoint());
            finalMovePointY = xy.getRangeAxis().java2DToValue(p.getY(), dataArea, xy.getRangeAxisEdge());
            double difference = finalMovePointY - initialMovePointY;
            if (profileSeries.getY(itemIndex).doubleValue() + difference > xy.getRangeAxis().getRange().getLength()
                  || profileSeries.getY(itemIndex).doubleValue() + difference < 0.0D) {
               initialMovePointY = finalMovePointY;
            }
            // Restrict movement for upper and lower limit (upper limit
            // Should be as per application needs)
            double targetPoint = profileSeries.getY(itemIndex).doubleValue() + difference;
            if (targetPoint > 300 || targetPoint < 0) {
               return;
            }
            profileSeries.updateByIndex(itemIndex, (Double)targetPoint);
            chart.fireChartChanged();
            localChartPanel.updateUI();
            initialMovePointY = finalMovePointY;
         }
      } catch (Exception e) {
         e.printStackTrace();
         System.out.println(e);
      }
   }

   @Override
   public void mouseEntered(MouseEvent event) {
   }

   public void mouseExited(MouseEvent event) {
      canMove = false; // stop movement if cursor is moved out from the chart area
      initialMovePointY = 0;
      setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
   }

   public void mouseMoved(MouseEvent event) {
   }

   public void mousePressed(MouseEvent event) {
      int x = event.getX(); // initialized point whenever mouse is pressed
      int y = event.getY();
      EntityCollection entities = info.getEntityCollection();
      ChartMouseEvent cme = new ChartMouseEvent(chart, event, entities.getEntity(x, y));
      ChartEntity entity = cme.getEntity();
      if ((entity == null) || !(entity instanceof XYItemEntity)) {
         xyItemEntity = null;
         return;
      }
      xyItemEntity = (XYItemEntity) entity;
      XYPlot xy = chart.getXYPlot();
      Rectangle2D dataArea = getChartRenderingInfo().getPlotInfo().getDataArea();
      Point2D p = translateScreenToJava2D(event.getPoint());
      initialMovePointY = xy.getRangeAxis().java2DToValue(p.getY(), dataArea, xy.getRangeAxisEdge());
      canMove = true;
      setCursor(new Cursor(Cursor.HAND_CURSOR));
   }

   public void mouseReleased(MouseEvent event) {
      // stop dragging on mouse released
      canMove = false;
      initialMovePointY = 0;
      setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
   }

}
