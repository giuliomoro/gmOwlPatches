#ifndef __WavetableOscillatorTestPatch_hpp__
#define __WavetableOscillatorTestPatch_hpp__

#include "Patch.h"
#include "WavetableOscillator.h"
#include "SineOscillator.hpp"

class WavetableOscillatorTestPatch : public Patch {
public:
  WavetableOscillator oscNo;
  SmoothWavetableOscillator oscLin;
  SmoothWavetableOscillator4 osc4;
  SmoothWavetableOscillator4 lfo;
  SmoothWavetableOscillator4 fm;
  FloatArray table;
  FloatArray fc;
  WavetableOscillatorTestPatch()
  {
    registerParameter(PARAMETER_A, "Frequency");
    registerParameter(PARAMETER_B, "LFO rate");
    registerParameter(PARAMETER_C, "FM freq");
    registerParameter(PARAMETER_D, "Interpolation type");
    int size = 1024;
    table = FloatArray::create(size + 3);
    for(unsigned int n = 0; n < size; ++n){
      table[n] = sin(2 * M_PI * n / size);
    }
    for(unsigned int n = 0; n < 3; ++n){
      table[n + size] = table[n];
    }
    oscNo.setTable(FloatArray(table.getData() + 1, size));
    oscLin.setTable(FloatArray(table.getData() + 1, size + 1));
    FloatArray table4 = FloatArray(table.getData(), size + 3);
    osc4.setTable(table4);
    lfo.setTable(table4);
    lfo.setTimeBase(getBlockSize());
    fm.setTable(table4);
	fc = FloatArray::create(getBlockSize());
  }

  ~WavetableOscillatorTestPatch(){
    FloatArray::destroy(table);
  }

  void processAudio(AudioBuffer &buffer){
    float frequency = getParameterValue(PARAMETER_A) * 570 + 30;
    float parameterB = getParameterValue(PARAMETER_B);
    float parameterC = getParameterValue(PARAMETER_C);
    float parameterD = getParameterValue(PARAMETER_D);
    float fmWidth = 0.1;
    FloatArray fa=buffer.getSamples(0);
    FloatArray fb=buffer.getSamples(1);
    float lfoValue;
    // make the lfo knob inactive when close to 0
    if (parameterB < 0.05){
      lfoValue = 1;
    } else {
      lfo.setFrequency(parameterB * 30);
      lfoValue = lfo.getNextSample();
    }
    // smooth the LFO value
    for(int n = 0; n < fc.getSize(); ++n){
      static float oldLfoValue = 0;
      fc[n] = oldLfoValue * 0.99 + lfoValue * 0.01;
      oldLfoValue = fc[n];
    }
 
    // make the width fm knob inactive when close to 0
    if (parameterC < 0.05){
      fa.setAll(0);
    } else {
      float fmFreq = (parameterC - 0.05) * 1000;
      fm.setFrequency(fmFreq);
      fm.getSamples(fa);
      fa.multiply(fmWidth);
    }
    static WavetableOscillator* osc = &oscNo;
	static float oldParameterD = parameterD;
    if(parameterD < 0.33){ // use no interpolation
	  if(oldParameterD >= 0.33)
        oscNo.setPhase(osc->getPhase());
      osc = &oscNo;
    } else if (parameterD < 0.66){ // use linear interpolation
	  if(oldParameterD >= 0.66 || oldParameterD < 0.33)
        oscLin.setPhase(osc->getPhase());
      osc = &oscLin;
    } else {
	  if(oldParameterD < 0.66)
        osc4.setPhase(osc->getPhase());
      osc = &osc4;
    }
    oldParameterD = parameterD;
    fb.copyFrom(fa); // output the modulator to the right channel
    osc->setFrequency(frequency);
    osc->getSamples(fa, fa);// the carrier oscillator is FM'd
    fa.multiply(fc); // the carrier oscillator is AM'd  and output to the left channel
  }
};

#endif // __WavetableOscillatorTestPatch_hpp__
