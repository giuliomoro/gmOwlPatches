#ifndef __DbapExamplePatch_hpp__
#define __DbapExamplePatch_hpp__

#include "Patch.h"
#include "Dbap.hpp"

class DbapExamplePatch : public Patch {
public:
  static constexpr unsigned int numSources = 2;
  static constexpr unsigned int numSpeakers = 4;
  PatchParameterId outputParameters[numSources][numSpeakers] = {
    { PARAMETER_AA, PARAMETER_AB, PARAMETER_AC, PARAMETER_AD},
    { PARAMETER_AE, PARAMETER_AF, PARAMETER_AG, PARAMETER_AH},
    { PARAMETER_BA, PARAMETER_BB, PARAMETER_BC, PARAMETER_BD},
    { PARAMETER_BE, PARAMETER_BF, PARAMETER_BG, PARAMETER_BH},
  };
  Dbap dbap;
  // this limits the number of sources to 32 ! 
  uint32_t controlledSources;
  FloatArray speakersX;
  FloatArray speakersY;
  FloatArray gains;
  FloatArray temp0;
  FloatArray temp1;
  DbapExamplePatch():
    dbap(numSources),
    controlledSources(0)
  {
    temp0 = FloatArray::create(getBlockSize()); 
    temp1 = FloatArray::create(getBlockSize()); 
    gains = FloatArray::create(numSpeakers);
    speakersX = FloatArray::create(numSpeakers);
    speakersY = FloatArray::create(numSpeakers);
    speakersX[0] = 1;
    speakersY[0] = 1;
    speakersX[1] = -1;
    speakersY[1] = 1;
    speakersX[2] = -1;
    speakersY[2] = -1;
    speakersX[3] = 1;
    speakersY[3] = -1;
    dbap.setSpeakers(speakersX, speakersY); 
    dbap.homeSources();
    registerParameter(PARAMETER_A, "Source");
    registerParameter(PARAMETER_B, "Angle");
    registerParameter(PARAMETER_C, "Distance");
    registerParameter(PARAMETER_D, "Spread");
  }
  
  void processAudio(AudioBuffer &buffer){
    updateControlledSources(getParameterValue(PARAMETER_A));
    float angle = 360 * (0.75 - getParameterValue(PARAMETER_B));
    float distance = getParameterValue(PARAMETER_C);
    float spread = 2 * getParameterValue(PARAMETER_D);

    // spread is set globally for all sources (as it is a parameter of the room)
    dbap.setSpread(spread);
    moveSources(angle, distance);

    float source0LeftGain;
    float source1LeftGain;
    float source0RightGain;
    float source1RightGain;
    // render:
    for(unsigned int n = 0; n < numSources; ++n){
      dbap.getAmplitudes(n, gains);
      setOutputParameters(n, gains);

      // With two in/two outs, we can listen to two sources and the two front channels
      // so let's remember here the gains
      if(n == 0){
        // speaker 1 is front left
        source0LeftGain = gains[1];
        // speaker 0 is front right
        source0RightGain = gains[0];
      }
      if(n == 1){
        // speaker 1 is front left
        source1LeftGain = gains[1];
        // speaker 0 is front right
        source1RightGain = gains[0];
      }
    }
    FloatArray leftIO = buffer.getSamples(0);
    FloatArray rightIO = buffer.getSamples(1);
    // generate one test signal on left input  (alternative: use line input)
    static int count = 0;
    count += getBlockSize();
    if(count > 10000){
      leftIO.noise(); 
      if(count > 20000)
        count = 0;
    }

    // downmix two sources to stereo
    rightIO.multiply(source1LeftGain, temp0);
    rightIO.multiply(source1RightGain, temp1);
    leftIO.multiply(source0RightGain, rightIO);
    leftIO.multiply(source0LeftGain);
    leftIO.add(temp0);
    rightIO.add(temp1);
  }

  void setOutputParameters(unsigned int source, FloatArray gains){
    for(unsigned int speaker = 0; speaker < gains.getSize(); ++speaker){
      setParameterValue(outputParameters[source][speaker], gains[speaker]);
    }
  }

  void moveSources(float angle, float distance){
    for(unsigned int n = 0; n < numSources; ++n){
      if(isSourceControlled(n)){
        dbap.setSourcePositionPolar(n, angle, distance);
      }
    }
  }

  void updateControlledSources(float par){
    static float previousPar = -1;
    const int numStates = 6;
    if(fabsf(par - previousPar) > 0.005){
      previousPar = par;
      unsigned int state = (int)(par * numStates);
      resetControlledSources();
      switch(state){
      case numStates - 2:
      // The last but one state controls the first half sources
        for(unsigned int n = 0; n < numSources / 2; ++n){
          controlSource(n);
        }
        debugMessage("Controlling lower half");
      break;
      case numStates - 1:
      // The last but one state controls the second half sources
        for(unsigned int n =  numSources / 2; n < numSources; ++n){
          controlSource(n);
        }
        debugMessage("Controlling upper half");
      break;
      default:
      // most states would control the sources with the
      // corresponding number 
        controlSource(state);
        debugMessage("Controlling", (int)state);
      }
    }
  }

  void resetControlledSources(){
    controlledSources = 0;
  }

  void controlSource(unsigned int newSource){
    controlledSources |= 1 << newSource;
  }

  bool isSourceControlled(unsigned int source){
    return controlledSources & (1 << source);
  }

  void uncontrolSource(unsigned int newSource){
    controlledSources &= ~(1 << newSource);
  }
};

#endif // __DbapExamplePatch_hpp__
