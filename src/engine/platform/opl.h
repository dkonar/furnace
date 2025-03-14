/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2023 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPL_H
#define _OPL_H

#include "../dispatch.h"
#include "../../fixedQueue.h"
#include "../../../extern/opl/opl3.h"
extern "C" {
#include "../../../extern/YM3812-LLE/fmopl2.h"
#include "../../../extern/YMF262-LLE/fmopl3.h"
}
#include "sound/ymfm/ymfm_adpcm.h"
#include "sound/ymfm/ymfm_opl.h"

class DivOPLAInterface: public ymfm::ymfm_interface {
  public:
    unsigned char* adpcmBMem;
    int sampleBank;
    uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address);
    void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data);
    DivOPLAInterface(): adpcmBMem(NULL), sampleBank(0) {}
};

class DivPlatformOPL: public DivDispatch {
  protected:
    struct Channel: public SharedChannel<int> {
      DivInstrumentFM state;
      unsigned char freqH, freqL;
      int sample, fixedFreq;
      bool furnacePCM, fourOp, hardReset;
      unsigned char pan;
      int macroVolMul;
      Channel():
        SharedChannel<int>(0),
        freqH(0),
        freqL(0),
        sample(-1),
        fixedFreq(0),
        furnacePCM(false),
        fourOp(false),
        hardReset(false),
        pan(3),
        macroVolMul(64) {
        state.ops=2;
      }
    };
    Channel chan[20];
    DivDispatchOscBuffer* oscBuf[20];
    bool isMuted[20];
    struct QueuedWrite {
      unsigned short addr;
      unsigned char val;
      bool addrOrVal;
      QueuedWrite(): addr(0), val(0), addrOrVal(false) {}
      QueuedWrite(unsigned short a, unsigned char v): addr(a), val(v), addrOrVal(false) {}
    };
    FixedQueue<QueuedWrite,2048> writes;

    unsigned int dacVal;
    unsigned int dacVal2;
    int dacOut;
    int dacOut3[4];
    bool lastSH;
    bool lastSH2;
    bool lastSY;
    bool waitingBusy;
    
    unsigned char* adpcmBMem;
    size_t adpcmBMemLen;
    DivOPLAInterface iface;
    unsigned int sampleOffB[256];
    bool sampleLoaded[256];
  
    ymfm::adpcm_b_engine* adpcmB;
    const unsigned char** slotsNonDrums;
    const unsigned char** slotsDrums;
    const unsigned char** slots;
    const unsigned short* chanMap;
    const unsigned char* outChanMap;
    int chipFreqBase, chipRateBase;
    int delay, chipType, oplType, chans, melodicChans, totalChans, adpcmChan, sampleBank, totalOutputs;
    unsigned char lastBusy;
    unsigned char drumState;
    unsigned char drumVol[5];

    unsigned char regPool[512];
  
    bool properDrums, properDrumsSys, dam, dvb;

    unsigned char lfoValue;

    // 0: Nuked-OPL3
    // 1: ymfm
    // 2: YM3812-LLE/YMF262-LLE
    unsigned char emuCore;

    bool update4OpMask, pretendYMU, downsample, compatPan;
  
    short oldWrites[512];
    short pendingWrites[512];

    // chips
    opl3_chip fm;
    ymfm::ym3526* fm_ymfm1;
    ymfm::ym3812* fm_ymfm2;
    ymfm::y8950* fm_ymfm8950;
    ymfm::ymf262* fm_ymfm3;
    fmopl2_t fm_lle2;
    fmopl3_t fm_lle3;

    int octave(int freq);
    int toFreq(int freq);
    double NOTE_ADPCMB(int note);
    void commitState(int ch, DivInstrument* ins);

    friend void putDispatchChip(void*,int);
    friend void putDispatchChan(void*,int,int);

    void acquire_nukedLLE2(short** buf, size_t len);
    void acquire_nukedLLE3(short** buf, size_t len);
    void acquire_nuked(short** buf, size_t len);
    void acquire_ymfm3(short** buf, size_t len);
    void acquire_ymfm8950(short** buf, size_t len);
    void acquire_ymfm2(short** buf, size_t len);
    void acquire_ymfm1(short** buf, size_t len);
  
  public:
    void acquire(short** buf, size_t len);
    int dispatch(DivCommand c);
    void* getChanState(int chan);
    DivMacroInt* getChanMacroInt(int ch);
    unsigned short getPan(int chan);
    DivChannelPair getPaired(int chan);
    DivDispatchOscBuffer* getOscBuffer(int chan);
    int mapVelocity(int ch, float vel);
    unsigned char* getRegisterPool();
    int getRegisterPoolSize();
    void reset();
    void forceIns();
    void tick(bool sysTick=true);
    void muteChannel(int ch, bool mute);
    int getOutputCount();
    void setCore(unsigned char which);
    void setOPLType(int type, bool drums);
    bool keyOffAffectsArp(int ch);
    bool keyOffAffectsPorta(int ch);
    void toggleRegisterDump(bool enable);
    void setFlags(const DivConfig& flags);
    void notifyInsChange(int ins);
    void notifyInsDeletion(void* ins);
    int getPortaFloor(int ch);
    void poke(unsigned int addr, unsigned short val);
    void poke(std::vector<DivRegWrite>& wlist);
    const void* getSampleMem(int index);
    size_t getSampleMemCapacity(int index);
    size_t getSampleMemUsage(int index);
    bool isSampleLoaded(int index, int sample);
    void renderSamples(int chipID);
    int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags);
    void quit();
    ~DivPlatformOPL();
};
#endif
