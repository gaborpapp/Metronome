#pragma once

#include "cinder/CinderResources.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/GenNode.h"
#include "cinder/audio/GainNode.h"

class Sound {
public:
    
    Sound();
    
    void setup( ci::audio::Context &ctx );
    void update( float bpm );
    void draw();
    
    ci::audio::GenNodeRef   mPhasorGen;
    ci::audio::GainNodeRef	mGain;	
};
