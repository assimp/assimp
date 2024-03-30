/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifdef SDL_JOYSTICK_HIDAPI

#include "SDL_events.h"
#include "SDL_timer.h"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"
#include "../../SDL_hints_c.h"
#include "../SDL_sysjoystick.h"
#include "SDL_hidapijoystick_c.h"
#include "SDL_hidapi_rumble.h"

#ifdef SDL_JOYSTICK_HIDAPI_PS5

/* Define this if you want to log all packets from the controller */
/*#define DEBUG_PS5_PROTOCOL*/

/* Define this if you want to log calibration data */
/*#define DEBUG_PS5_CALIBRATION*/

#define GYRO_RES_PER_DEGREE             1024.0f
#define ACCEL_RES_PER_G                 8192.0f
#define BLUETOOTH_DISCONNECT_TIMEOUT_MS 500

#define LOAD16(A, B)       (Sint16)((Uint16)(A) | (((Uint16)(B)) << 8))
#define LOAD32(A, B, C, D) ((((Uint32)(A)) << 0) |  \
                            (((Uint32)(B)) << 8) |  \
                            (((Uint32)(C)) << 16) | \
                            (((Uint32)(D)) << 24))

enum
{
    SDL_CONTROLLER_BUTTON_PS5_TOUCHPAD = SDL_CONTROLLER_BUTTON_MISC1 + 1,
    SDL_CONTROLLER_BUTTON_PS5_LEFT_FUNCTION,
    SDL_CONTROLLER_BUTTON_PS5_RIGHT_FUNCTION,
    SDL_CONTROLLER_BUTTON_PS5_LEFT_PADDLE,
    SDL_CONTROLLER_BUTTON_PS5_RIGHT_PADDLE
};

typedef enum
{
    k_EPS5ReportIdState = 0x01,
    k_EPS5ReportIdUsbEffects = 0x02,
    k_EPS5ReportIdBluetoothEffects = 0x31,
    k_EPS5ReportIdBluetoothState = 0x31,
} EPS5ReportId;

typedef enum
{
    k_EPS5FeatureReportIdCapabilities = 0x03,
    k_EPS5FeatureReportIdCalibration = 0x05,
    k_EPS5FeatureReportIdSerialNumber = 0x09,
    k_EPS5FeatureReportIdFirmwareInfo = 0x20,
} EPS5FeatureReportId;

typedef struct
{
    Uint8 ucLeftJoystickX;
    Uint8 ucLeftJoystickY;
    Uint8 ucRightJoystickX;
    Uint8 ucRightJoystickY;
    Uint8 rgucButtonsHatAndCounter[3];
    Uint8 ucTriggerLeft;
    Uint8 ucTriggerRight;
} PS5SimpleStatePacket_t;

typedef struct
{
    Uint8 ucLeftJoystickX;        /* 0 */
    Uint8 ucLeftJoystickY;        /* 1 */
    Uint8 ucRightJoystickX;       /* 2 */
    Uint8 ucRightJoystickY;       /* 3 */
    Uint8 ucTriggerLeft;          /* 4 */
    Uint8 ucTriggerRight;         /* 5 */
    Uint8 ucCounter;              /* 6 */
    Uint8 rgucButtonsAndHat[4];   /* 7 */
    Uint8 rgucPacketSequence[4];  /* 11 - 32 bit little endian */
    Uint8 rgucGyroX[2];           /* 15 */
    Uint8 rgucGyroY[2];           /* 17 */
    Uint8 rgucGyroZ[2];           /* 19 */
    Uint8 rgucAccelX[2];          /* 21 */
    Uint8 rgucAccelY[2];          /* 23 */
    Uint8 rgucAccelZ[2];          /* 25 */
    Uint8 rgucSensorTimestamp[4]; /* 27 - 32 bit little endian */

} PS5StatePacketCommon_t;

typedef struct
{
    Uint8 ucLeftJoystickX;        /* 0 */
    Uint8 ucLeftJoystickY;        /* 1 */
    Uint8 ucRightJoystickX;       /* 2 */
    Uint8 ucRightJoystickY;       /* 3 */
    Uint8 ucTriggerLeft;          /* 4 */
    Uint8 ucTriggerRight;         /* 5 */
    Uint8 ucCounter;              /* 6 */
    Uint8 rgucButtonsAndHat[4];   /* 7 */
    Uint8 rgucPacketSequence[4];  /* 11 - 32 bit little endian */
    Uint8 rgucGyroX[2];           /* 15 */
    Uint8 rgucGyroY[2];           /* 17 */
    Uint8 rgucGyroZ[2];           /* 19 */
    Uint8 rgucAccelX[2];          /* 21 */
    Uint8 rgucAccelY[2];          /* 23 */
    Uint8 rgucAccelZ[2];          /* 25 */
    Uint8 rgucSensorTimestamp[4]; /* 27 - 32 bit little endian */
    Uint8 ucSensorTemp;           /* 31 */
    Uint8 ucTouchpadCounter1;     /* 32 - high bit clear + counter */
    Uint8 rgucTouchpadData1[3];   /* 33 - X/Y, 12 bits per axis */
    Uint8 ucTouchpadCounter2;     /* 36 - high bit clear + counter */
    Uint8 rgucTouchpadData2[3];   /* 37 - X/Y, 12 bits per axis */
    Uint8 rgucUnknown1[8];        /* 40 */
    Uint8 rgucTimer2[4];          /* 48 - 32 bit little endian */
    Uint8 ucBatteryLevel;         /* 52 */
    Uint8 ucConnectState;         /* 53 - 0x08 = USB, 0x01 = headphone */

    /* There's more unknown data at the end, and a 32-bit CRC on Bluetooth */
} PS5StatePacket_t;

typedef struct
{
    Uint8 ucLeftJoystickX;        /* 0 */
    Uint8 ucLeftJoystickY;        /* 1 */
    Uint8 ucRightJoystickX;       /* 2 */
    Uint8 ucRightJoystickY;       /* 3 */
    Uint8 ucTriggerLeft;          /* 4 */
    Uint8 ucTriggerRight;         /* 5 */
    Uint8 ucCounter;              /* 6 */
    Uint8 rgucButtonsAndHat[4];   /* 7 */
    Uint8 rgucPacketSequence[4];  /* 11 - 32 bit little endian */
    Uint8 rgucGyroX[2];           /* 15 */
    Uint8 rgucGyroY[2];           /* 17 */
    Uint8 rgucGyroZ[2];           /* 19 */
    Uint8 rgucAccelX[2];          /* 21 */
    Uint8 rgucAccelY[2];          /* 23 */
    Uint8 rgucAccelZ[2];          /* 25 */
    Uint8 rgucSensorTimestamp[4]; /* 27 - 32 bit little endian */
    Uint8 ucTouchpadCounter1;     /* 31 - high bit clear + counter */
    Uint8 rgucTouchpadData1[3];   /* 32 - X/Y, 12 bits per axis */
    Uint8 ucTouchpadCounter2;     /* 35 - high bit clear + counter */
    Uint8 rgucTouchpadData2[3];   /* 36 - X/Y, 12 bits per axis */

    /* There's more unknown data at the end, and a 32-bit CRC on Bluetooth */
} PS5StatePacketAlt_t;

typedef struct
{
    Uint8 ucEnableBits1;              /* 0 */
    Uint8 ucEnableBits2;              /* 1 */
    Uint8 ucRumbleRight;              /* 2 */
    Uint8 ucRumbleLeft;               /* 3 */
    Uint8 ucHeadphoneVolume;          /* 4 */
    Uint8 ucSpeakerVolume;            /* 5 */
    Uint8 ucMicrophoneVolume;         /* 6 */
    Uint8 ucAudioEnableBits;          /* 7 */
    Uint8 ucMicLightMode;             /* 8 */
    Uint8 ucAudioMuteBits;            /* 9 */
    Uint8 rgucRightTriggerEffect[11]; /* 10 */
    Uint8 rgucLeftTriggerEffect[11];  /* 21 */
    Uint8 rgucUnknown1[6];            /* 32 */
    Uint8 ucEnableBits3;              /* 38 */
    Uint8 rgucUnknown2[2];            /* 39 */
    Uint8 ucLedAnim;                  /* 41 */
    Uint8 ucLedBrightness;            /* 42 */
    Uint8 ucPadLights;                /* 43 */
    Uint8 ucLedRed;                   /* 44 */
    Uint8 ucLedGreen;                 /* 45 */
    Uint8 ucLedBlue;                  /* 46 */
} DS5EffectsState_t;

typedef enum
{
    k_EDS5EffectRumbleStart = (1 << 0),
    k_EDS5EffectRumble = (1 << 1),
    k_EDS5EffectLEDReset = (1 << 2),
    k_EDS5EffectLED = (1 << 3),
    k_EDS5EffectPadLights = (1 << 4),
    k_EDS5EffectMicLight = (1 << 5)
} EDS5Effect;

typedef enum
{
    k_EDS5LEDResetStateNone,
    k_EDS5LEDResetStatePending,
    k_EDS5LEDResetStateComplete,
} EDS5LEDResetState;

typedef struct
{
    Sint16 bias;
    float sensitivity;
} IMUCalibrationData;

typedef struct
{
    SDL_HIDAPI_Device *device;
    SDL_Joystick *joystick;
    SDL_bool use_alternate_report;
    SDL_bool sensors_supported;
    SDL_bool lightbar_supported;
    SDL_bool vibration_supported;
    SDL_bool playerled_supported;
    SDL_bool touchpad_supported;
    SDL_bool effects_supported;
    SDL_bool enhanced_mode;
    SDL_bool report_sensors;
    SDL_bool report_touchpad;
    SDL_bool hardware_calibration;
    IMUCalibrationData calibration[6];
    Uint16 firmware_version;
    Uint32 last_packet;
    int player_index;
    SDL_bool player_lights;
    Uint8 rumble_left;
    Uint8 rumble_right;
    SDL_bool color_set;
    Uint8 led_red;
    Uint8 led_green;
    Uint8 led_blue;
    EDS5LEDResetState led_reset_state;
    Uint32 last_timestamp;
    Uint64 timestamp;
    union
    {
        PS5SimpleStatePacket_t simple;
        PS5StatePacketCommon_t state;
        PS5StatePacket_t full_state;
        Uint8 data[64];
    } last_state;
} SDL_DriverPS5_Context;

static int HIDAPI_DriverPS5_SendJoystickEffect(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, const void *effect, int size);

static void HIDAPI_DriverPS5_RegisterHints(SDL_HintCallback callback, void *userdata)
{
    SDL_AddHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5, callback, userdata);
}

static void HIDAPI_DriverPS5_UnregisterHints(SDL_HintCallback callback, void *userdata)
{
    SDL_DelHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5, callback, userdata);
}

static SDL_bool HIDAPI_DriverPS5_IsEnabled(void)
{
    return SDL_GetHintBoolean(SDL_HINT_JOYSTICK_HIDAPI_PS5, SDL_GetHintBoolean(SDL_HINT_JOYSTICK_HIDAPI, SDL_HIDAPI_DEFAULT));
}

static int ReadFeatureReport(SDL_hid_device *dev, Uint8 report_id, Uint8 *report, size_t length)
{
    SDL_memset(report, 0, length);
    report[0] = report_id;
    return SDL_hid_get_feature_report(dev, report, length);
}

static SDL_bool HIDAPI_DriverPS5_IsSupportedDevice(SDL_HIDAPI_Device *device, const char *name, SDL_GameControllerType type, Uint16 vendor_id, Uint16 product_id, Uint16 version, int interface_number, int interface_class, int interface_subclass, int interface_protocol)
{
    Uint8 data[USB_PACKET_LENGTH];
    int size;

    if (type == SDL_CONTROLLER_TYPE_PS5) {
        return SDL_TRUE;
    }

    if (HIDAPI_SupportsPlaystationDetection(vendor_id, product_id)) {
        if (device && device->dev) {
            size = ReadFeatureReport(device->dev, k_EPS5FeatureReportIdCapabilities, data, sizeof(data));
            if (size == 48 && data[2] == 0x28) {
                /* Supported third party controller */
                return SDL_TRUE;
            } else {
                return SDL_FALSE;
            }
        } else {
            /* Might be supported by this driver, enumerate and find out */
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}

static void SetLedsForPlayerIndex(DS5EffectsState_t *effects, int player_index)
{
    /* This list is the same as what hid-sony.c uses in the Linux kernel.
       The first 4 values correspond to what the PS4 assigns.
    */
    static const Uint8 colors[7][3] = {
        { 0x00, 0x00, 0x40 }, /* Blue */
        { 0x40, 0x00, 0x00 }, /* Red */
        { 0x00, 0x40, 0x00 }, /* Green */
        { 0x20, 0x00, 0x20 }, /* Pink */
        { 0x20, 0x10, 0x00 }, /* Orange */
        { 0x00, 0x10, 0x10 }, /* Teal */
        { 0x10, 0x10, 0x10 }  /* White */
    };

    if (player_index >= 0) {
        player_index %= SDL_arraysize(colors);
    } else {
        player_index = 0;
    }

    effects->ucLedRed = colors[player_index][0];
    effects->ucLedGreen = colors[player_index][1];
    effects->ucLedBlue = colors[player_index][2];
}

static void SetLightsForPlayerIndex(DS5EffectsState_t *effects, int player_index)
{
    static const Uint8 lights[] = {
        0x04,
        0x0A,
        0x15,
        0x1B
    };

    if (player_index >= 0) {
        /* Bitmask, 0x1F enables all lights, 0x20 changes instantly instead of fade */
        player_index %= SDL_arraysize(lights);
        effects->ucPadLights = lights[player_index] | 0x20;
    } else {
        effects->ucPadLights = 0x00;
    }
}

static SDL_bool HIDAPI_DriverPS5_InitDevice(SDL_HIDAPI_Device *device)
{
    SDL_DriverPS5_Context *ctx;
    Uint8 data[USB_PACKET_LENGTH * 2];
    int size;
    char serial[18];
    SDL_JoystickType joystick_type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;

    ctx = (SDL_DriverPS5_Context *)SDL_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        SDL_OutOfMemory();
        return SDL_FALSE;
    }
    ctx->device = device;

    device->context = ctx;

    if (device->serial && SDL_strlen(device->serial) == 12) {
        int i, j;

        j = -1;
        for (i = 0; i < 12; i += 2) {
            j += 1;
            SDL_memcpy(&serial[j], &device->serial[i], 2);
            j += 2;
            serial[j] = '-';
        }
        serial[j] = '\0';
    } else {
        serial[0] = '\0';
    }

    /* Read a report to see what mode we're in */
    size = SDL_hid_read_timeout(device->dev, data, sizeof(data), 16);
#ifdef DEBUG_PS5_PROTOCOL
    if (size > 0) {
        HIDAPI_DumpPacket("PS5 first packet: size = %d", data, size);
    } else {
        SDL_Log("PS5 first packet: size = %d\n", size);
    }
#endif
    if (size == 64) {
        /* Connected over USB */
        device->is_bluetooth = SDL_FALSE;
        ctx->enhanced_mode = SDL_TRUE;
    } else if (size > 0 && data[0] == k_EPS5ReportIdBluetoothEffects) {
        /* Connected over Bluetooth, using enhanced reports */
        device->is_bluetooth = SDL_TRUE;
        ctx->enhanced_mode = SDL_TRUE;
    } else {
        /* Connected over Bluetooth, using simple reports (DirectInput enabled) */
        device->is_bluetooth = SDL_TRUE;

        /* Games written prior the introduction of PS5 controller support in SDL will not be aware of
           SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, but they did know SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE.
           To support apps that only knew about the PS4 hint, we'll use the PS4 hint as the default.
        */
        ctx->enhanced_mode = SDL_GetHintBoolean(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE,
                                                SDL_GetHintBoolean(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, SDL_FALSE));
    }

    if (ctx->enhanced_mode) {
        /* Read the serial number (Bluetooth address in reverse byte order)
           This will also enable enhanced reports over Bluetooth
        */
        if (ReadFeatureReport(device->dev, k_EPS5FeatureReportIdSerialNumber, data, sizeof(data)) >= 7) {
            (void)SDL_snprintf(serial, sizeof(serial), "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x",
                               data[6], data[5], data[4], data[3], data[2], data[1]);
        }

        /* Read the firmware version
           This will also enable enhanced reports over Bluetooth
        */
        if (ReadFeatureReport(device->dev, k_EPS5FeatureReportIdFirmwareInfo, data, USB_PACKET_LENGTH) >= 46) {
            ctx->firmware_version = (Uint16)data[44] | ((Uint16)data[45] << 8);
        }
    }

    size = ReadFeatureReport(device->dev, k_EPS5FeatureReportIdCapabilities, data, sizeof(data));
    /* Get the device capabilities */
    if (device->vendor_id == USB_VENDOR_SONY) {
        ctx->sensors_supported = SDL_TRUE;
        ctx->lightbar_supported = SDL_TRUE;
        ctx->vibration_supported = SDL_TRUE;
        ctx->playerled_supported = SDL_TRUE;
        ctx->touchpad_supported = SDL_TRUE;
    } else if (size == 48 && data[2] == 0x28) {
        Uint8 capabilities = data[4];
        Uint8 capabilities2 = data[20];
        Uint8 device_type = data[5];

#ifdef DEBUG_PS5_PROTOCOL
        HIDAPI_DumpPacket("PS5 capabilities: size = %d", data, size);
#endif
        if (capabilities & 0x02) {
            ctx->sensors_supported = SDL_TRUE;
        }
        if (capabilities & 0x04) {
            ctx->lightbar_supported = SDL_TRUE;
        }
        if (capabilities & 0x08) {
            ctx->vibration_supported = SDL_TRUE;
        }
        if (capabilities & 0x40) {
            ctx->touchpad_supported = SDL_TRUE;
        }
        if (capabilities2 & 0x80) {
            ctx->playerled_supported = SDL_TRUE;
        }

        switch (device_type) {
        case 0x00:
            joystick_type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
            break;
        case 0x01:
            joystick_type = SDL_JOYSTICK_TYPE_GUITAR;
            break;
        case 0x02:
            joystick_type = SDL_JOYSTICK_TYPE_DRUM_KIT;
            break;
        case 0x06:
            joystick_type = SDL_JOYSTICK_TYPE_WHEEL;
            break;
        case 0x07:
            joystick_type = SDL_JOYSTICK_TYPE_ARCADE_STICK;
            break;
        case 0x08:
            joystick_type = SDL_JOYSTICK_TYPE_FLIGHT_STICK;
            break;
        default:
            joystick_type = SDL_JOYSTICK_TYPE_UNKNOWN;
            break;
        }

        ctx->use_alternate_report = SDL_TRUE;
    } else if (device->vendor_id == USB_VENDOR_RAZER &&
               (device->product_id == USB_PRODUCT_RAZER_WOLVERINE_V2_PRO_PS5_WIRED ||
                device->product_id == USB_PRODUCT_RAZER_WOLVERINE_V2_PRO_PS5_WIRELESS)) {
        /* The Razer Wolverine V2 Pro doesn't respond to the detection protocol, but has a touchpad and sensors, but no vibration */
        ctx->sensors_supported = SDL_TRUE;
        ctx->touchpad_supported = SDL_TRUE;
    }
    ctx->effects_supported = (ctx->lightbar_supported || ctx->vibration_supported || ctx->playerled_supported);

    device->joystick_type = joystick_type;
    device->type = SDL_CONTROLLER_TYPE_PS5;
    if (device->vendor_id == USB_VENDOR_SONY) {
        if (SDL_IsJoystickDualSenseEdge(device->vendor_id, device->product_id)) {
            HIDAPI_SetDeviceName(device, "DualSense Edge Wireless Controller");
        } else {
            HIDAPI_SetDeviceName(device, "DualSense Wireless Controller");
        }
    }
    HIDAPI_SetDeviceSerial(device, serial);

    /* Prefer the USB device over the Bluetooth device */
    if (device->is_bluetooth) {
        if (HIDAPI_HasConnectedUSBDevice(device->serial)) {
            return SDL_TRUE;
        }
    } else {
        HIDAPI_DisconnectBluetoothDevice(device->serial);
    }
    return HIDAPI_JoystickConnected(device, NULL);
}

static int HIDAPI_DriverPS5_GetDevicePlayerIndex(SDL_HIDAPI_Device *device, SDL_JoystickID instance_id)
{
    return -1;
}

static void HIDAPI_DriverPS5_LoadCalibrationData(SDL_HIDAPI_Device *device)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    int i, size;
    Uint8 data[USB_PACKET_LENGTH];

    size = ReadFeatureReport(device->dev, k_EPS5FeatureReportIdCalibration, data, sizeof(data));
    if (size < 35) {
#ifdef DEBUG_PS5_CALIBRATION
        SDL_Log("Short read of calibration data: %d, ignoring calibration\n", size);
#endif
        return;
    }

    {
        Sint16 sGyroPitchBias, sGyroYawBias, sGyroRollBias;
        Sint16 sGyroPitchPlus, sGyroPitchMinus;
        Sint16 sGyroYawPlus, sGyroYawMinus;
        Sint16 sGyroRollPlus, sGyroRollMinus;
        Sint16 sGyroSpeedPlus, sGyroSpeedMinus;

        Sint16 sAccXPlus, sAccXMinus;
        Sint16 sAccYPlus, sAccYMinus;
        Sint16 sAccZPlus, sAccZMinus;

        float flNumerator;
        Sint16 sRange2g;

#ifdef DEBUG_PS5_CALIBRATION
        HIDAPI_DumpPacket("PS5 calibration packet: size = %d", data, size);
#endif

        sGyroPitchBias = LOAD16(data[1], data[2]);
        sGyroYawBias = LOAD16(data[3], data[4]);
        sGyroRollBias = LOAD16(data[5], data[6]);

        sGyroPitchPlus = LOAD16(data[7], data[8]);
        sGyroPitchMinus = LOAD16(data[9], data[10]);
        sGyroYawPlus = LOAD16(data[11], data[12]);
        sGyroYawMinus = LOAD16(data[13], data[14]);
        sGyroRollPlus = LOAD16(data[15], data[16]);
        sGyroRollMinus = LOAD16(data[17], data[18]);

        sGyroSpeedPlus = LOAD16(data[19], data[20]);
        sGyroSpeedMinus = LOAD16(data[21], data[22]);

        sAccXPlus = LOAD16(data[23], data[24]);
        sAccXMinus = LOAD16(data[25], data[26]);
        sAccYPlus = LOAD16(data[27], data[28]);
        sAccYMinus = LOAD16(data[29], data[30]);
        sAccZPlus = LOAD16(data[31], data[32]);
        sAccZMinus = LOAD16(data[33], data[34]);

        flNumerator = (sGyroSpeedPlus + sGyroSpeedMinus) * GYRO_RES_PER_DEGREE;
        ctx->calibration[0].bias = sGyroPitchBias;
        ctx->calibration[0].sensitivity = flNumerator / (sGyroPitchPlus - sGyroPitchMinus);

        ctx->calibration[1].bias = sGyroYawBias;
        ctx->calibration[1].sensitivity = flNumerator / (sGyroYawPlus - sGyroYawMinus);

        ctx->calibration[2].bias = sGyroRollBias;
        ctx->calibration[2].sensitivity = flNumerator / (sGyroRollPlus - sGyroRollMinus);

        sRange2g = sAccXPlus - sAccXMinus;
        ctx->calibration[3].bias = sAccXPlus - sRange2g / 2;
        ctx->calibration[3].sensitivity = 2.0f * ACCEL_RES_PER_G / (float)sRange2g;

        sRange2g = sAccYPlus - sAccYMinus;
        ctx->calibration[4].bias = sAccYPlus - sRange2g / 2;
        ctx->calibration[4].sensitivity = 2.0f * ACCEL_RES_PER_G / (float)sRange2g;

        sRange2g = sAccZPlus - sAccZMinus;
        ctx->calibration[5].bias = sAccZPlus - sRange2g / 2;
        ctx->calibration[5].sensitivity = 2.0f * ACCEL_RES_PER_G / (float)sRange2g;

        ctx->hardware_calibration = SDL_TRUE;
        for (i = 0; i < 6; ++i) {
            float divisor = (i < 3 ? 64.0f : 1.0f);
#ifdef DEBUG_PS5_CALIBRATION
            SDL_Log("calibration[%d] bias = %d, sensitivity = %f\n", i, ctx->calibration[i].bias, ctx->calibration[i].sensitivity);
#endif
            /* Some controllers have a bad calibration */
            if ((SDL_abs(ctx->calibration[i].bias) > 1024) || (SDL_fabs(1.0f - ctx->calibration[i].sensitivity / divisor) > 0.5f)) {
#ifdef DEBUG_PS5_CALIBRATION
                SDL_Log("invalid calibration, ignoring\n");
#endif
                ctx->hardware_calibration = SDL_FALSE;
            }
        }
    }
}

static float HIDAPI_DriverPS5_ApplyCalibrationData(SDL_DriverPS5_Context *ctx, int index, Sint16 value)
{
    float result;

    if (ctx->hardware_calibration) {
        IMUCalibrationData *calibration = &ctx->calibration[index];

        result = (value - calibration->bias) * calibration->sensitivity;
    } else if (index < 3) {
        result = value * 64.f;
    } else {
        result = value;
    }

    /* Convert the raw data to the units expected by SDL */
    if (index < 3) {
        result = (result / GYRO_RES_PER_DEGREE) * (float)M_PI / 180.0f;
    } else {
        result = (result / ACCEL_RES_PER_G) * SDL_STANDARD_GRAVITY;
    }
    return result;
}

static int HIDAPI_DriverPS5_UpdateEffects(SDL_HIDAPI_Device *device, int effect_mask)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    DS5EffectsState_t effects;

    if (!ctx->enhanced_mode || !ctx->effects_supported) {
        return SDL_Unsupported();
    }

    SDL_zero(effects);

    /* Make sure the Bluetooth connection sequence has completed before sending LED color change */
    if (device->is_bluetooth &&
        (effect_mask & (k_EDS5EffectLED | k_EDS5EffectPadLights)) != 0) {
        if (ctx->led_reset_state != k_EDS5LEDResetStateComplete) {
            ctx->led_reset_state = k_EDS5LEDResetStatePending;
            return 0;
        }
    }

    if (ctx->vibration_supported) {
        if (ctx->rumble_left || ctx->rumble_right) {
            if (ctx->firmware_version < 0x0224) {
                effects.ucEnableBits1 |= 0x01; /* Enable rumble emulation */

                /* Shift to reduce effective rumble strength to match Xbox controllers */
                effects.ucRumbleLeft = ctx->rumble_left >> 1;
                effects.ucRumbleRight = ctx->rumble_right >> 1;
            } else {
                effects.ucEnableBits3 |= 0x04; /* Enable improved rumble emulation on 2.24 firmware and newer */

                effects.ucRumbleLeft = ctx->rumble_left;
                effects.ucRumbleRight = ctx->rumble_right;
            }
            effects.ucEnableBits1 |= 0x02; /* Disable audio haptics */
        } else {
            /* Leaving emulated rumble bits off will restore audio haptics */
        }

        if ((effect_mask & k_EDS5EffectRumbleStart) != 0) {
            effects.ucEnableBits1 |= 0x02; /* Disable audio haptics */
        }
        if ((effect_mask & k_EDS5EffectRumble) != 0) {
            /* Already handled above */
        }
    }
    if (ctx->lightbar_supported) {
        if ((effect_mask & k_EDS5EffectLEDReset) != 0) {
            effects.ucEnableBits2 |= 0x08; /* Reset LED state */
        }
        if ((effect_mask & k_EDS5EffectLED) != 0) {
            effects.ucEnableBits2 |= 0x04; /* Enable LED color */

            /* Populate the LED state with the appropriate color from our lookup table */
            if (ctx->color_set) {
                effects.ucLedRed = ctx->led_red;
                effects.ucLedGreen = ctx->led_green;
                effects.ucLedBlue = ctx->led_blue;
            } else {
                SetLedsForPlayerIndex(&effects, ctx->player_index);
            }
        }
    }
    if (ctx->playerled_supported) {
        if ((effect_mask & k_EDS5EffectPadLights) != 0) {
            effects.ucEnableBits2 |= 0x10; /* Enable touchpad lights */

            if (ctx->player_lights) {
                SetLightsForPlayerIndex(&effects, ctx->player_index);
            } else {
                effects.ucPadLights = 0x00;
            }
        }
    }
    if ((effect_mask & k_EDS5EffectMicLight) != 0) {
        effects.ucEnableBits2 |= 0x01; /* Enable microphone light */

        effects.ucMicLightMode = 0; /* Bitmask, 0x00 = off, 0x01 = solid, 0x02 = pulse */
    }

    return HIDAPI_DriverPS5_SendJoystickEffect(device, ctx->joystick, &effects, sizeof(effects));
}

static void HIDAPI_DriverPS5_CheckPendingLEDReset(SDL_HIDAPI_Device *device)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    SDL_bool led_reset_complete = SDL_FALSE;

    if (ctx->sensors_supported) {
        const PS5StatePacketCommon_t *packet = &ctx->last_state.state;

        /* Check the timer to make sure the Bluetooth connection LED animation is complete */
        const Uint32 connection_complete = 10200000;
        Uint32 timestamp = LOAD32(packet->rgucSensorTimestamp[0],
                                  packet->rgucSensorTimestamp[1],
                                  packet->rgucSensorTimestamp[2],
                                  packet->rgucSensorTimestamp[3]);
        if (SDL_TICKS_PASSED(timestamp, connection_complete)) {
            led_reset_complete = SDL_TRUE;
        }
    } else {
        /* We don't know how to check the timer, just assume it's complete for now */
        led_reset_complete = SDL_TRUE;
    }

    if (led_reset_complete) {
        HIDAPI_DriverPS5_UpdateEffects(device, k_EDS5EffectLEDReset);

        ctx->led_reset_state = k_EDS5LEDResetStateComplete;

        HIDAPI_DriverPS5_UpdateEffects(device, (k_EDS5EffectLED | k_EDS5EffectPadLights));
    }
}

static void HIDAPI_DriverPS5_TickleBluetooth(SDL_HIDAPI_Device *device)
{
    /* This is just a dummy packet that should have no effect, since we don't set the CRC */
    Uint8 data[78];

    SDL_zeroa(data);

    data[0] = k_EPS5ReportIdBluetoothEffects;
    data[1] = 0x02; /* Magic value */

    if (SDL_HIDAPI_LockRumble() == 0) {
        SDL_HIDAPI_SendRumbleAndUnlock(device, data, sizeof(data));
    }
}

static void HIDAPI_DriverPS5_SetEnhancedMode(SDL_HIDAPI_Device *device, SDL_Joystick *joystick)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    if (!ctx->enhanced_mode) {
        ctx->enhanced_mode = SDL_TRUE;

        if (ctx->touchpad_supported) {
            SDL_PrivateJoystickAddTouchpad(joystick, 2);
            ctx->report_touchpad = SDL_TRUE;
        }
        if (ctx->sensors_supported) {
            if (device->is_bluetooth) {
                /* Bluetooth sensor update rate appears to be 1000 Hz */
                SDL_PrivateJoystickAddSensor(joystick, SDL_SENSOR_GYRO, 1000.0f);
                SDL_PrivateJoystickAddSensor(joystick, SDL_SENSOR_ACCEL, 1000.0f);
            } else {
                SDL_PrivateJoystickAddSensor(joystick, SDL_SENSOR_GYRO, 250.0f);
                SDL_PrivateJoystickAddSensor(joystick, SDL_SENSOR_ACCEL, 250.0f);
            }
        }

        /* Switch into enhanced report mode */
        HIDAPI_DriverPS5_UpdateEffects(device, 0);

        /* Update the light effects */
        HIDAPI_DriverPS5_UpdateEffects(device, (k_EDS5EffectLED | k_EDS5EffectPadLights));
    }
}

static void SDLCALL SDL_PS5RumbleHintChanged(void *userdata, const char *name, const char *oldValue, const char *hint)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)userdata;

    /* This is a one-way trip, you can't switch the controller back to simple report mode */
    if (SDL_GetStringBoolean(hint, SDL_FALSE)) {
        HIDAPI_DriverPS5_SetEnhancedMode(ctx->device, ctx->joystick);
    }
}

static void SDLCALL SDL_PS5PlayerLEDHintChanged(void *userdata, const char *name, const char *oldValue, const char *hint)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)userdata;
    SDL_bool player_lights = SDL_GetStringBoolean(hint, SDL_TRUE);

    if (player_lights != ctx->player_lights) {
        ctx->player_lights = player_lights;

        HIDAPI_DriverPS5_UpdateEffects(ctx->device, k_EDS5EffectPadLights);
    }
}

static void HIDAPI_DriverPS5_SetDevicePlayerIndex(SDL_HIDAPI_Device *device, SDL_JoystickID instance_id, int player_index)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    if (!ctx->joystick) {
        return;
    }

    ctx->player_index = player_index;

    /* This will set the new LED state based on the new player index */
    HIDAPI_DriverPS5_UpdateEffects(device, (k_EDS5EffectLED | k_EDS5EffectPadLights));
}

static SDL_bool HIDAPI_DriverPS5_OpenJoystick(SDL_HIDAPI_Device *device, SDL_Joystick *joystick)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    SDL_AssertJoysticksLocked();

    ctx->joystick = joystick;
    ctx->last_packet = SDL_GetTicks();
    ctx->report_sensors = SDL_FALSE;
    ctx->report_touchpad = SDL_FALSE;
    ctx->rumble_left = 0;
    ctx->rumble_right = 0;
    ctx->color_set = SDL_FALSE;
    ctx->led_reset_state = k_EDS5LEDResetStateNone;
    SDL_zero(ctx->last_state);

    /* Initialize player index (needed for setting LEDs) */
    ctx->player_index = SDL_JoystickGetPlayerIndex(joystick);
    ctx->player_lights = SDL_GetHintBoolean(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, SDL_TRUE);

    /* Initialize the joystick capabilities */
    if (SDL_IsJoystickDualSenseEdge(device->vendor_id, device->product_id)) {
        joystick->nbuttons = 21;
    } else if (ctx->touchpad_supported) {
        joystick->nbuttons = 17;
    } else {
        joystick->nbuttons = 15;
    }
    joystick->naxes = SDL_CONTROLLER_AXIS_MAX;
    joystick->epowerlevel = device->is_bluetooth ? SDL_JOYSTICK_POWER_UNKNOWN : SDL_JOYSTICK_POWER_WIRED;
    joystick->firmware_version = ctx->firmware_version;

    if (ctx->enhanced_mode) {
        /* Force initialization when opening the joystick */
        ctx->enhanced_mode = SDL_FALSE;
        HIDAPI_DriverPS5_SetEnhancedMode(device, joystick);
    } else {
        SDL_AddHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE,
                            SDL_PS5RumbleHintChanged, ctx);
    }
    SDL_AddHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED,
                        SDL_PS5PlayerLEDHintChanged, ctx);

    return SDL_TRUE;
}

static int HIDAPI_DriverPS5_RumbleJoystick(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    if (!ctx->vibration_supported) {
        return SDL_Unsupported();
    }

    if (!ctx->rumble_left && !ctx->rumble_right) {
        HIDAPI_DriverPS5_UpdateEffects(device, k_EDS5EffectRumbleStart);
    }

    ctx->rumble_left = (low_frequency_rumble >> 8);
    ctx->rumble_right = (high_frequency_rumble >> 8);

    return HIDAPI_DriverPS5_UpdateEffects(device, k_EDS5EffectRumble);
}

static int HIDAPI_DriverPS5_RumbleJoystickTriggers(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static Uint32 HIDAPI_DriverPS5_GetJoystickCapabilities(SDL_HIDAPI_Device *device, SDL_Joystick *joystick)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    Uint32 result = 0;

    if (ctx->enhanced_mode) {
        if (ctx->lightbar_supported) {
            result |= SDL_JOYCAP_LED;
        }
        if (ctx->vibration_supported) {
            result |= SDL_JOYCAP_RUMBLE;
        }
    }

    return result;
}

static int HIDAPI_DriverPS5_SetJoystickLED(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    if (!ctx->lightbar_supported) {
        return SDL_Unsupported();
    }

    ctx->color_set = SDL_TRUE;
    ctx->led_red = red;
    ctx->led_green = green;
    ctx->led_blue = blue;

    return HIDAPI_DriverPS5_UpdateEffects(device, k_EDS5EffectLED);
}

static int HIDAPI_DriverPS5_SendJoystickEffect(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, const void *effect, int size)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    Uint8 data[78];
    int report_size, offset;
    Uint8 *pending_data;
    int *pending_size;
    int maximum_size;

    if (!ctx->effects_supported) {
        return SDL_Unsupported();
    }

    if (!ctx->enhanced_mode) {
        HIDAPI_DriverPS5_SetEnhancedMode(device, joystick);
    }

    SDL_zeroa(data);

    if (device->is_bluetooth) {
        data[0] = k_EPS5ReportIdBluetoothEffects;
        data[1] = 0x02; /* Magic value */

        report_size = 78;
        offset = 2;
    } else {
        data[0] = k_EPS5ReportIdUsbEffects;

        report_size = 48;
        offset = 1;
    }

    SDL_memcpy(&data[offset], effect, SDL_min((sizeof(data) - offset), (size_t)size));

    if (device->is_bluetooth) {
        /* Bluetooth reports need a CRC at the end of the packet (at least on Linux) */
        Uint8 ubHdr = 0xA2; /* hidp header is part of the CRC calculation */
        Uint32 unCRC;
        unCRC = SDL_crc32(0, &ubHdr, 1);
        unCRC = SDL_crc32(unCRC, data, (size_t)(report_size - sizeof(unCRC)));
        SDL_memcpy(&data[report_size - sizeof(unCRC)], &unCRC, sizeof(unCRC));
    }

    if (SDL_HIDAPI_LockRumble() != 0) {
        return -1;
    }

    /* See if we can update an existing pending request */
    if (SDL_HIDAPI_GetPendingRumbleLocked(device, &pending_data, &pending_size, &maximum_size)) {
        DS5EffectsState_t *effects = (DS5EffectsState_t *)&data[offset];
        DS5EffectsState_t *pending_effects = (DS5EffectsState_t *)&pending_data[offset];
        if (report_size == *pending_size &&
            effects->ucEnableBits1 == pending_effects->ucEnableBits1 &&
            effects->ucEnableBits2 == pending_effects->ucEnableBits2) {
            /* We're simply updating the data for this request */
            SDL_memcpy(pending_data, data, report_size);
            SDL_HIDAPI_UnlockRumble();
            return 0;
        }
    }

    if (SDL_HIDAPI_SendRumbleAndUnlock(device, data, report_size) != report_size) {
        return -1;
    }

    return 0;
}

static int HIDAPI_DriverPS5_SetJoystickSensorsEnabled(SDL_HIDAPI_Device *device, SDL_Joystick *joystick, SDL_bool enabled)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    if (!ctx->enhanced_mode) {
        return SDL_Unsupported();
    }

    if (enabled) {
        HIDAPI_DriverPS5_LoadCalibrationData(device);
    }
    ctx->report_sensors = enabled;
    ctx->timestamp = 0;

    return 0;
}

static void HIDAPI_DriverPS5_HandleSimpleStatePacket(SDL_Joystick *joystick, SDL_hid_device *dev, SDL_DriverPS5_Context *ctx, PS5SimpleStatePacket_t *packet)
{
    Sint16 axis;

    if (ctx->last_state.simple.rgucButtonsHatAndCounter[0] != packet->rgucButtonsHatAndCounter[0]) {
        {
            Uint8 data = (packet->rgucButtonsHatAndCounter[0] >> 4);

            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_X, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_A, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_B, (data & 0x04) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_Y, (data & 0x08) ? SDL_PRESSED : SDL_RELEASED);
        }
        {
            Uint8 data = (packet->rgucButtonsHatAndCounter[0] & 0x0F);
            SDL_bool dpad_up = SDL_FALSE;
            SDL_bool dpad_down = SDL_FALSE;
            SDL_bool dpad_left = SDL_FALSE;
            SDL_bool dpad_right = SDL_FALSE;

            switch (data) {
            case 0:
                dpad_up = SDL_TRUE;
                break;
            case 1:
                dpad_up = SDL_TRUE;
                dpad_right = SDL_TRUE;
                break;
            case 2:
                dpad_right = SDL_TRUE;
                break;
            case 3:
                dpad_right = SDL_TRUE;
                dpad_down = SDL_TRUE;
                break;
            case 4:
                dpad_down = SDL_TRUE;
                break;
            case 5:
                dpad_left = SDL_TRUE;
                dpad_down = SDL_TRUE;
                break;
            case 6:
                dpad_left = SDL_TRUE;
                break;
            case 7:
                dpad_up = SDL_TRUE;
                dpad_left = SDL_TRUE;
                break;
            default:
                break;
            }
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_DOWN, dpad_down);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_UP, dpad_up);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, dpad_right);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_LEFT, dpad_left);
        }
    }

    if (ctx->last_state.simple.rgucButtonsHatAndCounter[1] != packet->rgucButtonsHatAndCounter[1]) {
        Uint8 data = packet->rgucButtonsHatAndCounter[1];

        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_BACK, (data & 0x10) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_START, (data & 0x20) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_LEFTSTICK, (data & 0x40) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_RIGHTSTICK, (data & 0x80) ? SDL_PRESSED : SDL_RELEASED);
    }

    if (ctx->last_state.simple.rgucButtonsHatAndCounter[2] != packet->rgucButtonsHatAndCounter[2]) {
        Uint8 data = (packet->rgucButtonsHatAndCounter[2] & 0x03);

        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_GUIDE, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_MISC1, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
    }

    if (packet->ucTriggerLeft == 0 && (packet->rgucButtonsHatAndCounter[1] & 0x04)) {
        axis = SDL_JOYSTICK_AXIS_MAX;
    } else {
        axis = ((int)packet->ucTriggerLeft * 257) - 32768;
    }
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, axis);
    if (packet->ucTriggerRight == 0 && (packet->rgucButtonsHatAndCounter[1] & 0x08)) {
        axis = SDL_JOYSTICK_AXIS_MAX;
    } else {
        axis = ((int)packet->ucTriggerRight * 257) - 32768;
    }
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, axis);
    axis = ((int)packet->ucLeftJoystickX * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, axis);
    axis = ((int)packet->ucLeftJoystickY * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_LEFTY, axis);
    axis = ((int)packet->ucRightJoystickX * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTX, axis);
    axis = ((int)packet->ucRightJoystickY * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTY, axis);

    SDL_memcpy(&ctx->last_state.simple, packet, sizeof(ctx->last_state.simple));
}

static void HIDAPI_DriverPS5_HandleStatePacketCommon(SDL_Joystick *joystick, SDL_hid_device *dev, SDL_DriverPS5_Context *ctx, PS5StatePacketCommon_t *packet)
{
    Sint16 axis;

    if (ctx->last_state.state.rgucButtonsAndHat[0] != packet->rgucButtonsAndHat[0]) {
        {
            Uint8 data = (packet->rgucButtonsAndHat[0] >> 4);

            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_X, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_A, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_B, (data & 0x04) ? SDL_PRESSED : SDL_RELEASED);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_Y, (data & 0x08) ? SDL_PRESSED : SDL_RELEASED);
        }
        {
            Uint8 data = (packet->rgucButtonsAndHat[0] & 0x0F);
            SDL_bool dpad_up = SDL_FALSE;
            SDL_bool dpad_down = SDL_FALSE;
            SDL_bool dpad_left = SDL_FALSE;
            SDL_bool dpad_right = SDL_FALSE;

            switch (data) {
            case 0:
                dpad_up = SDL_TRUE;
                break;
            case 1:
                dpad_up = SDL_TRUE;
                dpad_right = SDL_TRUE;
                break;
            case 2:
                dpad_right = SDL_TRUE;
                break;
            case 3:
                dpad_right = SDL_TRUE;
                dpad_down = SDL_TRUE;
                break;
            case 4:
                dpad_down = SDL_TRUE;
                break;
            case 5:
                dpad_left = SDL_TRUE;
                dpad_down = SDL_TRUE;
                break;
            case 6:
                dpad_left = SDL_TRUE;
                break;
            case 7:
                dpad_up = SDL_TRUE;
                dpad_left = SDL_TRUE;
                break;
            default:
                break;
            }
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_DOWN, dpad_down);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_UP, dpad_up);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, dpad_right);
            SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_LEFT, dpad_left);
        }
    }

    if (ctx->last_state.state.rgucButtonsAndHat[1] != packet->rgucButtonsAndHat[1]) {
        Uint8 data = packet->rgucButtonsAndHat[1];

        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_BACK, (data & 0x10) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_START, (data & 0x20) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_LEFTSTICK, (data & 0x40) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_RIGHTSTICK, (data & 0x80) ? SDL_PRESSED : SDL_RELEASED);
    }

    if (ctx->last_state.state.rgucButtonsAndHat[2] != packet->rgucButtonsAndHat[2]) {
        Uint8 data = packet->rgucButtonsAndHat[2];

        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_GUIDE, (data & 0x01) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_MISC1, (data & 0x02) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_PS5_TOUCHPAD, (data & 0x04) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_PS5_LEFT_FUNCTION, (data & 0x10) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_PS5_RIGHT_FUNCTION, (data & 0x20) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_PS5_LEFT_PADDLE, (data & 0x40) ? SDL_PRESSED : SDL_RELEASED);
        SDL_PrivateJoystickButton(joystick, SDL_CONTROLLER_BUTTON_PS5_RIGHT_PADDLE, (data & 0x80) ? SDL_PRESSED : SDL_RELEASED);
    }

    if (packet->ucTriggerLeft == 0 && (packet->rgucButtonsAndHat[1] & 0x04)) {
        axis = SDL_JOYSTICK_AXIS_MAX;
    } else {
        axis = ((int)packet->ucTriggerLeft * 257) - 32768;
    }
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, axis);
    if (packet->ucTriggerRight == 0 && (packet->rgucButtonsAndHat[1] & 0x08)) {
        axis = SDL_JOYSTICK_AXIS_MAX;
    } else {
        axis = ((int)packet->ucTriggerRight * 257) - 32768;
    }
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, axis);
    axis = ((int)packet->ucLeftJoystickX * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, axis);
    axis = ((int)packet->ucLeftJoystickY * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_LEFTY, axis);
    axis = ((int)packet->ucRightJoystickX * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTX, axis);
    axis = ((int)packet->ucRightJoystickY * 257) - 32768;
    SDL_PrivateJoystickAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTY, axis);

    if (ctx->report_sensors) {
        Uint32 timestamp;
        Uint64 timestamp_us;
        float data[3];

        timestamp = LOAD32(packet->rgucSensorTimestamp[0],
                           packet->rgucSensorTimestamp[1],
                           packet->rgucSensorTimestamp[2],
                           packet->rgucSensorTimestamp[3]);
        if (ctx->timestamp) {
            Uint32 delta;

            if (ctx->last_timestamp > timestamp) {
                delta = (SDL_MAX_UINT32 - ctx->last_timestamp + timestamp + 1);
            } else {
                delta = (timestamp - ctx->last_timestamp);
            }
            ctx->timestamp += delta;
        } else {
            ctx->timestamp = timestamp;
        }
        ctx->last_timestamp = timestamp;

        /* Sensor timestamp is in 0.33us units */
        timestamp_us = ctx->timestamp / 3;

        data[0] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 0, LOAD16(packet->rgucGyroX[0], packet->rgucGyroX[1]));
        data[1] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 1, LOAD16(packet->rgucGyroY[0], packet->rgucGyroY[1]));
        data[2] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 2, LOAD16(packet->rgucGyroZ[0], packet->rgucGyroZ[1]));
        SDL_PrivateJoystickSensor(joystick, SDL_SENSOR_GYRO, timestamp_us, data, 3);

        data[0] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 3, LOAD16(packet->rgucAccelX[0], packet->rgucAccelX[1]));
        data[1] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 4, LOAD16(packet->rgucAccelY[0], packet->rgucAccelY[1]));
        data[2] = HIDAPI_DriverPS5_ApplyCalibrationData(ctx, 5, LOAD16(packet->rgucAccelZ[0], packet->rgucAccelZ[1]));
        SDL_PrivateJoystickSensor(joystick, SDL_SENSOR_ACCEL, timestamp_us, data, 3);
    }
}

static void HIDAPI_DriverPS5_HandleStatePacket(SDL_Joystick *joystick, SDL_hid_device *dev, SDL_DriverPS5_Context *ctx, PS5StatePacket_t *packet)
{
    static const float TOUCHPAD_SCALEX = 1.0f / 1920;
    static const float TOUCHPAD_SCALEY = 1.0f / 1070;
    Uint8 touchpad_state;
    int touchpad_x, touchpad_y;

    if (ctx->report_touchpad) {
        touchpad_state = !(packet->ucTouchpadCounter1 & 0x80) ? SDL_PRESSED : SDL_RELEASED;
        touchpad_x = packet->rgucTouchpadData1[0] | (((int)packet->rgucTouchpadData1[1] & 0x0F) << 8);
        touchpad_y = (packet->rgucTouchpadData1[1] >> 4) | ((int)packet->rgucTouchpadData1[2] << 4);
        SDL_PrivateJoystickTouchpad(joystick, 0, 0, touchpad_state, touchpad_x * TOUCHPAD_SCALEX, touchpad_y * TOUCHPAD_SCALEY, touchpad_state ? 1.0f : 0.0f);

        touchpad_state = !(packet->ucTouchpadCounter2 & 0x80) ? SDL_PRESSED : SDL_RELEASED;
        touchpad_x = packet->rgucTouchpadData2[0] | (((int)packet->rgucTouchpadData2[1] & 0x0F) << 8);
        touchpad_y = (packet->rgucTouchpadData2[1] >> 4) | ((int)packet->rgucTouchpadData2[2] << 4);
        SDL_PrivateJoystickTouchpad(joystick, 0, 1, touchpad_state, touchpad_x * TOUCHPAD_SCALEX, touchpad_y * TOUCHPAD_SCALEY, touchpad_state ? 1.0f : 0.0f);
    }

    /* A check of packet->ucBatteryLevel & 0x10 should work as a check for BT vs USB but doesn't
     * seem to always work. Possibly related to being 100% charged?
     */
    if (!ctx->device->is_bluetooth) {
        /* 0x20 set means fully charged */
        SDL_PrivateJoystickBatteryLevel(joystick, SDL_JOYSTICK_POWER_WIRED);
    } else {
        /* Battery level ranges from 0 to 10 */
        int level = (packet->ucBatteryLevel & 0xF);
        if (level == 0) {
            SDL_PrivateJoystickBatteryLevel(joystick, SDL_JOYSTICK_POWER_EMPTY);
        } else if (level <= 2) {
            SDL_PrivateJoystickBatteryLevel(joystick, SDL_JOYSTICK_POWER_LOW);
        } else if (level <= 7) {
            SDL_PrivateJoystickBatteryLevel(joystick, SDL_JOYSTICK_POWER_MEDIUM);
        } else {
            SDL_PrivateJoystickBatteryLevel(joystick, SDL_JOYSTICK_POWER_FULL);
        }
    }

    SDL_memcpy(&ctx->last_state, packet, sizeof(ctx->last_state));
}

static void HIDAPI_DriverPS5_HandleStatePacketAlt(SDL_Joystick *joystick, SDL_hid_device *dev, SDL_DriverPS5_Context *ctx, PS5StatePacketAlt_t *packet)
{
    static const float TOUCHPAD_SCALEX = 1.0f / 1920;
    static const float TOUCHPAD_SCALEY = 1.0f / 1070;
    Uint8 touchpad_state;
    int touchpad_x, touchpad_y;

    if (ctx->report_touchpad) {
        touchpad_state = !(packet->ucTouchpadCounter1 & 0x80) ? SDL_PRESSED : SDL_RELEASED;
        touchpad_x = packet->rgucTouchpadData1[0] | (((int)packet->rgucTouchpadData1[1] & 0x0F) << 8);
        touchpad_y = (packet->rgucTouchpadData1[1] >> 4) | ((int)packet->rgucTouchpadData1[2] << 4);
        SDL_PrivateJoystickTouchpad(joystick, 0, 0, touchpad_state, touchpad_x * TOUCHPAD_SCALEX, touchpad_y * TOUCHPAD_SCALEY, touchpad_state ? 1.0f : 0.0f);

        touchpad_state = !(packet->ucTouchpadCounter2 & 0x80) ? SDL_PRESSED : SDL_RELEASED;
        touchpad_x = packet->rgucTouchpadData2[0] | (((int)packet->rgucTouchpadData2[1] & 0x0F) << 8);
        touchpad_y = (packet->rgucTouchpadData2[1] >> 4) | ((int)packet->rgucTouchpadData2[2] << 4);
        SDL_PrivateJoystickTouchpad(joystick, 0, 1, touchpad_state, touchpad_x * TOUCHPAD_SCALEX, touchpad_y * TOUCHPAD_SCALEY, touchpad_state ? 1.0f : 0.0f);
    }

    SDL_memcpy(&ctx->last_state, packet, sizeof(ctx->last_state));
}

static SDL_bool VerifyCRC(Uint8 *data, int size)
{
    Uint8 ubHdr = 0xA1; /* hidp header is part of the CRC calculation */
    Uint32 unCRC, unPacketCRC;
    Uint8 *packetCRC = data + size - sizeof(unPacketCRC);
    unCRC = SDL_crc32(0, &ubHdr, 1);
    unCRC = SDL_crc32(unCRC, data, (size_t)(size - sizeof(unCRC)));

    unPacketCRC = LOAD32(packetCRC[0],
                         packetCRC[1],
                         packetCRC[2],
                         packetCRC[3]);
    return (unCRC == unPacketCRC) ? SDL_TRUE : SDL_FALSE;
}

static SDL_bool HIDAPI_DriverPS5_IsPacketValid(SDL_DriverPS5_Context *ctx, Uint8 *data, int size)
{
    switch (data[0]) {
    case k_EPS5ReportIdState:
        return SDL_TRUE;

    case k_EPS5ReportIdBluetoothState:
        if (VerifyCRC(data, size)) {
            return SDL_TRUE;
        }
        break;
    default:
        break;
    }
    return SDL_FALSE;
}

static SDL_bool HIDAPI_DriverPS5_UpdateDevice(SDL_HIDAPI_Device *device)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;
    SDL_Joystick *joystick = NULL;
    Uint8 data[USB_PACKET_LENGTH * 2];
    int size;
    int packet_count = 0;
    Uint32 now = SDL_GetTicks();

    if (device->num_joysticks > 0) {
        joystick = SDL_JoystickFromInstanceID(device->joysticks[0]);
    }

    while ((size = SDL_hid_read_timeout(device->dev, data, sizeof(data), 0)) > 0) {
#ifdef DEBUG_PS5_PROTOCOL
        HIDAPI_DumpPacket("PS5 packet: size = %d", data, size);
#endif
        if (!HIDAPI_DriverPS5_IsPacketValid(ctx, data, size)) {
            continue;
        }

        ++packet_count;
        ctx->last_packet = now;

        if (joystick == NULL) {
            continue;
        }

        switch (data[0]) {
        case k_EPS5ReportIdState:
            if (size == 10 || size == 78) {
                HIDAPI_DriverPS5_HandleSimpleStatePacket(joystick, device->dev, ctx, (PS5SimpleStatePacket_t *)&data[1]);
            } else {
                HIDAPI_DriverPS5_HandleStatePacketCommon(joystick, device->dev, ctx, (PS5StatePacketCommon_t *)&data[1]);
                if (ctx->use_alternate_report) {
                    HIDAPI_DriverPS5_HandleStatePacketAlt(joystick, device->dev, ctx, (PS5StatePacketAlt_t *)&data[1]);
                } else {
                    HIDAPI_DriverPS5_HandleStatePacket(joystick, device->dev, ctx, (PS5StatePacket_t *)&data[1]);
                }
            }
            break;
        case k_EPS5ReportIdBluetoothState:
            if (!ctx->enhanced_mode) {
                /* This is the extended report, we can enable effects now */
                HIDAPI_DriverPS5_SetEnhancedMode(device, joystick);
            }
            if (ctx->led_reset_state == k_EDS5LEDResetStatePending) {
                HIDAPI_DriverPS5_CheckPendingLEDReset(device);
            }
            HIDAPI_DriverPS5_HandleStatePacketCommon(joystick, device->dev, ctx, (PS5StatePacketCommon_t *)&data[2]);
            if (ctx->use_alternate_report) {
                HIDAPI_DriverPS5_HandleStatePacketAlt(joystick, device->dev, ctx, (PS5StatePacketAlt_t *)&data[2]);
            } else {
                HIDAPI_DriverPS5_HandleStatePacket(joystick, device->dev, ctx, (PS5StatePacket_t *)&data[2]);
            }
            break;
        default:
#ifdef DEBUG_JOYSTICK
            SDL_Log("Unknown PS5 packet: 0x%.2x\n", data[0]);
#endif
            break;
        }
    }

    if (device->is_bluetooth) {
        if (packet_count == 0) {
            /* Check to see if it looks like the device disconnected */
            if (SDL_TICKS_PASSED(now, ctx->last_packet + BLUETOOTH_DISCONNECT_TIMEOUT_MS)) {
                /* Send an empty output report to tickle the Bluetooth stack */
                HIDAPI_DriverPS5_TickleBluetooth(device);
            }
        } else {
            /* Reconnect the Bluetooth device once the USB device is gone */
            if (device->num_joysticks == 0 &&
                !HIDAPI_HasConnectedUSBDevice(device->serial)) {
                HIDAPI_JoystickConnected(device, NULL);
            }
        }
    }

    if (size < 0 && device->num_joysticks > 0) {
        /* Read error, device is disconnected */
        HIDAPI_JoystickDisconnected(device, device->joysticks[0]);
    }
    return size >= 0;
}

static void HIDAPI_DriverPS5_CloseJoystick(SDL_HIDAPI_Device *device, SDL_Joystick *joystick)
{
    SDL_DriverPS5_Context *ctx = (SDL_DriverPS5_Context *)device->context;

    SDL_DelHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE,
                        SDL_PS5RumbleHintChanged, ctx);

    SDL_DelHintCallback(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED,
                        SDL_PS5PlayerLEDHintChanged, ctx);

    ctx->joystick = NULL;
}

static void HIDAPI_DriverPS5_FreeDevice(SDL_HIDAPI_Device *device)
{
}

SDL_HIDAPI_DeviceDriver SDL_HIDAPI_DriverPS5 = {
    SDL_HINT_JOYSTICK_HIDAPI_PS5,
    SDL_TRUE,
    HIDAPI_DriverPS5_RegisterHints,
    HIDAPI_DriverPS5_UnregisterHints,
    HIDAPI_DriverPS5_IsEnabled,
    HIDAPI_DriverPS5_IsSupportedDevice,
    HIDAPI_DriverPS5_InitDevice,
    HIDAPI_DriverPS5_GetDevicePlayerIndex,
    HIDAPI_DriverPS5_SetDevicePlayerIndex,
    HIDAPI_DriverPS5_UpdateDevice,
    HIDAPI_DriverPS5_OpenJoystick,
    HIDAPI_DriverPS5_RumbleJoystick,
    HIDAPI_DriverPS5_RumbleJoystickTriggers,
    HIDAPI_DriverPS5_GetJoystickCapabilities,
    HIDAPI_DriverPS5_SetJoystickLED,
    HIDAPI_DriverPS5_SendJoystickEffect,
    HIDAPI_DriverPS5_SetJoystickSensorsEnabled,
    HIDAPI_DriverPS5_CloseJoystick,
    HIDAPI_DriverPS5_FreeDevice,
};

#endif /* SDL_JOYSTICK_HIDAPI_PS5 */

#endif /* SDL_JOYSTICK_HIDAPI */

/* vi: set ts=4 sw=4 expandtab: */
