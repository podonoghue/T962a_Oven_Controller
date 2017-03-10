/**
 * @file    SCPIInterface.cpp
 * @brief   SCPI (Very incomplete)
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */

#include <SCPIInterface.h>

#include "cmsis.h"

SCPI_Interface::Command   *SCPI_Interface::command;
SCPI_Interface::Response  *SCPI_Interface::response;

CMSIS::Thread *SCPI_Interface::handlerThread;

CMSIS::Pool<SCPI_Interface::Command, 4>     *SCPI_Interface::commandPool;
CMSIS::Pool<SCPI_Interface::Response, 4>    *SCPI_Interface::responsePool;

CMSIS::Message<SCPI_Interface::Command, 4>  *SCPI_Interface::commandQueue;
CMSIS::Message<SCPI_Interface::Response, 4> *SCPI_Interface::responseQueue;

//const SCPI_Interface::Response SCPI_Interface::IDN_RESPONSE = {"Oven\n\r", 6};
const char     *SCPI_Interface::IDN = "SMT-Oven 1.0.0.0\n\r";
