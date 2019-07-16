/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

// Menu contents for PID, RATES, RC preview, misc
// Should be part of the relevant .c file.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "platform.h"

#ifdef USE_CMS

#include "build/version.h"

#include "cms/cms.h"
#include "cms/cms_types.h"
#include "cms/cms_menu_imu.h"

#include "common/utils.h"

#include "config/feature.h"
#include "pg/pg.h"

#include "fc/config.h"
#include "fc/controlrate_profile.h"
#include "fc/rc_controls.h"
#include "fc/runtime_config.h"

#include "flight/pid.h"

#include "sensors/gyro.h"


//
// PID
//
static uint8_t tmpPidProfileIndex;
static uint8_t pidProfileIndex;
static char pidProfileIndexString[] = " p";
static uint8_t buttered_pids;
static uint8_t i_decay;
static uint8_t r_weight;
static uint8_t tempPid[3][3];
static uint16_t tempPidF[3];

static uint8_t tmpRateProfileIndex;
static uint8_t rateProfileIndex;
static char rateProfileIndexString[] = " p-r";
static controlRateConfig_t rateProfile;

static const char * const cms_offOnLabels[] = {
    "OFF", "ON"
};


static long cmsx_menuImu_onEnter(void)
{
    pidProfileIndex = getCurrentPidProfileIndex();
    tmpPidProfileIndex = pidProfileIndex + 1;

    rateProfileIndex = getCurrentControlRateProfileIndex();
    tmpRateProfileIndex = rateProfileIndex + 1;

    return 0;
}

static long cmsx_menuImu_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    changePidProfile(pidProfileIndex);
    changeControlRateProfile(rateProfileIndex);

    return 0;
}

static long cmsx_profileIndexOnChange(displayPort_t *displayPort, const void *ptr)
{
    UNUSED(displayPort);
    UNUSED(ptr);

    pidProfileIndex = tmpPidProfileIndex - 1;
    changePidProfile(pidProfileIndex);

    return 0;
}

static long cmsx_rateProfileIndexOnChange(displayPort_t *displayPort, const void *ptr)
{
    UNUSED(displayPort);
    UNUSED(ptr);

    rateProfileIndex = tmpRateProfileIndex - 1;
    changeControlRateProfile(rateProfileIndex);

    return 0;
}

static long cmsx_PidRead(void)
{

    const pidProfile_t *pidProfile = pidProfiles(pidProfileIndex);
    buttered_pids = pidProfile->buttered_pids;
    i_decay = pidProfile->i_decay;
    r_weight = pidProfile->r_weight;
    for (uint8_t i = 0; i < 3; i++) {
        tempPid[i][0] = pidProfile->pid[i].P;
        tempPid[i][1] = pidProfile->pid[i].I;
        tempPid[i][2] = pidProfile->pid[i].D;
        tempPidF[i] = pidProfile->pid[i].F;
    }

    return 0;
}

static long cmsx_PidOnEnter(void)
{
    pidProfileIndexString[1] = '0' + tmpPidProfileIndex;
    cmsx_PidRead();

    return 0;
}

static long cmsx_PidWriteback(const OSD_Entry *self)
{
    UNUSED(self);

    pidProfile_t *pidProfile = currentPidProfile;
    for (uint8_t i = 0; i < 3; i++) {
        pidProfile->buttered_pids = buttered_pids;
        pidProfile->pid[i].P = tempPid[i][0];
        pidProfile->pid[i].I = tempPid[i][1];
        pidProfile->pid[i].D = tempPid[i][2];
        pidProfile->pid[i].F = tempPidF[i];
    }
    pidProfile->i_decay = i_decay;
    pidProfile->r_weight = r_weight;
    pidInitConfig(currentPidProfile);

    return 0;
}

static OSD_Entry cmsx_menuPidEntries[] =
{
    { "-- PID --", OME_Label, NULL, pidProfileIndexString, 0},

    { "BUTTERED", OME_TAB, NULL, &(OSD_TAB_t){ &buttered_pids, 1, cms_offOnLabels }, 0 },
    { "ROLL  P", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_ROLL][0],  0, 200, 1 }, 0 },
    { "ROLL  I", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_ROLL][1],  0, 200, 1 }, 0 },
    { "ROLL  D", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_ROLL][2],  0, 200, 1 }, 0 },
    { "ROLL  F", OME_UINT16, NULL, &(OSD_UINT16_t){ &tempPidF[PID_ROLL],  0, 2000, 1 }, 0 },

    { "PITCH P", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_PITCH][0], 0, 200, 1 }, 0 },
    { "PITCH I", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_PITCH][1], 0, 200, 1 }, 0 },
    { "PITCH D", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_PITCH][2], 0, 200, 1 }, 0 },
    { "PITCH F", OME_UINT16, NULL, &(OSD_UINT16_t){ &tempPidF[PID_PITCH], 0, 2000, 1 }, 0 },

    { "YAW   P", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_YAW][0],   0, 200, 1 }, 0 },
    { "YAW   I", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_YAW][1],   0, 200, 1 }, 0 },
    { "YAW   D", OME_UINT8, NULL, &(OSD_UINT8_t){ &tempPid[PID_YAW][2],   0, 200, 1 }, 0 },
    { "YAW   F", OME_UINT16, NULL, &(OSD_UINT16_t){ &tempPidF[PID_YAW],   0, 2000, 1 }, 0 },

    { "I_DECAY", OME_UINT8, NULL, &(OSD_UINT8_t){ &i_decay,  1, 10, 1 }, 0 },
    { "R_WEIGHT", OME_UINT8, NULL, &(OSD_UINT8_t){ &r_weight,  1, 200, 1 }, 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { "SAVE&EXIT",   OME_OSD_Exit, cmsMenuExit,   (void *)CMS_EXIT_SAVE, 0},
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuPid = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XPID",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_PidOnEnter,
    .onExit = cmsx_PidWriteback,
    .entries = cmsx_menuPidEntries
};

//
// Rate & Expo
//

static long cmsx_RateProfileRead(void)
{
    memcpy(&rateProfile, controlRateProfiles(rateProfileIndex), sizeof(controlRateConfig_t));

    return 0;
}

static long cmsx_RateProfileWriteback(const OSD_Entry *self)
{
    UNUSED(self);

    memcpy(controlRateProfilesMutable(rateProfileIndex), &rateProfile, sizeof(controlRateConfig_t));

    return 0;
}

static long cmsx_RateProfileOnEnter(void)
{
    rateProfileIndexString[1] = '0' + tmpPidProfileIndex;
    rateProfileIndexString[3] = '0' + tmpRateProfileIndex;
    cmsx_RateProfileRead();

    return 0;
}

static OSD_Entry cmsx_menuRateProfileEntries[] =
{
    { "-- RATE --", OME_Label, NULL, rateProfileIndexString, 0 },

    { "RC R RATE",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcRates[FD_ROLL],    0, 255, 1, 10 }, 0 },
    { "RC P RATE",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcRates[FD_PITCH],    0, 255, 1, 10 }, 0 },
    { "RC Y RATE",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcRates[FD_YAW], 0, 255, 1, 10 }, 0 },

    { "ROLL SUPER",  OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rates[FD_ROLL],   0, 100, 1, 10 }, 0 },
    { "PITCH SUPER", OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rates[FD_PITCH],   0, 100, 1, 10 }, 0 },
    { "YAW SUPER",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rates[FD_YAW],   0, 100, 1, 10 }, 0 },

    { "RC R EXPO",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcExpo[FD_ROLL],    0, 100, 1, 10 }, 0 },
    { "RC P EXPO",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcExpo[FD_PITCH],    0, 100, 1, 10 }, 0 },
    { "RC Y EXPO",   OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.rcExpo[FD_YAW], 0, 100, 1, 10 }, 0 },

    { "THR MID",     OME_UINT8,  NULL, &(OSD_UINT8_t) { &rateProfile.thrMid8,           0,  100,  1}, 0 },
    { "THR EXPO",    OME_UINT8,  NULL, &(OSD_UINT8_t) { &rateProfile.thrExpo8,          0,  100,  1}, 0 },
    { "THRPID ATT",  OME_FLOAT,  NULL, &(OSD_FLOAT_t) { &rateProfile.dynThrPID,         0,  100,  1, 10}, 0 },
    { "TPA BRKPT",   OME_UINT16, NULL, &(OSD_UINT16_t){ &rateProfile.tpa_breakpoint, 1000, 2000, 10}, 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuRateProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "MENURATE",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_RateProfileOnEnter,
    .onExit = cmsx_RateProfileWriteback,
    .entries = cmsx_menuRateProfileEntries
};

static uint8_t  cmsx_feedForwardTransition;
static uint8_t  cmsx_angleStrength;
static uint8_t  cmsx_horizonStrength;
static uint8_t  cmsx_horizonTransition;
static uint8_t  cmsx_throttleBoost;
static uint16_t cmsx_itermAcceleratorGain;
static uint16_t cmsx_itermThrottleThreshold;

static long cmsx_profileOtherOnEnter(void)
{
    pidProfileIndexString[1] = '0' + tmpPidProfileIndex;

    const pidProfile_t *pidProfile = pidProfiles(pidProfileIndex);

    cmsx_feedForwardTransition  = pidProfile->feedForwardTransition;

    cmsx_angleStrength =     pidProfile->pid[PID_LEVEL].P;
    cmsx_horizonStrength =   pidProfile->pid[PID_LEVEL].I;
    cmsx_horizonTransition = pidProfile->pid[PID_LEVEL].D;

    cmsx_itermAcceleratorGain   = pidProfile->itermAcceleratorGain;
    cmsx_itermThrottleThreshold = pidProfile->itermThrottleThreshold;

    cmsx_throttleBoost = pidProfile->throttle_boost;

    return 0;
}

static long cmsx_profileOtherOnExit(const OSD_Entry *self)
{
    UNUSED(self);

    pidProfile_t *pidProfile = pidProfilesMutable(pidProfileIndex);
    pidProfile->feedForwardTransition = cmsx_feedForwardTransition;
    pidInitConfig(currentPidProfile);

    pidProfile->pid[PID_LEVEL].P = cmsx_angleStrength;
    pidProfile->pid[PID_LEVEL].I = cmsx_horizonStrength;
    pidProfile->pid[PID_LEVEL].D = cmsx_horizonTransition;

    pidProfile->itermAcceleratorGain   = cmsx_itermAcceleratorGain;
    pidProfile->itermThrottleThreshold = cmsx_itermThrottleThreshold;

    pidProfile->throttle_boost = cmsx_throttleBoost;

    return 0;
}

static OSD_Entry cmsx_menuProfileOtherEntries[] = {
    { "-- OTHER PP --", OME_Label, NULL, pidProfileIndexString, 0 },

    { "FF TRANS",    OME_FLOAT,  NULL, &(OSD_FLOAT_t)  { &cmsx_feedForwardTransition,  0,    100,   1, 10 }, 0 },
    { "ANGLE STR",   OME_UINT8,  NULL, &(OSD_UINT8_t)  { &cmsx_angleStrength,          0,    200,   1  }   , 0 },
    { "HORZN STR",   OME_UINT8,  NULL, &(OSD_UINT8_t)  { &cmsx_horizonStrength,        0,    200,   1  }   , 0 },
    { "HORZN TRS",   OME_UINT8,  NULL, &(OSD_UINT8_t)  { &cmsx_horizonTransition,      0,    200,   1  }   , 0 },
    { "AG GAIN",     OME_UINT16, NULL, &(OSD_UINT16_t) { &cmsx_itermAcceleratorGain,   1000, 30000, 10 }   , 0 },
    { "AG THR",      OME_UINT16, NULL, &(OSD_UINT16_t) { &cmsx_itermThrottleThreshold, 20,   1000,  1  }   , 0 },
#ifdef USE_THROTTLE_BOOST
    { "THR BOOST",   OME_UINT8,  NULL, &(OSD_UINT8_t)  { &cmsx_throttleBoost,          0,    100,   1  }   , 0 },
#endif

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuProfileOther = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XPROFOTHER",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_profileOtherOnEnter,
    .onExit = cmsx_profileOtherOnExit,
    .entries = cmsx_menuProfileOtherEntries,
};


static uint16_t gyroConfig_gyro_lowpass_hz;
static uint16_t gyroConfig_gyro_lowpass2_hz;
static uint16_t gyroConfig_gyro_soft_notch_hz_1;
static uint16_t gyroConfig_gyro_soft_notch_cutoff_1;
static uint16_t gyroConfig_gyro_soft_notch_hz_2;
static uint16_t gyroConfig_gyro_soft_notch_cutoff_2;
#ifndef USE_GYRO_IMUF9001
static uint16_t gyroConfig_gyro_filter_q;
static uint16_t gyroConfig_gyro_filter_w;
#endif

static long cmsx_menuGyro_onEnter(void)
{
    gyroConfig_gyro_lowpass_hz =  gyroConfig()->gyro_lowpass_hz;
    gyroConfig_gyro_lowpass2_hz =  gyroConfig()->gyro_lowpass2_hz;
    gyroConfig_gyro_soft_notch_hz_1 = gyroConfig()->gyro_soft_notch_hz_1;
    gyroConfig_gyro_soft_notch_cutoff_1 = gyroConfig()->gyro_soft_notch_cutoff_1;
    gyroConfig_gyro_soft_notch_hz_2 = gyroConfig()->gyro_soft_notch_hz_2;
    gyroConfig_gyro_soft_notch_cutoff_2 = gyroConfig()->gyro_soft_notch_cutoff_2;

#ifndef USE_GYRO_IMUF9001
    gyroConfig_gyro_filter_q = gyroConfig()->gyro_filter_q;
    gyroConfig_gyro_filter_w = gyroConfig()->gyro_filter_w;
#endif
    return 0;
}

static long cmsx_menuGyro_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    gyroConfigMutable()->gyro_lowpass_hz =  gyroConfig_gyro_lowpass_hz;
    gyroConfigMutable()->gyro_lowpass2_hz =  gyroConfig_gyro_lowpass2_hz;
    gyroConfigMutable()->gyro_soft_notch_hz_1 = gyroConfig_gyro_soft_notch_hz_1;
    gyroConfigMutable()->gyro_soft_notch_cutoff_1 = gyroConfig_gyro_soft_notch_cutoff_1;
    gyroConfigMutable()->gyro_soft_notch_hz_2 = gyroConfig_gyro_soft_notch_hz_2;
    gyroConfigMutable()->gyro_soft_notch_cutoff_2 = gyroConfig_gyro_soft_notch_cutoff_2;

#ifndef USE_GYRO_IMUF9001
    gyroConfigMutable()->gyro_filter_q = gyroConfig_gyro_filter_q;
    gyroConfigMutable()->gyro_filter_w = gyroConfig_gyro_filter_w;
#endif
    return 0;
}

static OSD_Entry cmsx_menuFilterGlobalEntries[] =
{
    { "-- FILTER GLB  --", OME_Label, NULL, NULL, 0 },

    { "GYRO LPF",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_lowpass_hz, 0, 16000, 1 }, 0 },
#ifdef USE_GYRO_LPF2
    { "GYRO LPF2",  OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_lowpass2_hz,  0, 16000, 1 }, 0 },
#endif
    { "GYRO NF1",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_soft_notch_hz_1,     0, 500, 1 }, 0 },
    { "GYRO NF1C",  OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_soft_notch_cutoff_1, 0, 500, 1 }, 0 },
    { "GYRO NF2",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_soft_notch_hz_2,     0, 500, 1 }, 0 },
    { "GYRO NF2C",  OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_soft_notch_cutoff_2, 0, 500, 1 }, 0 },
    #ifndef USE_GYRO_IMUF9001
    { "KALMAN Q",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_filter_q,            0, 16000, 1 }, 0 },
    { "KALMAN W",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_gyro_filter_w,            3, 512, 1 }, 0 },
    #endif
    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuFilterGlobal = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XFLTGLB",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuGyro_onEnter,
    .onExit = cmsx_menuGyro_onExit,
    .entries = cmsx_menuFilterGlobalEntries,
};

static uint16_t cmsx_dterm_lowpass_hz;
static uint16_t cmsx_dterm_lowpass2_hz;
//
// SPRING Imuf
//

#if defined(USE_GYRO_IMUF9001)
static uint16_t gyroConfig_imuf_roll_q;
static uint16_t gyroConfig_imuf_pitch_q;
static uint16_t gyroConfig_imuf_yaw_q;
static uint16_t gyroConfig_imuf_w;
static uint16_t gyroConfig_imuf_pitch_lpf_cutoff_hz;
static uint16_t gyroConfig_imuf_roll_lpf_cutoff_hz;
static uint16_t gyroConfig_imuf_yaw_lpf_cutoff_hz;
static uint16_t gyroConfig_imuf_acc_lpf_cutoff_hz;
#endif

#if defined(USE_GYRO_IMUF9001)
static long cmsx_menuImuf_onEnter(void)
{
    gyroConfig_imuf_roll_q = gyroConfig()->imuf_roll_q;
    gyroConfig_imuf_pitch_q = gyroConfig()->imuf_pitch_q;
    gyroConfig_imuf_yaw_q = gyroConfig()->imuf_yaw_q;
    gyroConfig_imuf_w = gyroConfig()->imuf_w;
    gyroConfig_imuf_pitch_lpf_cutoff_hz = gyroConfig()->imuf_pitch_lpf_cutoff_hz;
    gyroConfig_imuf_roll_lpf_cutoff_hz = gyroConfig()->imuf_roll_lpf_cutoff_hz;
    gyroConfig_imuf_yaw_lpf_cutoff_hz = gyroConfig()->imuf_yaw_lpf_cutoff_hz;
    gyroConfig_imuf_acc_lpf_cutoff_hz = gyroConfig()->imuf_acc_lpf_cutoff_hz;

    return 0;
}
#endif

#if defined(USE_GYRO_IMUF9001)
static long cmsx_menuImuf_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    gyroConfigMutable()->imuf_roll_q = gyroConfig_imuf_roll_q;
    gyroConfigMutable()->imuf_pitch_q = gyroConfig_imuf_pitch_q;
    gyroConfigMutable()->imuf_yaw_q = gyroConfig_imuf_yaw_q;
    gyroConfigMutable()->imuf_w = gyroConfig_imuf_w;
    gyroConfigMutable()->imuf_roll_lpf_cutoff_hz = gyroConfig_imuf_roll_lpf_cutoff_hz;
    gyroConfigMutable()->imuf_pitch_lpf_cutoff_hz = gyroConfig_imuf_pitch_lpf_cutoff_hz;
    gyroConfigMutable()->imuf_yaw_lpf_cutoff_hz = gyroConfig_imuf_yaw_lpf_cutoff_hz;
    gyroConfigMutable()->imuf_acc_lpf_cutoff_hz = gyroConfig_imuf_acc_lpf_cutoff_hz;
    return 0;
}
#endif

#if defined(USE_GYRO_IMUF9001)
static OSD_Entry cmsx_menuImufEntries[] =
{
    { "-- SPRING IMU-F --", OME_Label, NULL, NULL, 0 },
    { "-- CHANGES REQUIRE REBOOT --", OME_Label, NULL, NULL, 0 },
    { "IMUF W",    OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_w,                   0, 1024,    1 }, 0 },
    { "ROLL Q",    OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_roll_q,              0, 16000, 50 }, 0 },
    { "PITCH Q",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_pitch_q,             0, 16000, 50 }, 0 },
    { "YAW Q",     OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_yaw_q,               0, 16000, 50 }, 0 },
    { "ROLL LPF",  OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_roll_lpf_cutoff_hz,  0, 450,    1 }, 0 },
    { "PITCH LPF", OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_pitch_lpf_cutoff_hz, 0, 450,    1 }, 0 },
    { "YAW LPF",   OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_yaw_lpf_cutoff_hz,   0, 450,    1 }, 0 },
    { "IMUF ACC",  OME_UINT16, NULL, &(OSD_UINT16_t) { &gyroConfig_imuf_acc_lpf_cutoff_hz,   0, 450,    1 }, 0 },

	{ "BACK",        OME_Back,            NULL,   NULL,             0},
    { "SAVE&REBOOT", OME_OSD_Exit, cmsMenuExit,   (void *)CMS_EXIT_SAVEREBOOT, 0},
    { NULL, OME_END, NULL, NULL, 0 }
};
#endif

#if defined(USE_GYRO_IMUF9001)
static CMS_Menu cmsx_menuImuf = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XIMUF",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuImuf_onEnter,
    .onExit = cmsx_menuImuf_onExit,
    .entries = cmsx_menuImufEntries,
};
#endif

static uint16_t cmsx_dterm_notch_hz;
static uint16_t cmsx_dterm_notch_cutoff;
static uint16_t cmsx_yaw_lowpass_hz;

static long cmsx_FilterPerProfileRead(void)
{
    const pidProfile_t *pidProfile = pidProfiles(pidProfileIndex);

    cmsx_dterm_lowpass_hz   = pidProfile->dterm_lowpass_hz;
    cmsx_dterm_lowpass2_hz  = pidProfile->dterm_lowpass2_hz;
    cmsx_dterm_notch_hz     = pidProfile->dterm_notch_hz;
    cmsx_dterm_notch_cutoff = pidProfile->dterm_notch_cutoff;
    cmsx_yaw_lowpass_hz     = pidProfile->yaw_lowpass_hz;

    return 0;
}

static long cmsx_FilterPerProfileWriteback(const OSD_Entry *self)
{
    UNUSED(self);

    pidProfile_t *pidProfile = currentPidProfile;

    pidProfile->dterm_lowpass_hz   = cmsx_dterm_lowpass_hz;
    pidProfile->dterm_lowpass2_hz  = cmsx_dterm_lowpass2_hz;
    pidProfile->dterm_notch_hz     = cmsx_dterm_notch_hz;
    pidProfile->dterm_notch_cutoff = cmsx_dterm_notch_cutoff;
    pidProfile->yaw_lowpass_hz     = cmsx_yaw_lowpass_hz;

    return 0;
}

static OSD_Entry cmsx_menuFilterPerProfileEntries[] =
{
    { "-- FILTER PP  --", OME_Label, NULL, NULL, 0 },

    { "DTERM LPF",  OME_UINT16, NULL, &(OSD_UINT16_t){ &cmsx_dterm_lowpass_hz,     0, 500, 1 }, 0 },
    { "DTERM LPF2", OME_UINT16, NULL, &(OSD_UINT16_t){ &cmsx_dterm_lowpass2_hz,    0, 500, 1 }, 0 },
    { "DTERM NF",   OME_UINT16, NULL, &(OSD_UINT16_t){ &cmsx_dterm_notch_hz,       0, 500, 1 }, 0 },
    { "DTERM NFCO", OME_UINT16, NULL, &(OSD_UINT16_t){ &cmsx_dterm_notch_cutoff,   0, 500, 1 }, 0 },
    { "YAW LPF",    OME_UINT16, NULL, &(OSD_UINT16_t){ &cmsx_yaw_lowpass_hz,       0, 500, 1 }, 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuFilterPerProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XFLTPP",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_FilterPerProfileRead,
    .onExit = cmsx_FilterPerProfileWriteback,
    .entries = cmsx_menuFilterPerProfileEntries,
};

#ifdef USE_EXTENDED_CMS_MENUS

static uint8_t cmsx_dstPidProfile;
static uint8_t cmsx_dstControlRateProfile;

static const char * const cmsx_ProfileNames[] = {
    "-",
    "1",
    "2",
    "3"
};

static OSD_TAB_t cmsx_PidProfileTable = { &cmsx_dstPidProfile, 3, cmsx_ProfileNames };
static OSD_TAB_t cmsx_ControlRateProfileTable = { &cmsx_dstControlRateProfile, 3, cmsx_ProfileNames };

static long cmsx_menuCopyProfile_onEnter(void)
{
    cmsx_dstPidProfile = 0;
    cmsx_dstControlRateProfile = 0;

    return 0;
}

static long cmsx_CopyPidProfile(displayPort_t *pDisplay, const void *ptr)
{
    UNUSED(pDisplay);
    UNUSED(ptr);

    if (cmsx_dstPidProfile > 0) {
        pidCopyProfile(cmsx_dstPidProfile - 1, getCurrentPidProfileIndex());
    }

    return 0;
}

static long cmsx_CopyControlRateProfile(displayPort_t *pDisplay, const void *ptr)
{
    UNUSED(pDisplay);
    UNUSED(ptr);

    if (cmsx_dstControlRateProfile > 0) {
        copyControlRateProfile(cmsx_dstControlRateProfile - 1, getCurrentControlRateProfileIndex());
    }

    return 0;
}

static OSD_Entry cmsx_menuCopyProfileEntries[] =
{
    { "-- COPY PROFILE --", OME_Label, NULL, NULL, 0},

    { "CPY PID PROF TO",   OME_TAB,      NULL,                        &cmsx_PidProfileTable, 0 },
    { "COPY PP",           OME_Funcall,  cmsx_CopyPidProfile,         NULL, 0 },
    { "CPY RATE PROF TO",  OME_TAB,      NULL,                        &cmsx_ControlRateProfileTable, 0 },
    { "COPY RP",           OME_Funcall,  cmsx_CopyControlRateProfile, NULL, 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

CMS_Menu cmsx_menuCopyProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XCPY",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuCopyProfile_onEnter,
    .onExit = NULL,
    .entries = cmsx_menuCopyProfileEntries,
};

#endif

static OSD_Entry cmsx_menuImuEntries[] =
{
    { "-- PROFILE --", OME_Label, NULL, NULL, 0},

    {"PID PROF",  OME_UINT8,   cmsx_profileIndexOnChange,     &(OSD_UINT8_t){ &tmpPidProfileIndex, 1, MAX_PROFILE_COUNT, 1},    0},
    {"PID",       OME_Submenu, cmsMenuChange,                 &cmsx_menuPid,                                                 0},
    {"MISC PP",   OME_Submenu, cmsMenuChange,                 &cmsx_menuProfileOther,                                        0},
    {"FILT PP",   OME_Submenu, cmsMenuChange,                 &cmsx_menuFilterPerProfile,                                    0},

    {"RATE PROF", OME_UINT8,   cmsx_rateProfileIndexOnChange, &(OSD_UINT8_t){ &tmpRateProfileIndex, 1, CONTROL_RATE_PROFILE_COUNT, 1}, 0},
    {"RATE",      OME_Submenu, cmsMenuChange,                 &cmsx_menuRateProfile,                                         0},

    {"FILT GLB",  OME_Submenu, cmsMenuChange,                 &cmsx_menuFilterGlobal,                                        0},
#ifdef USE_EXTENDED_CMS_MENUS
#if defined(USE_GYRO_IMUF9001)
    {"IMUF",      OME_Submenu, cmsMenuChange,                 &cmsx_menuImuf,                                                0},
#endif
#ifdef USE_COPY_PROFILE_CMS_MENU
    {"COPY PROF", OME_Submenu, cmsMenuChange,                 &cmsx_menuCopyProfile,                                         0},
#endif
#endif /* USE_EXTENDED_CMS_MENUS */

    {"BACK", OME_Back, NULL, NULL, 0},
    {NULL, OME_END, NULL, NULL, 0}
};

CMS_Menu cmsx_menuImu = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XIMU",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuImu_onEnter,
    .onExit = cmsx_menuImu_onExit,
    .entries = cmsx_menuImuEntries,
};

#endif // CMS
