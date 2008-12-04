/*
    pmidiall.c:

    Copyright (C) 2004 John ffitch after Barry Vercoe
              (C) 2005 Istvan Varga
              (C) 2008 Andres Cabrera

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/* Realtime MIDI using Portmidi library */

#include "csdl.h"                               /*      PMIDI.C         */
#include "csGblMtx.h"
#include "midiops.h"
#include "oload.h"
#include <portmidi.h>
#include <porttime.h>

/* Stub for compiling this file with MinGW and linking
   with portmidi.lib built witn MSVC AND with Windows
   libraries from MinGW (missing __wassert).
*/
#if defined(WIN32) && !defined(MSVC)

void _wassert(wchar_t *condition)
{
}

#endif

typedef struct _pmall_data {
  PortMidiStream *midistream;
  struct _pmall_data *next;
} pmall_data;

static  const   int     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

static int portMidiErrMsg(CSOUND *csound, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    csound->ErrMsgV(csound, " *** PortMIDI: ", msg, args);
    va_end(args);
    return -1;
}

static int portMidi_getDeviceCount(int output)
{
    int           i, cnt1, cnt2;
    PmDeviceInfo  *info;

    cnt1 = (int)Pm_CountDevices();
    if (cnt1 < 1)
      return cnt1;      /* no devices */
    cnt2 = 0;
    for (i = 0; i < cnt1; i++) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if (output && info->output)
        cnt2++;
      else if (!output && info->input)
        cnt2++;
    }
    return cnt2;
}

static int portMidi_getRealDeviceID(int dev, int output)
{
    int           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int)Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && !(info->output)) || (!output && !(info->input)))
        continue;
      if (++j == dev)
        return i;
    }
    return -1;
}

static int portMidi_getPackedDeviceID(int dev, int output)
{
    int           i, j, cnt;
    PmDeviceInfo  *info;

    cnt = (int)Pm_CountDevices();
    i = j = -1;
    while (++i < cnt) {
      info = (PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i);
      if ((output && info->output) || (!output && info->input))
        j++;
      if (i == dev)
        return j;
    }
    return -1;
}

static PmDeviceInfo *portMidi_getDeviceInfo(int dev, int output)
{
    int i;

    i = portMidi_getRealDeviceID(dev, output);
    if (i < 0)
      return NULL;
    return ((PmDeviceInfo*)Pm_GetDeviceInfo((PmDeviceID) i));
}

// static void portMidi_listDevices(CSOUND *csound, int output)
// {
//     int           i, cnt;
//     PmDeviceInfo  *info;
// 
//     cnt = portMidi_getDeviceCount(output);
//     if (cnt < 1)
//       return;
//     if (output)
//       csound->Message(csound, Str("The available MIDI out devices are:\n"));
//     else
//       csound->Message(csound, Str("The available MIDI in devices are:\n"));
//     for (i = 0; i < cnt; i++) {
//       info = portMidi_getDeviceInfo(i, output);
//       if (info->interf != NULL)
//         csound->Message(csound, " %3d: %s (%s)\n", i, info->name, info->interf);
//       else
//         csound->Message(csound, " %3d: %s\n", i, info->name);
//     }
// }

/* reference count for PortMidi initialisation */

static unsigned long portmidi_init_cnt = 0UL;

static int stop_portmidi(CSOUND *csound, void *userData)
{
    (void) csound;
    (void) userData;
    csound_global_mutex_lock();
    if (portmidi_init_cnt) {
      if (--portmidi_init_cnt == 0UL) {
        Pm_Terminate();
        Pt_Stop();
      }
    }
    csound_global_mutex_unlock();
    return 0;
}

static int start_portmidi(CSOUND *csound)
{
    const char  *errMsg = NULL;

    csound_global_mutex_lock();
    if (!portmidi_init_cnt) {
      if (Pm_Initialize() != pmNoError)
        errMsg = Str(" *** error initialising PortMIDI");
      else if (Pt_Start(1, NULL, NULL) != ptNoError)
        errMsg = Str(" *** error initialising PortTime");
    }
    if (errMsg == NULL)
      portmidi_init_cnt++;
    csound_global_mutex_unlock();
    if (errMsg != NULL) {
      csound->ErrorMsg(csound, Str(errMsg));
      return -1;
    }
    return csound->RegisterResetCallback(csound, NULL, stop_portmidi);
}

static int OpenMidiInDevice_(CSOUND *csound, void **userData, const char *dev)
{
    int         cntdev, /* devnum,*/ i;
    PmEvent     buffer;
    PmError     retval;
    PmDeviceInfo *info;
//     PortMidiStream *midistream;
    pmall_data *data;
    pmall_data *next;


    if (start_portmidi(csound) != 0)
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(0);
    if (cntdev < 1) {
      return portMidiErrMsg(csound, Str("no input devices are available"));
    }
//     portMidi_listDevices(csound, 0);
    /* look up device in list */
//     if (dev == NULL || dev[0] == '\0')
//       devnum =
//         portMidi_getPackedDeviceID((int)Pm_GetDefaultInputDeviceID(), 0);
//     else if (dev[0] < '0' || dev[0] > '9') {
//       portMidiErrMsg(csound, Str("error: must specify a device number (>=0), "
//                                  "not a name"));
//       return -1;
//     }
//     else
//       devnum = (int)atoi(dev);
//     if (devnum < 0 || devnum >= cntdev) {
//       portMidiErrMsg(csound, Str("error: device number is out of range"));
//       return -1;
//     }
//     data->midistreams = (PortMidiStream **) calloc(data->numDevices, sizeof(PortMidiStream *));
    for (i = 0; i < cntdev; i++) {
      if (i == 0) {
        data = (pmall_data *) malloc(sizeof(pmall_data));
        next = data;
      }
      else {
        next->next = (pmall_data *) malloc(sizeof(pmall_data));
        next = next->next;
        next->next = NULL;
      }
      info = portMidi_getDeviceInfo(i, 0);
      if (info->interf != NULL)
        csound->Message(csound,
                        Str("PortMIDI: Activated input device %d: '%s' (%s)\n"),
                        i, info->name, info->interf);
      else
        csound->Message(csound, Str("PortMIDI: Activated input device %d: '%s'\n"),
                                i, info->name);
      retval = Pm_OpenInput(&next->midistream,
                            (PmDeviceID) portMidi_getRealDeviceID(i, 0),
                            NULL, 512L, (PmTimeProcPtr) NULL, NULL);
      if (retval != pmNoError) {
        return portMidiErrMsg(csound, Str("error opening input device %d: %s"),
                                      i, Pm_GetErrorText(retval));
      }
      /* only interested in channel messages (note on, control change, etc.) */
      Pm_SetFilter(next->midistream, (PM_FILT_ACTIVE | PM_FILT_SYSEX)); /* GAB: fixed for portmidi v.23Aug06 */
      /* empty the buffer after setting filter */
      while (Pm_Poll(next->midistream) == TRUE) {
        Pm_Read(next->midistream, &buffer, 1);
      }
    }
    *userData = (void*) data;
    /* report success */
    return 0;
}

static int OpenMidiOutDevice_(CSOUND *csound, void **userData, const char *dev)
{
    int         cntdev, devnum;
    PmError     retval;
    PmDeviceInfo *info;
    PortMidiStream *midistream;

    if (start_portmidi(csound) != 0)
      return -1;
    /* check if there are any devices available */
    cntdev = portMidi_getDeviceCount(1);
    if (cntdev < 1) {
      return portMidiErrMsg(csound, Str("no output devices are available"));
    }
    /* look up device in list */
//     portMidi_listDevices(csound, 1);
    if (dev == NULL || dev[0] == '\0')
      devnum =
        portMidi_getPackedDeviceID((int)Pm_GetDefaultOutputDeviceID(), 1);
    else if (dev[0] < '0' || dev[0] > '9') {
      portMidiErrMsg(csound, Str("error: must specify a device number (>=0), "
                                 "not a name"));
      return -1;
    }
    else
      devnum = (int)atoi(dev);
    if (devnum < 0 || devnum >= cntdev) {
      portMidiErrMsg(csound, Str("error: device number is out of range"));
      return -1;
    }
    info = portMidi_getDeviceInfo(devnum, 1);
    if (info->interf != NULL)
      csound->Message(csound,
                      Str("PortMIDI: selected output device %d: '%s' (%s)\n"),
                      devnum, info->name, info->interf);
    else
      csound->Message(csound,
                      Str("PortMIDI: selected output device %d: '%s'\n"),
                      devnum, info->name);
    retval = Pm_OpenOutput(&midistream,
                           (PmDeviceID) portMidi_getRealDeviceID(devnum, 1),
                           NULL, 512L, (PmTimeProcPtr) NULL, NULL, 0L);
    if (retval != pmNoError) {
      return portMidiErrMsg(csound, Str("error opening output device %d: %s"),
                                    devnum, Pm_GetErrorText(retval));
    }
    *userData = (void*) midistream;
    /* report success */
    return 0;
}

static int ReadMidiData_(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int nbytes)
{
    int             n, retval, st, d1, d2;
    PmEvent         mev;
    pmall_data *data;
    /*
     * Reads from user-defined MIDI input.
     */
    data = (pmall_data *)userData;
    n = 0;
    while (data) {
      retval = Pm_Poll(data->midistream);
      if (retval != FALSE) {
        if (retval < 0)
          return portMidiErrMsg(csound, Str("error polling input device"));
        while ((retval = Pm_Read(data->midistream, &mev, 1L)) > 0) {
          st = (int)Pm_MessageStatus(mev.message);
          d1 = (int)Pm_MessageData1(mev.message);
          d2 = (int)Pm_MessageData2(mev.message);
          /* unknown message or sysex data: ignore */
          if (st < 0x80)
            continue;
          /* ignore most system messages */
          if (st >= 0xF0 &&
              !(st == 0xF8 || st == 0xFA || st == 0xFB || st == 0xFC || st == 0xFF))
            continue;
          nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
          if (nbytes < 0) {
            portMidiErrMsg(csound, Str("buffer overflow in MIDI input"));
            break;
          }
          /* channel messages */
          n += (datbyts[(st - 0x80) >> 4] + 1);
          switch (datbyts[(st - 0x80) >> 4]) {
            case 0:
              *mbuf++ = (unsigned char) st;
              break;
            case 1:
              *mbuf++ = (unsigned char) st;
              *mbuf++ = (unsigned char) d1;
              break;
            case 2:
              *mbuf++ = (unsigned char) st;
              *mbuf++ = (unsigned char) d1;
              *mbuf++ = (unsigned char) d2;
              break;
          }
        }
        if (retval < 0) {
          portMidiErrMsg(csound, Str("read error %d"), retval);
          if (n < 1)
            n = -1;
        }
      }
      data = data->next;
    }
    /* return the number of bytes read */
    return n;
}

static int WriteMidiData_(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int nbytes)
{
    int             n, st;
    PmEvent         mev;
    PortMidiStream  *midistream;
    /*
     * Writes to user-defined MIDI output.
     */
    midistream = (PortMidiStream*) userData;
    if (nbytes < 1)
      return 0;
    n = 0;
    do {
      st = (int)*(mbuf++);
      if (st < 0x80) {
        portMidiErrMsg(csound, Str("invalid MIDI out data"));
        break;
      }
      if (st >= 0xF0 && st < 0xF8) {
        portMidiErrMsg(csound,
                       Str("MIDI out: system message 0x%02X is not supported"),
                       (unsigned int) st);
        break;
      }
      nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
      if (nbytes < 0) {
        portMidiErrMsg(csound, Str("MIDI out: truncated message"));
        break;
      }
      mev.message = (PmMessage) 0;
      mev.timestamp = (PmTimestamp) 0;
      mev.message |= (PmMessage) Pm_Message(st, 0, 0);
      if (datbyts[(st - 0x80) >> 4] > 0)
        mev.message |= (PmMessage) Pm_Message(0, (int)*(mbuf++), 0);
      if (datbyts[(st - 0x80) >> 4] > 1)
        mev.message |= (PmMessage) Pm_Message(0, 0, (int)*(mbuf++));
      if (Pm_Write(midistream, &mev, 1L) != pmNoError)
        portMidiErrMsg(csound, Str("MIDI out: error writing message"));
      else
        n += (datbyts[(st - 0x80) >> 4] + 1);
    } while (nbytes > 0);
    /* return the number of bytes written */
    return n;
}

static int CloseMidiInDevice_(CSOUND *csound, void *userData)
{
    int i = 0;
    PmError retval;
    pmall_data* data = (pmall_data*) userData;
    while (data) {
      retval = Pm_Close(data->midistream);
      if (retval != pmNoError) {
        return portMidiErrMsg(csound, Str("error closing input device"));
      }
      pmall_data* olddata;
      olddata = data;
      data = data->next;
      free(olddata);
    }
    return 0;
}

static int CloseMidiOutDevice_(CSOUND *csound, void *userData)
{
    PmError retval;

    if (userData != NULL) {
      retval = Pm_Close((PortMidiStream*) userData);
      if (retval != pmNoError) {
        return portMidiErrMsg(csound, Str("error closing output device"));
      }
    }
    return 0;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    csound->Message(csound, Str("PortMIDI real time MIDI plugin for Csound (All devices)\n"));
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;

    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portmidiall") == 0 || strcmp(drv, "PortMidiall") == 0 ||
          strcmp(drv, "PortMIDIall") == 0 || strcmp(drv, "pmall") == 0))
      return 0;
    csound->Message(csound, Str("rtmidiall: PortMIDI module enabled. (All devices)\n"));
    csound->SetExternalMidiInOpenCallback(csound, OpenMidiInDevice_);
    csound->SetExternalMidiReadCallback(csound, ReadMidiData_);
    csound->SetExternalMidiInCloseCallback(csound, CloseMidiInDevice_);
    csound->SetExternalMidiOutOpenCallback(csound, OpenMidiOutDevice_);
    csound->SetExternalMidiWriteCallback(csound, WriteMidiData_);
    csound->SetExternalMidiOutCloseCallback(csound, CloseMidiOutDevice_);
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}
