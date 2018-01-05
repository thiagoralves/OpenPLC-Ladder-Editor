//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
//
// This file is part of LDmicro.
// 
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// Load/save the circuit from/to a file in a nice ASCII format.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

static ElemSubcktSeries *LoadSeriesFromFile(FILE *f);

//-----------------------------------------------------------------------------
// Check a line of text from a saved project file to determine whether it
// contains a leaf element (coil, contacts, etc.). If so, create an element
// for and save that in *any and *which, and return TRUE, else return FALSE.
//-----------------------------------------------------------------------------
static BOOL LoadLeafFromFile(char *line, void **any, int *which)
{
    ElemLeaf *l = AllocLeaf();
    int x;

    if(memcmp(line, "COMMENT", 7)==0) {
        char *s = line + 8;
        int i = 0;
        while(*s && *s != '\n') {
            if(*s == '\\') {
                if(s[1] == 'n') {
                    l->d.comment.str[i++] = '\n';
                    s++;
                } else if(s[1] == 'r') {
                    l->d.comment.str[i++] = '\r';
                    s++;
                } else if(s[1] == '\\') {
                    l->d.comment.str[i++] = '\\';
                    s++;
                } else {
                    // that is odd
                }
            } else {
                l->d.comment.str[i++] = *s;
            }
            s++;
        }
        l->d.comment.str[i++] = '\0';
        *which = ELEM_COMMENT;
    } else if(sscanf_s(line, "CONTACTS %s %d", l->d.contacts.name, MAX_NAME_LEN,
        &l->d.contacts.negated)==2)
    {
        *which = ELEM_CONTACTS;
    } else if(sscanf_s(line, "COIL %s %d %d %d", l->d.coil.name, MAX_NAME_LEN,
        &l->d.coil.negated, &l->d.coil.setOnly, &l->d.coil.resetOnly)==4)
    {
        *which = ELEM_COIL;
    } else if(memcmp(line, "PLACEHOLDER", 11)==0) {
        *which = ELEM_PLACEHOLDER;
    } else if(memcmp(line, "SHORT", 5)==0) {
        *which = ELEM_SHORT;
    } else if(memcmp(line, "OPEN", 4)==0) {
        *which = ELEM_OPEN;
    } else if(memcmp(line, "MASTER_RELAY", 12)==0) {
        *which = ELEM_MASTER_RELAY;
    } else if(sscanf_s(line, "SHIFT_REGISTER %s %d", l->d.shiftRegister.name, MAX_NAME_LEN,
        &(l->d.shiftRegister.stages))==2)
    {
        *which = ELEM_SHIFT_REGISTER;
    } else if(memcmp(line, "OSR", 3)==0) {
        *which = ELEM_ONE_SHOT_RISING;
    } else if(memcmp(line, "OSF", 3)==0) {
        *which = ELEM_ONE_SHOT_FALLING;
    } else if((sscanf_s(line, "TON %s %d", l->d.timer.name, MAX_NAME_LEN,
        &l->d.timer.delay)==2))
    {
        *which = ELEM_TON;
    } else if((sscanf_s(line, "TOF %s %d", l->d.timer.name, MAX_NAME_LEN,
        &l->d.timer.delay)==2))
    {
        *which = ELEM_TOF;
    } else if((sscanf_s(line, "RTO %s %d", l->d.timer.name, MAX_NAME_LEN,
        &l->d.timer.delay)==2))
    {
        *which = ELEM_RTO;
    } else if((sscanf_s(line, "CTD %s %d", l->d.counter.name, MAX_NAME_LEN,
        &l->d.counter.max)==2))
    {
        *which = ELEM_CTD;
    } else if((sscanf_s(line, "CTU %s %d", l->d.counter.name, MAX_NAME_LEN,
        &l->d.counter.max)==2))
    {
        *which = ELEM_CTU;
    } else if((sscanf_s(line, "CTC %s %d", l->d.counter.name, MAX_NAME_LEN,
        &l->d.counter.max)==2))
    {
        *which = ELEM_CTC;
    } else if(sscanf_s(line, "RES %s", l->d.reset.name, MAX_NAME_LEN)==1) {
        *which = ELEM_RES;
    } else if(sscanf_s(line, "MOVE %s %s", l->d.move.dest, MAX_NAME_LEN, l->d.move.src, MAX_NAME_LEN)==2) {
        *which = ELEM_MOVE;
    } else if(sscanf_s(line, "ADD %s %s %s", l->d.math.dest, MAX_NAME_LEN, l->d.math.op1, MAX_NAME_LEN,
        l->d.math.op2, MAX_NAME_LEN)==3)
    {
        *which = ELEM_ADD;
    } else if(sscanf_s(line, "SUB %s %s %s", l->d.math.dest, MAX_NAME_LEN, l->d.math.op1, MAX_NAME_LEN,
        l->d.math.op2, MAX_NAME_LEN)==3)
    {
        *which = ELEM_SUB;
    } else if(sscanf_s(line, "MUL %s %s %s", l->d.math.dest, MAX_NAME_LEN, l->d.math.op1, MAX_NAME_LEN,
        l->d.math.op2, MAX_NAME_LEN)==3)
    {
        *which = ELEM_MUL;
    } else if(sscanf_s(line, "DIV %s %s %s", l->d.math.dest, MAX_NAME_LEN, l->d.math.op1, MAX_NAME_LEN,
        l->d.math.op2, MAX_NAME_LEN)==3)
    {
        *which = ELEM_DIV;
    } else if(sscanf_s(line, "EQU %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_EQU;
    } else if(sscanf_s(line, "NEQ %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_NEQ;
    } else if(sscanf_s(line, "GRT %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_GRT;
    } else if(sscanf_s(line, "GEQ %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_GEQ;
    } else if(sscanf_s(line, "LEQ %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_LEQ;
    } else if(sscanf_s(line, "LES %s %s", l->d.cmp.op1, MAX_NAME_LEN, l->d.cmp.op2, MAX_NAME_LEN)==2) {
        *which = ELEM_LES;
    } else if(sscanf_s(line, "READ_ADC %s", l->d.readAdc.name, MAX_NAME_LEN)==1) {
        *which = ELEM_READ_ADC;
    } else if(sscanf_s(line, "SET_PWM %s %d", l->d.setPwm.name, MAX_NAME_LEN,
        &(l->d.setPwm.targetFreq))==2)
    {
        *which = ELEM_SET_PWM;
    } else if(sscanf_s(line, "UART_RECV %s", l->d.uart.name, MAX_NAME_LEN)==1) {
        *which = ELEM_UART_RECV;
    } else if(sscanf_s(line, "UART_SEND %s", l->d.uart.name, MAX_NAME_LEN)==1) {
        *which = ELEM_UART_SEND;
    } else if(sscanf_s(line, "PERSIST %s", l->d.persist.var, MAX_NAME_LEN)==1) {
        *which = ELEM_PERSIST;
    } else if(sscanf_s(line, "FORMATTED_STRING %s %d", l->d.fmtdStr.var, MAX_NAME_LEN,
        &x)==2)
    {
        if(strcmp(l->d.fmtdStr.var, "(none)")==0) {
            strcpy_s(l->d.fmtdStr.var, MAX_NAME_LEN, "");
        }

        char *p = line;
        int i;
        for(i = 0; i < 3; i++) {
            while(!isspace(*p)) p++;
            while( isspace(*p)) p++;
        }
        for(i = 0; i < x; i++) {
            l->d.fmtdStr.string[i] = atoi(p);
            if(l->d.fmtdStr.string[i] < 32) {
                l->d.fmtdStr.string[i] = 'X';
            }
            while(!isspace(*p) && *p) p++;
            while( isspace(*p) && *p) p++;
        }
        l->d.fmtdStr.string[i] = '\0';

        *which = ELEM_FORMATTED_STRING;
    } else if(sscanf_s(line, "LOOK_UP_TABLE %s %s %d %d", l->d.lookUpTable.dest, MAX_NAME_LEN,
        l->d.lookUpTable.index, MAX_NAME_LEN, &(l->d.lookUpTable.count),
        &(l->d.lookUpTable.editAsString))==4)
    {
        char *p = line;
        int i;
        // First skip over the parts that we already sscanf'd.
        for(i = 0; i < 5; i++) {
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        // Then copy over the look-up table entries.
        for(i = 0; i < l->d.lookUpTable.count; i++) {
            l->d.lookUpTable.vals[i] = atoi(p);
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        *which = ELEM_LOOK_UP_TABLE;
    } else if(sscanf_s(line, "PIECEWISE_LINEAR %s %s %d", 
        l->d.piecewiseLinear.dest, MAX_NAME_LEN, l->d.piecewiseLinear.index, MAX_NAME_LEN,
        &(l->d.piecewiseLinear.count))==3)
    {
        char *p = line;
        int i;
        // First skip over the parts that we already sscanf'd.
        for(i = 0; i < 4; i++) {
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        // Then copy over the piecewise linear points.
        for(i = 0; i < l->d.piecewiseLinear.count*2; i++) {
            l->d.piecewiseLinear.vals[i] = atoi(p);
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        *which = ELEM_PIECEWISE_LINEAR;
    } else {
        // that's odd; nothing matched
        CheckFree(l);
        return FALSE;
    }
    *any = l;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Load a parallel subcircuit from a file. We look for leaf nodes using
// LoadLeafFromFile, which we can put directly into the parallel circuit
// that we're building up, or series subcircuits that we can pass to
// LoadSeriesFromFile. Returns the parallel subcircuit built up, or NULL if
// something goes wrong.
//-----------------------------------------------------------------------------
static ElemSubcktParallel *LoadParallelFromFile(FILE *f)
{
    char line[512];
    void *any;
    int which;

    ElemSubcktParallel *ret = AllocSubcktParallel();
    int cnt = 0;
    
    for(;;) {
        if(!fgets(line, sizeof(line), f)) return NULL;
        char *s = line;
        while(isspace(*s)) s++;

        if(strcmp(s, "SERIES\n")==0) {
            which = ELEM_SERIES_SUBCKT;
            any = LoadSeriesFromFile(f);
            if(!any) return NULL;

        } else if(LoadLeafFromFile(s, &any, &which)) {
            // got it
        } else if(strcmp(s, "END\n")==0) {
            ret->count = cnt;
            return ret;
        } else {
            return NULL;
        }
        ret->contents[cnt].which = which;
        ret->contents[cnt].d.any = any;
        cnt++;
        if(cnt >= MAX_ELEMENTS_IN_SUBCKT) return NULL;
    }
}

//-----------------------------------------------------------------------------
// Same as LoadParallelFromFile, but for a series subcircuit. Thus builds
// a series circuit out of parallel circuits and leaf elements.
//-----------------------------------------------------------------------------
static ElemSubcktSeries *LoadSeriesFromFile(FILE *f)
{
    char line[512];
    void *any;
    int which;

    ElemSubcktSeries *ret = AllocSubcktSeries();
    int cnt = 0;
    
    for(;;) {
        if(!fgets(line, sizeof(line), f)) return NULL;
        char *s = line;
        while(isspace(*s)) s++;

        if(strcmp(s, "PARALLEL\n")==0) {
            which = ELEM_PARALLEL_SUBCKT;
            any = LoadParallelFromFile(f);
            if(!any) return NULL;

        } else if(LoadLeafFromFile(s, &any, &which)) {
            // got it
        } else if(strcmp(s, "END\n")==0) {
            ret->count = cnt;
            return ret;
        } else {
            return NULL;
        }
        ret->contents[cnt].which = which;
        ret->contents[cnt].d.any = any;
        cnt++;
        if(cnt >= MAX_ELEMENTS_IN_SUBCKT) return NULL;
    }
}

//-----------------------------------------------------------------------------
// Load a project from a saved project description files. This describes the
// program, the target processor, plus certain configuration settings (cycle
// time, processor clock, etc.). Return TRUE for success, FALSE if anything
// went wrong.
//-----------------------------------------------------------------------------
BOOL LoadProjectFromFile(char *filename)
{
    FreeEntireProgram();
    strcpy_s(CurrentCompileFile, MAX_PATH, "");

	FILE *f;
    if(fopen_s(&f, filename, "r")==EINVAL) return FALSE;

    char line[512];
    int crystal, cycle, baud;

    while(fgets(line, sizeof(line), f)) {
        if(strcmp(line, "IO LIST\n")==0) {
            if(!LoadIoListFromFile(f)) {
                fclose(f);
                return FALSE;
            }
        } else if(sscanf_s(line, "CRYSTAL=%d", &crystal)) {
            Prog.mcuClock = crystal;
        } else if(sscanf_s(line, "CYCLE=%d", &cycle)) {
            Prog.cycleTime = cycle;
        } else if(sscanf_s(line, "BAUD=%d", &baud)) {
            Prog.baudRate = baud;
        } else if(memcmp(line, "COMPILED=", 9)==0) {
            line[strlen(line)-1] = '\0';
            strcpy_s(CurrentCompileFile, MAX_PATH, line+9);
        } else if(memcmp(line, "MICRO=", 6)==0) {
            line[strlen(line)-1] = '\0';
            int i;
            for(i = 0; i < NUM_SUPPORTED_MCUS; i++) {
                if(strcmp(SupportedMcus[i].mcuName, line+6)==0) {
                    Prog.mcu = &SupportedMcus[i];
                    break;
                }
            }
            if(i == NUM_SUPPORTED_MCUS) {
                Error(_("Microcontroller '%s' not supported.\r\n\r\n"
                    "Defaulting to no selected MCU."), line+6);
            }
        } else if(strcmp(line, "PROGRAM\n")==0) {
            break;
        }
    }
    if(strcmp(line, "PROGRAM\n") != 0) goto failed;

    int rung;
    for(rung = 0;;) {
        if(!fgets(line, sizeof(line), f)) break;
        if(strcmp(line, "RUNG\n")!=0) goto failed;

        Prog.rungs[rung] = LoadSeriesFromFile(f);
        if(!Prog.rungs[rung]) goto failed;
        rung++;
    }
    Prog.numRungs = rung;

    fclose(f);
    return TRUE;

failed:
    fclose(f);
    NewProgram();
    Error(_("File format error; perhaps this program is for a newer version "
            "of LDmicro?"));
    return FALSE;
}

//-----------------------------------------------------------------------------
// Helper routine for outputting hierarchical representation of the ladder
// logic: indent on file f, by depth*4 spaces.
//-----------------------------------------------------------------------------
static void Indent(FILE *f, int depth)
{
    int i;
    for(i = 0; i < depth; i++) {
        fprintf_s(f, "    ");
    }
}

//-----------------------------------------------------------------------------
// Save an element to a file. If it is a leaf, then output a single line
// describing it and return. If it is a subcircuit, call ourselves
// recursively (with depth+1, so that the indentation is right) to handle
// the members of the subcircuit. Special case for depth=0: we do not
// output the SERIES/END delimiters. This is because the root is delimited
// by RUNG/END markers output elsewhere.
//-----------------------------------------------------------------------------
static void SaveElemToFile(FILE *f, int which, void *any, int depth)
{
    ElemLeaf *l = (ElemLeaf *)any;
    char *s;

    Indent(f, depth);

    switch(which) {
        case ELEM_PLACEHOLDER:
            fprintf_s(f, "PLACEHOLDER\n");
            break;

        case ELEM_COMMENT: {
            fprintf_s(f, "COMMENT ");
            char *s = l->d.comment.str;
            for(; *s; s++) {
                if(*s == '\\') {
                    fprintf_s(f, "\\\\");
                } else if(*s == '\n') {
                    fprintf_s(f, "\\n");
                } else if(*s == '\r') {
                    fprintf_s(f, "\\r");
                } else {
                    fprintf_s(f, "%c", *s);
                }
            }
            fprintf_s(f, "\n");
            break;
        }
        case ELEM_OPEN:
            fprintf_s(f, "OPEN\n");
            break;

        case ELEM_SHORT:
            fprintf_s(f, "SHORT\n");
            break;

        case ELEM_MASTER_RELAY:
            fprintf_s(f, "MASTER_RELAY\n");
            break;
        
        case ELEM_SHIFT_REGISTER:
            fprintf_s(f, "SHIFT_REGISTER %s %d\n", l->d.shiftRegister.name,
                l->d.shiftRegister.stages);
            break;

        case ELEM_CONTACTS:
            fprintf_s(f, "CONTACTS %s %d\n", l->d.contacts.name,
                l->d.contacts.negated);
            break;

        case ELEM_COIL:
            fprintf_s(f, "COIL %s %d %d %d\n", l->d.coil.name, l->d.coil.negated,
                l->d.coil.setOnly, l->d.coil.resetOnly);
            break;

        case ELEM_TON:
            s = "TON"; goto timer;
        case ELEM_TOF:
            s = "TOF"; goto timer;
        case ELEM_RTO:
            s = "RTO"; goto timer;

timer:
            fprintf_s(f, "%s %s %d\n", s, l->d.timer.name, l->d.timer.delay);
            break;

        case ELEM_CTU:
            s = "CTU"; goto counter;
        case ELEM_CTD:
            s = "CTD"; goto counter;
        case ELEM_CTC:
            s = "CTC"; goto counter;

counter:
            fprintf_s(f, "%s %s %d\n", s, l->d.counter.name, l->d.counter.max);
            break;

        case ELEM_RES:
            fprintf_s(f, "RES %s\n", l->d.reset.name);
            break;

        case ELEM_MOVE:
            fprintf_s(f, "MOVE %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_ADD: s = "ADD"; goto math;
        case ELEM_SUB: s = "SUB"; goto math;
        case ELEM_MUL: s = "MUL"; goto math;
        case ELEM_DIV: s = "DIV"; goto math;
math:
            fprintf_s(f, "%s %s %s %s\n", s, l->d.math.dest, l->d.math.op1,
                l->d.math.op2);
            break;

        case ELEM_EQU: s = "EQU"; goto cmp;
        case ELEM_NEQ: s = "NEQ"; goto cmp;
        case ELEM_GRT: s = "GRT"; goto cmp;
        case ELEM_GEQ: s = "GEQ"; goto cmp;
        case ELEM_LES: s = "LES"; goto cmp;
        case ELEM_LEQ: s = "LEQ"; goto cmp;
cmp:
            fprintf_s(f, "%s %s %s\n", s, l->d.cmp.op1, l->d.cmp.op2);
            break;

        case ELEM_ONE_SHOT_RISING:
            fprintf_s(f, "OSR\n");
            break;

        case ELEM_ONE_SHOT_FALLING:
            fprintf_s(f, "OSF\n");
            break;

        case ELEM_READ_ADC:
            fprintf_s(f, "READ_ADC %s\n", l->d.readAdc.name);
            break;

        case ELEM_SET_PWM:
            fprintf_s(f, "SET_PWM %s %d\n", l->d.setPwm.name,
                l->d.setPwm.targetFreq);
            break;

        case ELEM_UART_RECV:
            fprintf_s(f, "UART_RECV %s\n", l->d.uart.name);
            break;

        case ELEM_UART_SEND:
            fprintf_s(f, "UART_SEND %s\n", l->d.uart.name);
            break;

        case ELEM_PERSIST:
            fprintf_s(f, "PERSIST %s\n", l->d.persist.var);
            break;

        case ELEM_FORMATTED_STRING: {
            int i;
            fprintf_s(f, "FORMATTED_STRING ");
            if(*(l->d.fmtdStr.var)) {
                fprintf_s(f, "%s", l->d.fmtdStr.var);
            } else {
                fprintf_s(f, "(none)");
            }
            fprintf_s(f, " %d", strlen(l->d.fmtdStr.string));
            for(i = 0; i < (int)strlen(l->d.fmtdStr.string); i++) {
                fprintf_s(f, " %d", l->d.fmtdStr.string[i]);
            }
            fprintf_s(f, "\n");
            break;
        }
        case ELEM_LOOK_UP_TABLE: {
            int i;
            fprintf_s(f, "LOOK_UP_TABLE %s %s %d %d", l->d.lookUpTable.dest,
                l->d.lookUpTable.index, l->d.lookUpTable.count,
                l->d.lookUpTable.editAsString);
            for(i = 0; i < l->d.lookUpTable.count; i++) {
                fprintf_s(f, " %d", l->d.lookUpTable.vals[i]);
            }
            fprintf_s(f, "\n");
            break;
        }
        case ELEM_PIECEWISE_LINEAR: {
            int i;
            fprintf_s(f, "PIECEWISE_LINEAR %s %s %d", l->d.piecewiseLinear.dest,
                l->d.piecewiseLinear.index, l->d.piecewiseLinear.count);
            for(i = 0; i < l->d.piecewiseLinear.count*2; i++) {
                fprintf_s(f, " %d", l->d.piecewiseLinear.vals[i]);
            }
            fprintf_s(f, "\n");
            break;
        }

        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int i;
            if(depth == 0) {
                fprintf_s(f, "RUNG\n");
            } else {
                fprintf_s(f, "SERIES\n");
            }
            for(i = 0; i < s->count; i++) {
                SaveElemToFile(f, s->contents[i].which, s->contents[i].d.any,
                    depth+1);
            }
            Indent(f, depth);
            fprintf_s(f, "END\n");
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *s = (ElemSubcktParallel *)any;
            int i;
            fprintf_s(f, "PARALLEL\n");
            for(i = 0; i < s->count; i++) {
                SaveElemToFile(f, s->contents[i].which, s->contents[i].d.any,
                    depth+1);
            }
            Indent(f, depth);
            fprintf_s(f, "END\n");
            break;
        }

        default:
            oops();
            break;
    }
}

//-----------------------------------------------------------------------------
// Save the program in memory to the given file. Returns TRUE for success,
// FALSE otherwise.
//-----------------------------------------------------------------------------
BOOL SaveProjectToFile(char *filename)
{
    FILE *f;
    if(fopen_s(&f, filename, "w")==EINVAL) return FALSE;

    fprintf_s(f, "LDmicro0.1\n");
    if(Prog.mcu) {
        fprintf_s(f, "MICRO=%s\n", Prog.mcu->mcuName);
    }
    fprintf_s(f, "CYCLE=%d\n", Prog.cycleTime);
    fprintf_s(f, "CRYSTAL=%d\n", Prog.mcuClock);
    fprintf_s(f, "BAUD=%d\n", Prog.baudRate);
    if(strlen(CurrentCompileFile) > 0) {
        fprintf_s(f, "COMPILED=%s\n", CurrentCompileFile);
    }

    // list extracted from schematic, but the pin assignments are not
    fprintf_s(f, "\nIO LIST\n");
    SaveIoListToFile(f);
    fprintf_s(f, "END\n\nPROGRAM\n");

    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        SaveElemToFile(f, ELEM_SERIES_SUBCKT, Prog.rungs[i], 0);
    }

    fclose(f);
    return TRUE;
}
