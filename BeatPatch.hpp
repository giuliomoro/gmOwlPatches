////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 
 
 LICENSE:
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */


/* created by the OWL team 2013 */


////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __BeatPatch_hpp__
#define __BeatPatch_hpp__

#include "StompBox.h"
#include "BiquadFilter.h"

class BeatPatch : public Patch {
public:
  BiquadFilter *filter;
  float downsampleRatio = 100;
  float downsampledCount = 0;
  float time = 3;
  int writePtr = 0;
  float fs;
  int N = downsampleRatio * time;
  static constexpr int srange[2] = {183, 294};
  int startT = srange[1]+1; // TODO: really + 1?
  static const int lengthTaus = srange[1] - srange[0];
  FloatArray comb[lengthTaus];
  FloatArray sums;
  FloatArray tmp;
  FloatArray alphas;
  BeatPatch(){
    debugMessage(" ");
    filter = BiquadFilter::create(2);
    filter->setLowPass(downsampleRatio/getSampleRate(), 0.707);
    fs = getSampleRate() / downsampleRatio;
    sums = FloatArray::create(lengthTaus);
    tmp = FloatArray::create(getBlockSize());
    alphas = FloatArray::create(lengthTaus);
    float t0 = 1500 / fs;
    for(int n = 0; n < lengthTaus; ++n)
    {
      comb[n] = FloatArray::create(startT+N);
      comb[n].setAll(0);
      float tau = n + srange[0];
      alphas[n] = powf(0.5, (tau/fs) / t0);
    }

    //registerParameter(PARAMETER_A, "Cutoff");
    //registerParameter(PARAMETER_B, "Resonance");
  }
  void processAudio(AudioBuffer &buffer){
    //float resonance=10*getParameterValue(PARAMETER_B);
    FloatArray fa=buffer.getSamples(0);
    tmp.copyFrom(fa);
    //fa.noise();
    filter->process(fa);
    for(int n = 0; n < fa.getSize(); ++n)
    {
      if(downsampledCount == 0)
      {
        float in = fa[n];
        for(int tauIndex = 0; tauIndex < lengthTaus; ++tauIndex)
        {
          float out;
          int tau = tauIndex + srange[0];
          if(writePtr - tau >= 0)
          {
            out = alphas[n] * comb[tauIndex][writePtr - tau] + (1 - alphas[n]) * in;
          } else {
            out = 0;
          }
          comb[tauIndex][writePtr] = out;
          // compute the sum one sample at a time, to avoid spikes
          sums[tauIndex] += out;
        }
        ++writePtr;
        if(writePtr == N + startT)
        {
          float value;
          int index;
          //out = sum(abs(comb').^2)./taus;
          sums.rectify();
          sums.multiply(sums);
          //[~, m] = max(out);
          sums.getMax(&value, &index);
          writePtr = 0;
          //taus(m)/Fs*60;
          debugMessage("bpm: ", (srange[0] + index)/fs * 60);
          sums.clear();
        }
      }
      ++downsampledCount;
      if(downsampledCount == downsampleRatio)
        downsampledCount = 0;
    }

    buffer.getSamples(0).copyFrom(tmp);
  }
};
#endif // __BeatPatch_hpp__
