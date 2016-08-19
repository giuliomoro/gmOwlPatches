#ifndef __WavetableOscillatorTestPatch_hpp__
#define __WavetableOscillatorTestPatch_hpp__

#include "Patch.h"
#include "WavetableOscillator.h"

class WavetableOscillatorTestPatch : public Patch {
public:
  WavetableOscillator osc;
  WavetableOscillator lfo;
  WavetableOscillator fm;
  FloatArray table;
  WavetableOscillatorTestPatch() : 
    // a bit of a waste to construct these with an unallocated FloatArray
    osc(table), lfo(getBlockSize(),table), fm(table)
  {
    registerParameter(PARAMETER_A, "Frequency");
    registerParameter(PARAMETER_B, "LFO rate");
    registerParameter(PARAMETER_C, "FM freq");
    registerParameter(PARAMETER_D, "FM width");
    table = FloatArray::create(512);
    for(unsigned int n = 0; n < table.getSize(); ++n){
      table[n] = sin(2 * M_PI * n / getSampleRate());
    }
    osc.setTable(table);
    lfo.setTable(table);
    fm.setTable(table);
  }

  ~WavetableOscillatorTestPatch(){
    FloatArray::destroy(table);
  }

  void processAudio(AudioBuffer &buffer){
    float frequency = getParameterValue(PARAMETER_A) * 500 + 100;
    float parameterB = getParameterValue(PARAMETER_B);
    float fmFreq = getParameterValue(PARAMETER_C) * 1000;
    float fmWidth = getParameterValue(PARAMETER_D) * 1;

    FloatArray fa=buffer.getSamples(0);
    FloatArray fb=buffer.getSamples(1);
    float lfoValue;
    // make the lfo knob inactive when close to 0
    if (parameterB < 0.05){
      lfoValue = 1;
    } else {
      lfo.setPeriod(1 - (parameterB * 0.9 + 0.1) + 0.001);
      lfoValue = lfo.getNextSample();
    }
    // smooth the LFO value
    for(int n = 0; n < fb.getSize(); ++n){
      static float oldLfoValue = 0;
      fb[n] = oldLfoValue * 0.99 + lfoValue * 0.01;
      oldLfoValue = fb[n];
    }
    fm.setFrequency(fmFreq);
    fm.getSamples(fa);
    fa.multiply(fmWidth);
    osc.setFrequency(frequency);
    osc.getSamples(fa, fa); // the carrier oscillator is modulated
    fa.multiply(fb);
    buffer.getSamples(1).copyFrom(fa);
  }
};

#endif // __WavetableOscillatorTestPatch_hpp__
