/* dss_wrapper.c: DualShock 2 vibration, between the game and the DSR sequencer. */

#include "sh2.h"

/* Actuator levels to send, per controller port and actuator (0xFF = unset). */
static unsigned char _Peripheral_Controller_SendData[2][6] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

static unsigned int _sequencer_init;
static unsigned int _edit_flag;

/* Vibration strength from the options menu (off, low, mid, high). */
float DSS_Wrapper_AllVibrationRatio_Get(void) {
    float result;

    result = 0.0f;
    switch (playing.vibration) {
    case 1:
        result = 0.6f;
        break;
    case 2:
        result = 0.8f;
        break;
    case 3:
        result = 1.0f;
        break;
    case 0:
        break;
    }
    return result;
}

int DSS_Wrapper_ActuaterData_Send(unsigned int ControllerID, unsigned int ActuaterType, unsigned int ActuaterLV) {
    int result;

    _Peripheral_Controller_SendData[ControllerID][ActuaterType] = ActuaterLV;
    result = 1;
    return result;
}

int DSS_Wrapper_AllActuaterData_RealSend_to_Peripheral(void) {
    int result;
    int count;

    count = 0; /* Matching: reconstructed; the original's DWARF has count but none of its code survives */
    result = 1;
    return result;
}

void DSS_Wrapper_DualShock2_Sequencer(void) {
    if (_edit_flag == 0) {
        if (_sequencer_init == 0) {
            DSR_Sequencer_Initialize();
            DSR_Sequence_Different_Time_Set(1.0f / 60.0f);
            _sequencer_init = 1;
        }
        DSR_Sequencer();
    }
    DSS_Wrapper_AllActuaterData_RealSend_to_Peripheral();
}

unsigned int DSS_Wrapper_DualShock2_Send_ActuaterLV_Get(unsigned int ControllerID, unsigned int ActuaterType) {
    return _Peripheral_Controller_SendData[ControllerID][ActuaterType];
}

void DSS_Wrapper_DualShock2_Send_ActuaterLV_Claer(unsigned int ControllerID, unsigned int ActuaterType) {
    _Peripheral_Controller_SendData[ControllerID][ActuaterType] = 0;
}
