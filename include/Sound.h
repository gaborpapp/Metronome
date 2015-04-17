#pragma once

#include "cinder/CinderResources.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/GenNode.h"
#include "cinder/audio/GainNode.h"

class Sound {
public:
    
    Sound();
    
    void setup(ci::audio::Context &ctx);
    void update();
    void draw();
    
    ci::audio::GenNodeRef	mGen;	// Gen's generate audio signals
    ci::audio::GainNodeRef	mGain;	// Gain modifies the volume of the signal
    
    //  ADSR
    float attackVal;
    float attackTime;
    float decayVal;
    float decayTime;
    float sustainVal;
    float sustainTime;
    float releaseVal;
    float releaseTime;
    
    int now, then;

};
