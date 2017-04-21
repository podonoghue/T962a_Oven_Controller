package testingChart;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.EventQueue;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.SwingConstants;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import testingChart.OvenCommunication.OvenCommunicationException;

public class ReflowApplication {

   private JFrame frame;

   private JTextField descriptionField;
   private JTextField liquidusField;
   private JTextField preheatTimeField;
   private JTextField soakTemp1Field;
   private JTextField soakTemp2Field;
   private JTextField soakTimeField;
   private JTextField rampUpSlopeField;
   private JTextField peakTempField;
   private JTextField peakDwellField;
   private JTextField rampDownSlopeField;

   /**
    * Launch the application.
    */
   public static void main(String[] args) {
      EventQueue.invokeLater(new Runnable() {
         public void run() {
            try {
               ReflowApplication window = new ReflowApplication();
               window.frame.setVisible(true);
            } catch (Exception e) {
               e.printStackTrace();
            }
         }
      });
   }

   /**
    * Create the application.
    */
   public ReflowApplication() {
      initialize();
   }


   private void setProfileNumber(Number number) throws OvenCommunicationException {
      OvenCommunication oven = new OvenCommunication();
      oven.selectProfile(number);
   }
   
   /**
    * Initialize the contents of the frame.
    */
   private void initialize() {
      frame = new JFrame();
      frame.setBounds(100, 100, 1058, 678);
      frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      
      JTabbedPane tabbedPane = new JTabbedPane(JTabbedPane.TOP);
      frame.getContentPane().add(tabbedPane, BorderLayout.CENTER);
      
      JPanel reflowPanel = new JPanel();
      tabbedPane.addTab("Reflow", null, reflowPanel, null);
      reflowPanel.setLayout(new BorderLayout(0, 0));
      
      ReflowChartPanel chartPanel = new ReflowChartPanel();
      reflowPanel.add(chartPanel, BorderLayout.CENTER);
      chartPanel.setLayout(new FlowLayout(FlowLayout.CENTER, 5, 5));
      
      JPanel reflowButtonPanel = new JPanel();
      reflowPanel.add(reflowButtonPanel, BorderLayout.SOUTH);
      reflowButtonPanel.setLayout(new FlowLayout(FlowLayout.CENTER, 5, 5));
      
      JButton btnUpdate = new JButton("Update");
      btnUpdate.addActionListener(new ActionListener() {
         public void actionPerformed(ActionEvent arg0) {
            chartPanel.update();
         }
      });
      reflowButtonPanel.add(btnUpdate);
      
      JButton btnReflow = new JButton("Reflow");
      btnReflow.addActionListener(new ActionListener() {
         public void actionPerformed(ActionEvent arg0) {
            OvenCommunication oven = new OvenCommunication();
            try {
               oven.startReflow();
            } catch (OvenCommunicationException e) {
               JOptionPane.showMessageDialog(frame, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
               try {
                  oven.close();
               } catch (OvenCommunicationException e1) {
               }
            }
         }
      });
      reflowButtonPanel.add(btnReflow);
      
      JButton btnAbort = new JButton("Abort");
      btnAbort.addActionListener(new ActionListener() {
         public void actionPerformed(ActionEvent arg0) {
            OvenCommunication oven = new OvenCommunication();
            try {
               oven.abortReflow();
            } catch (OvenCommunicationException e) {
               JOptionPane.showMessageDialog(frame, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
               try {
                  oven.close();
               } catch (OvenCommunicationException e1) {
               }
            }
         }
      });
      reflowButtonPanel.add(btnAbort);
      
      JButton btnTry = new JButton("Try");
      btnTry.addActionListener(new ActionListener() {
         public void actionPerformed(ActionEvent arg0) {
            OvenCommunication oven = new OvenCommunication();
            try {
               SolderProfile profile = new SolderProfile("xx", SolderProfile.UNLOCKED, 120, 20, 100, 200, 20, 1.5f, 250, 10, -3.4f);
               System.err.println("Profile = " + profile);
               oven.setProfile(4, profile);
//               PidParameters pidParameters = oven.getPid();
//               System.err.println("PID = " + pidParameters);
//               oven.setPid(new PidParameters(.2f, .066f, 66.66f));
//               Thermocouple setting[] = {
//                     new Thermocouple(true, -3.0f),
//                     new Thermocouple(true, -3.0f),
//                     new Thermocouple(true, -3.0f),
//                     new Thermocouple(true, -3.0f) };
//               oven.setThermocouples(setting);
            } catch (OvenCommunicationException e) {
               JOptionPane.showMessageDialog(frame, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
               try {
                  oven.close();
               } catch (OvenCommunicationException e1) {
               }
            }
         }
      });
      reflowButtonPanel.add(btnTry);
      
      JPanel profilesPanel = new JPanel();
      tabbedPane.addTab("Profiles", null, profilesPanel, null);
      profilesPanel.setLayout(new BorderLayout(0, 0));
      
      ProfileChartPanel profilePanel = new ProfileChartPanel();
      profilesPanel.add(profilePanel, BorderLayout.CENTER);
      
      JPanel parameterPanel = new JPanel();
      profilesPanel.add(parameterPanel, BorderLayout.EAST);
      GridBagLayout gbl_parameterPanel = new GridBagLayout();
      gbl_parameterPanel.columnWeights = new double[]{1.0, 0.0, 0.0};
      gbl_parameterPanel.rowWeights = new double[]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
      parameterPanel.setLayout(gbl_parameterPanel);
      
      JLabel lblParameters = new JLabel("Profile Parameters");
      lblParameters.setFont(new Font("Tahoma", Font.PLAIN, 18));
      lblParameters.setAlignmentX(Component.CENTER_ALIGNMENT);
      lblParameters.setHorizontalAlignment(SwingConstants.CENTER);
      GridBagConstraints gbc_lblParameters = new GridBagConstraints();
      gbc_lblParameters.weighty = 0.1;
      gbc_lblParameters.gridwidth = 3;
      gbc_lblParameters.fill = GridBagConstraints.HORIZONTAL;
      gbc_lblParameters.insets = new Insets(0, 0, 5, 0);
      gbc_lblParameters.gridx = 0;
      gbc_lblParameters.gridy = 1;
      parameterPanel.add(lblParameters, gbc_lblParameters);
      
      JLabel lblNewLabel = new JLabel("Profile No");
      GridBagConstraints gbc_lblNewLabel = new GridBagConstraints();
      gbc_lblNewLabel.weighty = 0.1;
      gbc_lblNewLabel.fill = GridBagConstraints.BOTH;
      gbc_lblNewLabel.insets = new Insets(0, 0, 5, 5);
      gbc_lblNewLabel.gridx = 0;
      gbc_lblNewLabel.gridy = 2;
      parameterPanel.add(lblNewLabel, gbc_lblNewLabel);
      
      JSpinner profileNumberSpinner = new JSpinner(new SpinnerNumberModel(0, 0, 9, 1));
      profileNumberSpinner.addChangeListener(new ChangeListener() {
         public void stateChanged(ChangeEvent arg0) {
            SpinnerNumberModel model = (SpinnerNumberModel) profileNumberSpinner.getModel();
            System.err.println("Value = " + model.getNumber());
            try {
               setProfileNumber(model.getNumber());
               profilePanel.update();
            } catch (OvenCommunicationException e) {
               JOptionPane.showMessageDialog(frame, e.getMessage(), "Communication error", JOptionPane.ERROR_MESSAGE);
            }
         }
      });
//      profileNumberSpinner.addVetoableChangeListener(new VetoableChangeListener() {
//         public void vetoableChange(PropertyChangeEvent arg0) throws PropertyVetoException {
//            SpinnerNumberModel model = (SpinnerNumberModel) profileNumberSpinner.getModel();
//            System.err.println("Value = " + model.getNumber());
//            try {
//               setProfileNumber(model.getNumber());
//            } catch (OvenCommunicationException e) {
//               throw new PropertyVetoException(e.getMessage(), arg0);
//            }
//         }
//      });
      GridBagConstraints gbc_profileNumberSpinner = new GridBagConstraints();
      gbc_profileNumberSpinner.weighty = 0.1;
      gbc_profileNumberSpinner.fill = GridBagConstraints.BOTH;
      gbc_profileNumberSpinner.insets = new Insets(0, 0, 5, 5);
      gbc_profileNumberSpinner.gridx = 1;
      gbc_profileNumberSpinner.gridy = 2;
      parameterPanel.add(profileNumberSpinner, gbc_profileNumberSpinner);
      
      descriptionField = new JTextField();
      GridBagConstraints gbc_descriptionField = new GridBagConstraints();
      gbc_descriptionField.weighty = 0.1;
      gbc_descriptionField.gridwidth = 2;
      gbc_descriptionField.fill = GridBagConstraints.BOTH;
      gbc_descriptionField.insets = new Insets(0, 0, 5, 5);
      gbc_descriptionField.gridx = 0;
      gbc_descriptionField.gridy = 3;
      parameterPanel.add(descriptionField, gbc_descriptionField);
      descriptionField.setColumns(10);
      
      JLabel lblLiquidus = new JLabel("Liquidus Temp");
      GridBagConstraints gbc_lblLiquidus = new GridBagConstraints();
      gbc_lblLiquidus.weighty = 0.1;
      gbc_lblLiquidus.fill = GridBagConstraints.BOTH;
      gbc_lblLiquidus.insets = new Insets(0, 0, 5, 5);
      gbc_lblLiquidus.gridx = 0;
      gbc_lblLiquidus.gridy = 4;
      parameterPanel.add(lblLiquidus, gbc_lblLiquidus);
      
      liquidusField = new JTextField();
      GridBagConstraints gbc_liquidusField = new GridBagConstraints();
      gbc_liquidusField.weighty = 0.1;
      gbc_liquidusField.fill = GridBagConstraints.BOTH;
      gbc_liquidusField.insets = new Insets(0, 0, 5, 5);
      gbc_liquidusField.gridx = 1;
      gbc_liquidusField.gridy = 4;
      parameterPanel.add(liquidusField, gbc_liquidusField);
      liquidusField.setColumns(10);
      
      JLabel lblLiquidusUnit = new JLabel(" C");
      GridBagConstraints gbc_lblLiquidusUnit = new GridBagConstraints();
      gbc_lblLiquidusUnit.weighty = 0.1;
      gbc_lblLiquidusUnit.fill = GridBagConstraints.BOTH;
      gbc_lblLiquidusUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblLiquidusUnit.gridx = 2;
      gbc_lblLiquidusUnit.gridy = 4;
      parameterPanel.add(lblLiquidusUnit, gbc_lblLiquidusUnit);
      
      JLabel lblPreheatTime = new JLabel("Preheat Time");
      GridBagConstraints gbc_lblPreheatTime = new GridBagConstraints();
      gbc_lblPreheatTime.weighty = 0.1;
      gbc_lblPreheatTime.fill = GridBagConstraints.BOTH;
      gbc_lblPreheatTime.insets = new Insets(0, 0, 5, 5);
      gbc_lblPreheatTime.gridx = 0;
      gbc_lblPreheatTime.gridy = 5;
      parameterPanel.add(lblPreheatTime, gbc_lblPreheatTime);
      
      preheatTimeField = new JTextField();
      GridBagConstraints gbc_textField = new GridBagConstraints();
      gbc_textField.weighty = 0.1;
      gbc_textField.fill = GridBagConstraints.BOTH;
      gbc_textField.insets = new Insets(0, 0, 5, 5);
      gbc_textField.gridx = 1;
      gbc_textField.gridy = 5;
      parameterPanel.add(preheatTimeField, gbc_textField);
      preheatTimeField.setColumns(10);
      
      JLabel lblPreheatTimeUnit = new JLabel(" s");
      GridBagConstraints gbc_lblPreheatTimeUnit = new GridBagConstraints();
      gbc_lblPreheatTimeUnit.weighty = 0.1;
      gbc_lblPreheatTimeUnit.fill = GridBagConstraints.BOTH;
      gbc_lblPreheatTimeUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblPreheatTimeUnit.gridx = 2;
      gbc_lblPreheatTimeUnit.gridy = 5;
      parameterPanel.add(lblPreheatTimeUnit, gbc_lblPreheatTimeUnit);
      
      JLabel lblSoakTemp1 = new JLabel("Soak Temp 1");
      GridBagConstraints gbc_lblSoakTemp1 = new GridBagConstraints();
      gbc_lblSoakTemp1.weighty = 0.1;
      gbc_lblSoakTemp1.fill = GridBagConstraints.BOTH;
      gbc_lblSoakTemp1.insets = new Insets(0, 0, 5, 5);
      gbc_lblSoakTemp1.gridx = 0;
      gbc_lblSoakTemp1.gridy = 6;
      parameterPanel.add(lblSoakTemp1, gbc_lblSoakTemp1);
      
      soakTemp1Field = new JTextField();
      GridBagConstraints gbc_soakTemp1Field = new GridBagConstraints();
      gbc_soakTemp1Field.fill = GridBagConstraints.BOTH;
      gbc_soakTemp1Field.insets = new Insets(0, 0, 5, 5);
      gbc_soakTemp1Field.gridx = 1;
      gbc_soakTemp1Field.gridy = 6;
      parameterPanel.add(soakTemp1Field, gbc_soakTemp1Field);
      soakTemp1Field.setColumns(10);
      
      JLabel lblsoakTemp1FieldUnit = new JLabel(" C");
      GridBagConstraints gbc_lblsoakTemp1FieldUnit = new GridBagConstraints();
      gbc_lblsoakTemp1FieldUnit.weighty = 0.1;
      gbc_lblsoakTemp1FieldUnit.fill = GridBagConstraints.BOTH;
      gbc_lblsoakTemp1FieldUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblsoakTemp1FieldUnit.gridx = 2;
      gbc_lblsoakTemp1FieldUnit.gridy = 6;
      parameterPanel.add(lblsoakTemp1FieldUnit, gbc_lblsoakTemp1FieldUnit);
      
      JLabel lblSoakTemp2 = new JLabel("Soak Temp 2");
      GridBagConstraints gbc_lblSoakTemp2 = new GridBagConstraints();
      gbc_lblSoakTemp2.weighty = 0.1;
      gbc_lblSoakTemp2.fill = GridBagConstraints.BOTH;
      gbc_lblSoakTemp2.insets = new Insets(0, 0, 5, 5);
      gbc_lblSoakTemp2.gridx = 0;
      gbc_lblSoakTemp2.gridy = 7;
      parameterPanel.add(lblSoakTemp2, gbc_lblSoakTemp2);
      
      soakTemp2Field = new JTextField();
      GridBagConstraints gbc_soakTemp2Field = new GridBagConstraints();
      gbc_soakTemp2Field.weighty = 0.1;
      gbc_soakTemp2Field.fill = GridBagConstraints.BOTH;
      gbc_soakTemp2Field.insets = new Insets(0, 0, 5, 5);
      gbc_soakTemp2Field.gridx = 1;
      gbc_soakTemp2Field.gridy = 7;
      parameterPanel.add(soakTemp2Field, gbc_soakTemp2Field);
      soakTemp2Field.setColumns(10);
      
      JLabel lblsoakTemp2FieldUnit = new JLabel(" C");
      GridBagConstraints gbc_lblsoakTemp2FieldUnit = new GridBagConstraints();
      gbc_lblsoakTemp2FieldUnit.weighty = 0.1;
      gbc_lblsoakTemp2FieldUnit.fill = GridBagConstraints.BOTH;
      gbc_lblsoakTemp2FieldUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblsoakTemp2FieldUnit.gridx = 2;
      gbc_lblsoakTemp2FieldUnit.gridy = 7;
      parameterPanel.add(lblsoakTemp2FieldUnit, gbc_lblsoakTemp2FieldUnit);
      
      JLabel lblSoakTime = new JLabel("Soak Time");
      GridBagConstraints gbc_lblSoakTime = new GridBagConstraints();
      gbc_lblSoakTime.weighty = 0.1;
      gbc_lblSoakTime.fill = GridBagConstraints.BOTH;
      gbc_lblSoakTime.insets = new Insets(0, 0, 5, 5);
      gbc_lblSoakTime.gridx = 0;
      gbc_lblSoakTime.gridy = 8;
      parameterPanel.add(lblSoakTime, gbc_lblSoakTime);
      
      soakTimeField = new JTextField();
      GridBagConstraints gbc_soakTime = new GridBagConstraints();
      gbc_soakTime.weighty = 0.1;
      gbc_soakTime.fill = GridBagConstraints.BOTH;
      gbc_soakTime.insets = new Insets(0, 0, 5, 5);
      gbc_soakTime.gridx = 1;
      gbc_soakTime.gridy = 8;
      parameterPanel.add(soakTimeField, gbc_soakTime);
      soakTimeField.setColumns(10);
      
      JLabel lblsoakTimeUnit = new JLabel(" s");
      GridBagConstraints gbc_lblsoakTimeUnit = new GridBagConstraints();
      gbc_lblsoakTimeUnit.weighty = 0.1;
      gbc_lblsoakTimeUnit.fill = GridBagConstraints.BOTH;
      gbc_lblsoakTimeUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblsoakTimeUnit.gridx = 2;
      gbc_lblsoakTimeUnit.gridy = 8;
      parameterPanel.add(lblsoakTimeUnit, gbc_lblsoakTimeUnit);
      
      JLabel lblRampUpSlope = new JLabel("Ramp Up Rate");
      GridBagConstraints gbc_lblRampUpSlopw = new GridBagConstraints();
      gbc_lblRampUpSlopw.weighty = 0.1;
      gbc_lblRampUpSlopw.fill = GridBagConstraints.BOTH;
      gbc_lblRampUpSlopw.insets = new Insets(0, 0, 5, 5);
      gbc_lblRampUpSlopw.gridx = 0;
      gbc_lblRampUpSlopw.gridy = 9;
      parameterPanel.add(lblRampUpSlope, gbc_lblRampUpSlopw);
      
      rampUpSlopeField = new JTextField();
      GridBagConstraints gbc_rampUpSlopeField = new GridBagConstraints();
      gbc_rampUpSlopeField.weighty = 0.1;
      gbc_rampUpSlopeField.fill = GridBagConstraints.BOTH;
      gbc_rampUpSlopeField.insets = new Insets(0, 0, 5, 5);
      gbc_rampUpSlopeField.gridx = 1;
      gbc_rampUpSlopeField.gridy = 9;
      parameterPanel.add(rampUpSlopeField, gbc_rampUpSlopeField);
      rampUpSlopeField.setColumns(10);
      
      JLabel lblRampUpSlopeUnit = new JLabel(" C/s ");
      GridBagConstraints gbc_lblRampUpSlopeUnit = new GridBagConstraints();
      gbc_lblRampUpSlopeUnit.weighty = 0.1;
      gbc_lblRampUpSlopeUnit.fill = GridBagConstraints.BOTH;
      gbc_lblRampUpSlopeUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblRampUpSlopeUnit.gridx = 2;
      gbc_lblRampUpSlopeUnit.gridy = 9;
      parameterPanel.add(lblRampUpSlopeUnit, gbc_lblRampUpSlopeUnit);
      
      JLabel lblPeakTemp = new JLabel("Peak Temp");
      GridBagConstraints gbc_lblPeakTemp = new GridBagConstraints();
      gbc_lblPeakTemp.weighty = 0.1;
      gbc_lblPeakTemp.fill = GridBagConstraints.BOTH;
      gbc_lblPeakTemp.insets = new Insets(0, 0, 5, 5);
      gbc_lblPeakTemp.gridx = 0;
      gbc_lblPeakTemp.gridy = 10;
      parameterPanel.add(lblPeakTemp, gbc_lblPeakTemp);
      
      peakTempField = new JTextField();
      GridBagConstraints gbc_peakTempField = new GridBagConstraints();
      gbc_peakTempField.weighty = 0.1;
      gbc_peakTempField.fill = GridBagConstraints.BOTH;
      gbc_peakTempField.insets = new Insets(0, 0, 5, 5);
      gbc_peakTempField.gridx = 1;
      gbc_peakTempField.gridy = 10;
      parameterPanel.add(peakTempField, gbc_peakTempField);
      peakTempField.setColumns(10);
      
      JLabel lblpeakTempFieldUnit = new JLabel(" C");
      GridBagConstraints gbc_lblpeakTempFieldUnit = new GridBagConstraints();
      gbc_lblpeakTempFieldUnit.weighty = 0.1;
      gbc_lblpeakTempFieldUnit.fill = GridBagConstraints.BOTH;
      gbc_lblpeakTempFieldUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblpeakTempFieldUnit.gridx = 2;
      gbc_lblpeakTempFieldUnit.gridy = 10;
      parameterPanel.add(lblpeakTempFieldUnit, gbc_lblpeakTempFieldUnit);
      
      JLabel lblPeakDwell = new JLabel("Peak Dwell");
      GridBagConstraints gbc_lblPeakDwell = new GridBagConstraints();
      gbc_lblPeakDwell.weighty = 0.1;
      gbc_lblPeakDwell.fill = GridBagConstraints.BOTH;
      gbc_lblPeakDwell.insets = new Insets(0, 0, 5, 5);
      gbc_lblPeakDwell.gridx = 0;
      gbc_lblPeakDwell.gridy = 11;
      parameterPanel.add(lblPeakDwell, gbc_lblPeakDwell);
      
      peakDwellField = new JTextField();
      GridBagConstraints gbc_peakDwellField = new GridBagConstraints();
      gbc_peakDwellField.weighty = 0.1;
      gbc_peakDwellField.fill = GridBagConstraints.BOTH;
      gbc_peakDwellField.insets = new Insets(0, 0, 5, 5);
      gbc_peakDwellField.gridx = 1;
      gbc_peakDwellField.gridy = 11;
      parameterPanel.add(peakDwellField, gbc_peakDwellField);
      peakDwellField.setColumns(10);
      
      JLabel lblpeakDwellUnit = new JLabel(" s");
      GridBagConstraints gbc_lblpeakDwellUnit = new GridBagConstraints();
      gbc_lblpeakDwellUnit.weighty = 0.1;
      gbc_lblpeakDwellUnit.fill = GridBagConstraints.BOTH;
      gbc_lblpeakDwellUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblpeakDwellUnit.gridx = 2;
      gbc_lblpeakDwellUnit.gridy = 11;
      parameterPanel.add(lblpeakDwellUnit, gbc_lblpeakDwellUnit);
      
      JLabel lblRampDownSlope = new JLabel("Ramp Down Rate");
      GridBagConstraints gbc_lblRampDownSlope = new GridBagConstraints();
      gbc_lblRampDownSlope.weighty = 0.1;
      gbc_lblRampDownSlope.fill = GridBagConstraints.BOTH;
      gbc_lblRampDownSlope.insets = new Insets(0, 0, 5, 5);
      gbc_lblRampDownSlope.gridx = 0;
      gbc_lblRampDownSlope.gridy = 12;
      parameterPanel.add(lblRampDownSlope, gbc_lblRampDownSlope);
      
      rampDownSlopeField = new JTextField();
      GridBagConstraints gbc_rampDownSlopeField = new GridBagConstraints();
      gbc_rampDownSlopeField.weighty = 0.1;
      gbc_rampDownSlopeField.fill = GridBagConstraints.BOTH;
      gbc_rampDownSlopeField.insets = new Insets(0, 0, 5, 5);
      gbc_rampDownSlopeField.gridx = 1;
      gbc_rampDownSlopeField.gridy = 12;
      parameterPanel.add(rampDownSlopeField, gbc_rampDownSlopeField);
      rampDownSlopeField.setColumns(10);
      
      JLabel lblRampDownSlopeUnit = new JLabel(" C/s");
      GridBagConstraints gbc_lblRampDownSlopeUnit = new GridBagConstraints();
      gbc_lblRampDownSlopeUnit.weighty = 0.1;
      gbc_lblRampDownSlopeUnit.fill = GridBagConstraints.BOTH;
      gbc_lblRampDownSlopeUnit.insets = new Insets(0, 0, 5, 0);
      gbc_lblRampDownSlopeUnit.gridx = 2;
      gbc_lblRampDownSlopeUnit.gridy = 12;
      parameterPanel.add(lblRampDownSlopeUnit, gbc_lblRampDownSlopeUnit);
      
      JButton writeProfile2 = new JButton("Write Profile");
      writeProfile2.setAlignmentX(Component.CENTER_ALIGNMENT);
      GridBagConstraints gbc_writeProfile2 = new GridBagConstraints();
      gbc_writeProfile2.weighty = 0.1;
      gbc_writeProfile2.insets = new Insets(0, 0, 5, 0);
      gbc_writeProfile2.gridwidth = 3;
      gbc_writeProfile2.fill = GridBagConstraints.BOTH;
      gbc_writeProfile2.gridx = 0;
      gbc_writeProfile2.gridy = 13;
      parameterPanel.add(writeProfile2, gbc_writeProfile2);
      
      JLabel lblFiller = new JLabel("");
      GridBagConstraints gbc_lblFiller = new GridBagConstraints();
      gbc_lblFiller.gridwidth = 3;
      gbc_lblFiller.weighty = 1.0;
      gbc_lblFiller.gridx = 0;
      gbc_lblFiller.gridy = 15;
      parameterPanel.add(lblFiller, gbc_lblFiller);
   }

}
