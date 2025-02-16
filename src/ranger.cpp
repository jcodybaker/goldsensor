#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <malloc.h>
#include <esp_log.h>

static void print_pal_error(const char *op, VL53L0X_Error Status){
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(Status, buf);
    ESP_LOGI("ranger", "API(%s) Status: %i : %s\n", op, Status, buf);
}

VL53L0X_Error ranger_measure(VL53L0X_Dev_t *pMyDevice, VL53L0X_RangingMeasurementData_t *measurement)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    Status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, measurement);
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_PerformSingleRangingMeasurement", Status);
        return Status;
    }
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Dev_t * ranger_init()  {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_Dev_t *pMyDevice = (VL53L0X_Dev_t *) malloc(sizeof(VL53L0X_Dev_t));
    VL53L0X_DeviceInfo_t                DeviceInfo;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    // Initialize Commsr
    pMyDevice->I2cDevAddr      = 0x29;
    pMyDevice->comms_type      =  1;
    pMyDevice->comms_speed_khz =  100;

    // VL53L0X_trace_config("", TRACE_MODULE_ALL, TRACE_LEVEL_ALL, TRACE_FUNCTION_ALL);

    // End of implementation specific
    Status = VL53L0X_DataInit(pMyDevice); // Data initialization
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_DataInit", Status);
        return NULL;
    }
    
    Status = VL53L0X_GetDeviceInfo(pMyDevice, &DeviceInfo);
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_GetDeviceInfo", Status);
        return NULL;
    }
    
    printf("VL53L0X_GetDeviceInfo:\n");
    printf("Device Name : %s\n", DeviceInfo.Name);
    printf("Device Type : %s\n", DeviceInfo.Type);
    printf("Device ID : %s\n", DeviceInfo.ProductId);
    printf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
    printf("ProductRevisionMinor : %d\n", DeviceInfo.ProductRevisionMinor);

    if ((DeviceInfo.ProductRevisionMinor != 1) && (DeviceInfo.ProductRevisionMinor != 1)) {
        printf("Error expected cut 1.1 but found cut %d.%d\n",
                DeviceInfo.ProductRevisionMajor, DeviceInfo.ProductRevisionMinor);
        Status = VL53L0X_ERROR_NOT_SUPPORTED;
        print_pal_error("VL53L0X_GetDeviceInfo", Status);
        return NULL;
    }
    
    Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_StaticInit", Status);
        return NULL;
    }
    
    Status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal); // Device Initialization
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_PerformRefCalibration", Status);
        return NULL;
    }

    // This seemed to cause problems and in the docs it was described as optional if no cover-glass was used.
    // 
    // Status = VL53L0X_PerformRefSpadManagement(pMyDevice,	&refSpadCount, &isApertureSpads); // Device Initialization
    // if(Status != VL53L0X_ERROR_NONE) {
    //     print_pal_error(Status);
    //     return NULL;
    // }
    // printf ("refSpadCount = %d, isApertureSpads = %d\n", refSpadCount, isApertureSpads);

    // no need to do this when we use VL53L0X_PerformSingleRangingMeasurement
    Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_SetDeviceMode", Status);
        return NULL;
    }

    // This seems to set the minimum required signal amount.
    Status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.5*65536));
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_SetLimitCheckValue", Status);
        return NULL;
    }
    
    // This seems to set the minimum range.
    Status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(18*65536));
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_SetLimitCheckValue", Status);
        return NULL;
    }			
    
    // 200ms (200000us) is high-accuracy mode, 30ms default, 33 long-range, and 20ms high-speed.
    Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 200000);
    if(Status != VL53L0X_ERROR_NONE) {
        print_pal_error("VL53L0X_SetMeasurementTimingBudgetMicroSeconds", Status);
        return NULL;
    }
    
    return pMyDevice;
}
