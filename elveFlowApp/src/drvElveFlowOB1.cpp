/* drvUSBelveFlow_V1.cpp
 *
 * Driver for Measurement Computing USB-1608G multi-function DAQ board using asynPortDriver base class
 *
 * This version implements only simple analog outputs
 *
 * Mark Rivers
 * April 14, 2012
*/

#include <iocsh.h>
#include <asynPortDriver.h>

#include <Elveflow64.h>

#include <epicsExport.h>

static const char *driverName = "USBelveFlow";

// Analog output parameters, set Frequency of measurements up t0 100 Hz.
#define EFSetPressureString      "EF_SET_PRESSURE" // analogOutValue ANALOG_OUT_VALUE

// OKS #define NUM_ANALOG_OUT  2   // Number of analog outputs on 1608G
// OKS #define MAX_SIGNALS     NUM_ANALOG_OUT
// OKS I have all signals as singles, one board, maxAddr = 1 in the new version of AsynPortDriver constructor

/** Class definition for the USBelveFlow class
  */
class USBelveFlow : public asynPortDriver {
public:
  USBelveFlow(const char *portName);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);

protected:
  // Analog output parameters
  int setPressure_;
// OKS  #define FIRST_USBelveFlow_PARAM  analogOutValue_  #OKS no longer needed
// OKS  #define LAST_USBelveFlow_PARAM   analogOutValue_  #OKS no longer needed

private:
  int _MyOB1_ID;
  double *_Calibration;// define the cailbration (array of double). Size can vary, depending on the instrument but 1000 is always enough, will allocate in constructor
};

// OKS#define NUM_PARAMS ((int)(&LAST_USBelveFlow_PARAM - &FIRST_USBelveFlow_PARAM + 1))  #OKS no longer needed

/** Constructor for the USBelveFlow class
  */
USBelveFlow::USBelveFlow(const char *portName)
  : asynPortDriver( portName, 
                    1,                  // * maxAddr* /
      asynInt32Mask | asynDrvUserMask,  // Interfaces that we implement
      0,                                // Interfaces that do callbacks
      ASYN_CANBLOCK,                    //* ASYN_CANBLOCK=1, 
      1,                                // autoConnect=1 */
      0, 0)  /* Default priority and stack size */
{
  _MyOB1_ID = -1;  // initialized myOB1ID at negative value (after initialization it should become positive or =0)
                  // initialize the OB1 -> Use NiMAX to determine the device name
                  //avoid non alphanumeric characters in device name
  _Calibration = new double[1000]; // Size can vary, depending on the instrument but 1000 is always enough
  int error=0;
  error = OB1_Initialization("01C8453E", Z_regulator_type__0_2000_mbar, Z_regulator_type__0_2000_mbar, Z_regulator_type__0_8000_mbar, Z_regulator_type__0_8000_mbar, &_MyOB1_ID);
  // ID is found via NIMAX software as instructed. Channels are preconfiggured/hardware installed by ElveFlow
 // OKS to deleted Check_Error(error); // error send if not recognized, it is claimed API has it, I did not see it, it is part of NI MAX software, there should be an include some place
                      // Judging by the comments in    OB1.cpp this Check_Error() may include some GUI, since they they "will pop up" 
  printf("error = %d, _MyOB1_ID = %d \n", error, _MyOB1_ID);
  // Analog output parameters
  if(error==0){
     Elveflow_Calibration_Default(_Calibration, 1000); //use default calibration
  }
  createParam(EFSetPressureString, asynParamInt32, &setPressure_);
}

asynStatus USBelveFlow::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  // Analog outputs are 16-bit devices
  if (function == setPressure_) {
    *low = 0;
    *high =2000; //Channels 1 and 2 can go only up 2000 mbars
    return(asynSuccess);
  } else {
    return(asynError);
  }
}


asynStatus USBelveFlow::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  // OKS What if addr is screwed up somehow

  setIntegerParam(addr, function, value);

  // Analog output functions
  if (function == setPressure_) {
    status = OB1_Set_Press(_MyOB1_ID, 1, value, _Calibration, 1000);
    // I know numbers needs to be chaged to constants
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
  return (status==0) ? asynSuccess : asynError;
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
