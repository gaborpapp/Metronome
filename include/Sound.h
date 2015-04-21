#pragma once

#include "cinder/CinderResources.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/GenNode.h"
#include "cinder/audio/GainNode.h"

class Sound {
public:
    
    Sound();
    
    void setup( ci::audio::Context &ctx );
    void update( std::vector< int > bpmVals );
    void draw();
    
    std::vector< ci::audio::GenNodeRef >    mPhasorGens;
    std::vector< ci::audio::GainNodeRef >	mGains;
};
