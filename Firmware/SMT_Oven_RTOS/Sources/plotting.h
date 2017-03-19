/**
 * @file    Plotting.h
 * @brief   Represents the information required for a plotting to LCD\n
 *          Includes the profile and measured temperatures.
 *
 *  Created on: 17Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_PLOTTING_H_
#define SOURCES_PLOTTING_H_

#include <temperaturePlot.h>

/**
 * Functions associated with plotting profiles and results
 */
namespace Draw {

/**
 * Clears the plot dataPoints
 */
void reset();

/**
 * Draw a profile to plot data
 *
 * @param index Index of profile to draw
 */
void drawProfile(int index);

/**
 * Update the LCD from plot data
 */
void update();

/**
 * Add data point to plot
 *
 * @param time Time index of point
 * @param dataPoint Point to add
 */
void addDataPoint(int time, DataPoint dataPoint);

/**
 * Get data point
 *
 * @param time Time index of point
 *
 * @return dataPoint for time index
 */
const DataPoint &getDataPoint(int time);

/**
 * Get reference to entire plot data
 *
 * @return TemperaturePlot
 */
TemperaturePlot &getData();

}; // end namespace Draw

#endif /* SOURCES_PLOTTING_H_ */
