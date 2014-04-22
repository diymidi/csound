/*
    csdebug.h:

    Copyright (C) 2013 Andres Cabrera

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

#ifndef CSDEBUG_H
#define CSDEBUG_H

#ifndef CSDEBUGGER
#error You must build csound with debugger enabled to use this header.
#endif

// Necessary to access private members within the Csound structure even from applications that include this header
#ifdef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND_DEFINED
#else
#define __BUILDING_LIBCSOUND
#endif

#include "pthread.h"
#include "csoundCore.h"

typedef enum {
    CSDEBUG_BKPT_LINE,
    CSDEBUG_BKPT_INSTR,
    CSDEBUG_BKPT_DELETE,
    CSDEBUG_BKPT_CLEAR_ALL
} bkpt_mode_t;

typedef enum {
    CSDEBUG_STATUS_RUNNING,
    CSDEBUG_STATUS_STOPPED,
    CSDEBUG_STATUS_CONTINUE
} debug_status_t;

typedef struct bkpt_node_s {
    int line; /* if line is < 0 breakpoint is for instrument instances */
    MYFLT instr; /* instrument number (including fractional part */
    int skip; /* number of times to skip when arriving at the breakpoint */
    int count; /* current backwards count for skip, when 0 break */
    bkpt_mode_t mode;
    struct bkpt_node_s *next;
} bkpt_node_t;

typedef enum {
    CSDEBUG_CMD_NONE,
    CSDEBUG_CMD_STEPOVER,
    CSDEBUG_CMD_STEPINTO,
    CSDEBUG_CMD_NEXT,
    CSDEBUG_CMD_CONTINUE,
    CSDEBUG_CMD_STOP
} debug_command_t;

typedef enum {
    CSDEBUG_OFF = 0x0,
    CSDEBUG_K = 0x01,
    CSDEBUG_INIT = 0x02
} debug_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*breakpoint_cb_t) (CSOUND *, int line, double instr, void *userdata);

typedef struct {
    void *bkpt_buffer; /* for passing breakpoints to the running engine */
    void *cmd_buffer; /* for passing commands to the running engine */
    debug_status_t status;
    bkpt_node_t *bkpt_anchor; /* linked list for breakpoints */
    INSDS *debug_instr_ptr; /* != NULL when stopped at a breakpoint */
    breakpoint_cb_t bkpt_cb;
    void *cb_data;
} csdebug_data_t;


PUBLIC void csoundDebuggerInit(CSOUND *csound);
PUBLIC void csoundDebuggerClean(CSOUND *csound);

PUBLIC void csoundDebugSetMode(CSOUND *csound, debug_mode_t enabled);

PUBLIC void csoundSetBreakpoint(CSOUND *csound, int line, int skip);
PUBLIC void csoundRemoveBreakpoint(CSOUND *csound, int line);
PUBLIC void csoundSetInstrumentBreakpoint(CSOUND *csound, MYFLT instr, int skip);
PUBLIC void csoundRemoveInstrumentBreakpoint(CSOUND *csound, MYFLT instr);
PUBLIC void csoundClearBreakpoints(CSOUND *csound);

PUBLIC void csoundSetBreakpointCallback(CSOUND *csound, breakpoint_cb_t bkpt_cb, void *userdata);

PUBLIC void csoundDebugStepOver(CSOUND *csound);
PUBLIC void csoundDebugStepInto(CSOUND *csound);
PUBLIC void csoundDebugNext(CSOUND *csound);
PUBLIC void csoundDebugContinue(CSOUND *csound);
PUBLIC void csoundDebugStop(CSOUND *csound);

PUBLIC INSDS *csoundDebugGetInstrument(CSOUND *csound);


#ifdef __cplusplus
}
#endif

#ifndef __BUILDING_LIBCSOUND_DEFINED
#undef __BUILDING_LIBCSOUND
#endif
#endif