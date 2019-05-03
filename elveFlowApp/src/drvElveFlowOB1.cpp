/* drvUSBelveFlow.cpp
 *
 * Driver for elveFlow OB1 using asynPortDriver base class
 *
 * This version implements 
 * set pressure
 * read pressure
 * read sensor
 * ...
 *
 * Oksana Ivashkevych 
 * March 2019
*/

#include <iocsh.h>
#include <asynPortDriver.h>

#include <Elveflow64.h>

#include <epicsExport.h>
#include <epicsExit.h>
#include <iostream>   //if want to use cout
using namespace std;  //f want to use cout

// Forward function definitions
static void exitCallbackC(void *drvPvt);

static const char *driverName = "USBelveFlow";

//Sensor type parameters
#define EFSensorTypeString        "EF_Z_SENSOR_TYPE"

// Analog output parameters, set Frequency of measurements up t0 100 Hz.
#define EFSetPressureString      "EF_SET_PRESSURE" 

// Analog input parameters
#define EFReadPressureString      "EF_GET_PRESSURE"
#define EFReadFlowSting           "EF_GET_FLOW"

//This is a multidevice with 4 identical channels
#define MAX_SIGNALS 4

/** Class definition for the USBelveFlow class
  */
class USBelveFlow : public asynPortDriver {
public:
  USBelveFlow(const char *portName);
  ~USBelveFlow();
  void setAllPressure(int p1=0);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value); 
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual void report(FILE *fp, int details);

protected:
  int sensorType_;

  int setPressure_;
 
  int readPressure_;
  int readSensor_;

private:
  int _MyOB1_ID;
  double *_Calibration; // define the cailbration (array of double). 
                        // Size can vary, depending on the instrument but 1000 is always enough.
                        // will allocate in constructor
};



/** Constructor for the USBelveFlow class
  */
USBelveFlow::USBelveFlow(const char *portName)
  : asynPortDriver( portName, 
                    MAX_SIGNALS,                             // * maxAddr* /
      asynInt32Mask | asynFloat64Mask | asynDrvUserMask,    // Interfaces that we implement
      asynInt32Mask | asynFloat64Mask,                      // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK,                     //* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE =1 
      1,                                                    // autoConnect=1 */
      0, 0)  /* Default priority and stack size */
{
  static const char *functionName = "USBelveFlow";
  int status=0;

  _MyOB1_ID = -1;  // initialized myOB1ID at negative value (after initialization it should become positive or =0)
                  // initialize the OB1 -> Use NiMAX to determine the device name
                  //avoid non alphanumeric characters in device name
  _Calibration = new double[1000]; // Size can vary, depending on the instrument but 1000 is always enough

  status = OB1_Initialization("01C8453E", Z_regulator_type__0_2000_mbar, Z_regulator_type__0_2000_mbar, Z_regulator_type__0_8000_mbar, Z_regulator_type__0_8000_mbar, &_MyOB1_ID);
  // ID is found via NIMAX software. Should be configurable from epics record 
  if (status ==- 1)
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s device not found\n", driverName, functionName);
  else 
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s device found\n", driverName, functionName);

  // Add digital flow sensor with H2O Calibration
  /* OKS now this is a parameter
  status = OB1_Add_Sens(_MyOB1_ID, 1, Z_sensor_type_Flow_7_muL_min, Z_Sensor_digit_analog_Analog, Z_Sensor_FSD_Calib_H2O, Z_D_F_S_Resolution__16Bit); 
  if (status ==- 1)
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s device not found\n", driverName, functionName);
  else 
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s device found\n", driverName, functionName);
 */

  if(status==0){
     status = Elveflow_Calibration_Default(_Calibration, 1000); //use default _calibration
  }
  else
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s No calibration loaded\n", driverName, functionName);
  // Sensor type param
  createParam(EFSensorTypeString, asynParamInt32, &sensorType_);

  // Analog output parameters
  createParam(EFSetPressureString,    asynParamFloat64, &setPressure_);

  // Analog Input parameters
  createParam(EFReadFlowSting,        asynParamFloat64, &readSensor_);
  createParam(EFReadPressureString,   asynParamFloat64, &readPressure_);

  //read pressure for bumpless reboot
  double fVal;
  int channel =1;

  status = OB1_Get_Press(_MyOB1_ID, channel, 1, _Calibration, &fVal, 1000);
  //do smth with status: log, report

  setDoubleParam(readPressure_, fVal);
  setDoubleParam(setPressure_, fVal);
  // Set exit handler to clean up
  epicsAtExit(exitCallbackC, this);
}

 USBelveFlow::~USBelveFlow()
 {
  setAllPressure();
  OB1_Destructor(_MyOB1_ID);
  delete[] _Calibration;
 }

asynStatus USBelveFlow::writeInt32(asynUser *pasynUser, epicsInt32 value){
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);

  if (function == sensorType_) {
    cout<<"addr= "<<addr<<endl;
    status = OB1_Add_Sens(_MyOB1_ID, addr+1, value, Z_Sensor_digit_analog_Analog, Z_Sensor_FSD_Calib_H2O, Z_D_F_S_Resolution__16Bit); 
    if (status ==- 1){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s device not found\n", driverName, functionName);
    }

    else {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s device found\n", driverName, functionName);
    }
  }
  callParamCallbacks(addr);
  //If more params are added, consider adding asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, ....
  return (status == 0) ? asynSuccess : asynError;

}

asynStatus USBelveFlow::writeFloat64(asynUser *pasynUser, epicsFloat64 value){
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeFloat64";

  this->getAddress(pasynUser, &addr);

  setDoubleParam(addr, function, value);

  // Analog output functions
  if (function == setPressure_) {
    status = OB1_Set_Press(_MyOB1_ID, addr+1, value, _Calibration, 1000);
    // Numbers needs to be chaged to constants    
  }

  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote %d to address %d\n",
             driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing %d to address %d, status=%d\n",
             driverName, functionName, this->portName, value, addr, status);
  }

  return (status == 0) ? asynSuccess : asynError;
}


asynStatus USBelveFlow::readFloat64(asynUser *pasynUser, epicsFloat64 *value){
  int addr;
  int function = pasynUser->reason;
  int status=0;
  double fVal = -1;
  int filter=0;
  static const char *functionName = "readFloat64";

  this->getAddress(pasynUser, &addr);

  if (function == readPressure_) { 
    status = OB1_Get_Press(_MyOB1_ID, addr+1, 1, _Calibration, &fVal, 1000);
    *value = fVal;
    setDoubleParam(addr, readPressure_, *value);
  }

  else if(function == readSensor_){
    status = OB1_Get_Sens_Data(_MyOB1_ID, addr+1, 1, &fVal);  
    *value = fVal;
    setDoubleParam(addr, readSensor_, *value);
  }
  // Other functions we call the base class method
  else {
     status = asynPortDriver::readFloat64(pasynUser, value);
  }
  callParamCallbacks(addr);
  //If more params are added, consider adding asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, ....
  return (status == 0) ? asynSuccess : asynError;

}

void USBelveFlow::setAllPressure(int p1){
  // Sets all pressure to val in mbars, useful to bring all channels to 0. 
  // Caution! as different channels can have different ranges.
  double set_all_pressure [4];
  for (int i = 0; i < 4; i++)//Init the pressure array
      {
        set_all_pressure[i] = p1;// create the array with all data
      }
      OB1_Set_All_Press(_MyOB1_ID, set_all_pressure, _Calibration, 4, 1000);
}

/* Report parameters */ 
void USBelveFlow::report(FILE *fp, int details){
  fprintf(fp, " Port: %s \n", this->portName); 
  asynPortDriver::report(fp, details); 
}

//_____________________________________________________________________________________________

/** Callback function that is called by EPICS when the IOC exits */

static void exitCallbackC(void *pPvt)
{
  USBelveFlow *pUSBelveFlow = (USBelveFlow*) pPvt;
  delete(pUSBelveFlow);
}

/** Configuration command, called directly or from iocsh */
extern "C" int USBelveFlowConfig(const char *portName)
{
  new USBelveFlow(portName);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg * const configArgs[] = {&configArg0};
static const iocshFuncDef configFuncDef = {"USBelveFlowConfig", 1, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  USBelveFlowConfig(args[0].sval);
}

void drvUSBelveFlowRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvUSBelveFlowRegister);
}
