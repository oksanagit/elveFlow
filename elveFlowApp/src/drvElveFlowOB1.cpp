/* drvUSBelveFlow.cpp
 *
 * Driver for elveFlow OB1 using asynPortDriver base class
 *
 * This version implements 
 * set pressure
 * read pressure
 * ...
 *
 * Oksana Ivashkevych 
 * March 2019
*/
#include <iostream> //OKS for test
#include <iocsh.h>
#include <asynPortDriver.h>

#include <Elveflow64.h>

#include <epicsExport.h>
#include <epicsExit.h>
using namespace std;  //OKS for test

// Forward function definitions
static void exitCallbackC(void *drvPvt);

static const char *driverName = "USBelveFlow";

// Analog output parameters, set Frequency of measurements up t0 100 Hz.
#define EFSetPressureString      "EF_SET_PRESSURE" 

// Analog input parameters
#define EFReadPressureString      "EF_GET_PRESSURE"
#define EFReadFlowSting           "EF_GET_FLOW"

// OKS #define NUM_ANALOG_OUT  2   // Number of analog outputs on 1608G
// OKS #define MAX_SIGNALS     NUM_ANALOG_OUT
// OKS I have all signals as singles, one board, maxAddr = 1 in the new version of AsynPortDriver constructor

/** Class definition for the USBelveFlow class
  */
class USBelveFlow : public asynPortDriver {
public:
  USBelveFlow(const char *portName);
  ~USBelveFlow();
  void setAllPressure(int p1=0);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);     // This function is not used, for the future records 
  virtual asynStatus getBounds(asynUser *pasynUser, epicsFloat64 *low, epicsFloat64 *high);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);     // This function is not used, ready for the future records
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value); 
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual void report(FILE *fp, int details);

protected:

  int setPressure_;

  
  int readPressure_;
  int readSensor_;

private:
  int _MyOB1_ID;
  double *_Calibration;// define the cailbration (array of double). Size can vary, depending on the instrument but 1000 is always enough, will allocate in constructor
};

// OKS#define NUM_PARAMS ((int)(&LAST_USBelveFlow_PARAM - &FIRST_USBelveFlow_PARAM + 1))  #OKS no longer needed

/** Constructor for the USBelveFlow class
  */
USBelveFlow::USBelveFlow(const char *portName)
  : asynPortDriver( portName, 
                    1,                                      // * maxAddr* /
      asynInt32Mask | asynFloat64Mask | asynDrvUserMask,    // Interfaces that we implement
      asynInt32Mask | asynFloat64Mask,                      // Interfaces that do callbacks
      ASYN_CANBLOCK,                                        //* ASYN_CANBLOCK=1, 
      1,                                                    // autoConnect=1 */
      0, 0)  /* Default priority and stack size */
{
  _MyOB1_ID = -1;  // initialized myOB1ID at negative value (after initialization it should become positive or =0)
                  // initialize the OB1 -> Use NiMAX to determine the device name
                  //avoid non alphanumeric characters in device name
  _Calibration = new double[1000]; // Size can vary, depending on the instrument but 1000 is always enough
  int status=0;
  status = OB1_Initialization("01C8453E", Z_regulator_type__0_2000_mbar, Z_regulator_type__0_2000_mbar, Z_regulator_type__0_8000_mbar, Z_regulator_type__0_8000_mbar, &_MyOB1_ID);
  // ID is found via NIMAX software as instructed. Channels are preconfiggured/hardware installed by ElveFlow
 // OKS to deleted Check_Error(error); // error send if not recognized, it is claimed API has it, I did not see it, it is part of NI MAX software, there should be an include some place
                      // Judging by the comments in    OB1.cpp this Check_Error() may include some GUI, since they they "will pop up" 
  printf("error = %d, _MyOB1_ID = %d \n", status, _MyOB1_ID);

  // Add a sensor
  status = OB1_Add_Sens(_MyOB1_ID, 1, Z_sensor_type_Flow_7_muL_min, Z_Sensor_digit_analog_Analog, Z_Sensor_FSD_Calib_H2O, Z_D_F_S_Resolution__16Bit);
  // Add digital flow sensor with H2O Calibration
  // ! ! ! If the sensor is not recognized a pop up will indicate it)
  cout<<"error: "<< status << endl;// error send if not recognized

  if(status==0){
     status = Elveflow_Calibration_Default(_Calibration, 1000); //use default _calibration
  }
  // Analog output p arameters
  createParam(EFSetPressureString,    asynParamFloat64, &setPressure_);

  // Analog Input parameters
  createParam(EFReadFlowSting,        asynParamFloat64, &readSensor_);
  createParam(EFReadPressureString,   asynParamFloat64, &readPressure_);

  //read pressure for bumpless reboot
  double fVal;
  int channel =1;

  status = OB1_Get_Press(_MyOB1_ID, channel, 1, _Calibration, &fVal, 1000);
  //do smth with status: log, report
  cout << "in Constructor read pressure " <<fVal<< endl;

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
  printf("Destructor is called\n");
 }

asynStatus USBelveFlow::getBounds(asynUser *pasynUser, epicsFloat64 *low, epicsFloat64 *high)
{
  int function = pasynUser->reason;

  // Analog outputs are 16-bit devices
  if (function == setPressure_) {
  // function ==getSensor_, function == getPreassure do not have any size restriction
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

//  setIntegerParam(addr, function, value);
/*OKS
  // Analog output functions
  if (function == setPressure_) {
    status = OB1_Set_Press(_MyOB1_ID, 1, value, _Calibration, 1000);
    // Numbers needs to be chaged to constants
     // OKS test code
    double get_Sens_data;
    int channel =1;
    OB1_Get_Sens_Data(_MyOB1_ID, channel, 1, &get_Sens_data);//use pointer
    cout << "\n\nchannel 1" << channel << ": " << get_Sens_data << " ul/min" << endl;
    
  }
*/
  cout<<"In function writeInt32()\n";
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

asynStatus USBelveFlow::readInt32(asynUser *pasynUser, epicsInt32 *value){
  int addr;
  int function = pasynUser->reason;
  int status = 0;
 // int readVal;
  int channel = 1;
  static const char *functionName = "readInt32";

  this->getAddress(pasynUser, &addr);
  // Analog input function
  /* OKS flow is a double!!! not int32!!!
  if (function == readPressure_) { 
    status = OB1_Get_Press(_MyOB1_ID, channel, 1, _Calibration, &readVal, 1000);
    value = readVal;
    setIntegerParam(addr, readPressure_, *value);
  }

  else if(function == readSensor_){
    double readVal;
    status = OB1_Get_Sens_Data(_MyOB1_ID, channel, 1, &readVal);  //use pointer
    value = readVal;
    setIntegerParam(addr, readSensor_, *value);
    cout << "channel 1" << channel << ": " << readVal << endl;
  }
  */
  callParamCallbacks(addr);
  cout<<"\n!!!!!!!In readInt32\n";
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
    status = OB1_Set_Press(_MyOB1_ID, 1, value, _Calibration, 1000);
    // Numbers needs to be chaged to constants
     // OKS test code
    double get_Sens_data;
    int channel =1;
    OB1_Get_Sens_Data(_MyOB1_ID, channel, 1, &get_Sens_data);//use pointer
    cout << "\n\nchannel 1" << channel << ": " << get_Sens_data << " ul/min" << endl;
    
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

  cout<<"\n!!!!In writeFloat64\n";
  return (status == 0) ? asynSuccess : asynError;
}


asynStatus USBelveFlow::readFloat64(asynUser *pasynUser, epicsFloat64 *value){
  int addr;
  int function = pasynUser->reason;
  int status=0;
  int channel = 1;
  double fVal = -1;
  int filter=0;
  static const char *functionName = "readFloat64";

  this->getAddress(pasynUser, &addr);

  if (function == readPressure_) { 
    status = OB1_Get_Press(_MyOB1_ID, channel, 1, _Calibration, &fVal, 1000);
    *value = fVal;
    setDoubleParam(addr, readPressure_, *value);
   // cout<<"\n!!!!!!In readFloat64 readPressure_\n";
  }

  else if(function == readSensor_){
    status = OB1_Get_Sens_Data(_MyOB1_ID, channel, 1, &fVal);  //use pointer
    *value = fVal;
    setDoubleParam(addr, readSensor_, *value);
   // cout<<"\n!!!!!!In readFloat64 readSensor_\n";
  }
  callParamCallbacks(addr);
  return (status == 0) ? asynSuccess : asynError;

}

void USBelveFlow::setAllPressure(int p1){
  // Sets all preassure to val in mbars, usefull to bring all challel to 0. 
  // Caution as different channels can have different ranges
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
