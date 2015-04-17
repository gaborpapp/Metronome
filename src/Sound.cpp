#include "cinder/app/App.h"
#include "GlobalData.h"
#include "Sound.h"

using namespace ci;
using namespace ci::app;
using namespace std;

Sound::Sound(){};

void Sound::setup( audio::Context &ctx ) {
    mGen = ctx.makeNode( new audio::GenSineNode );
    mGain = ctx.makeNode( new audio::GainNode );
    mGen->setFreq( 1000 );
    mGen >> mGain >> ctx.getOutput();
    mGain->setValue( 0.0f );
    mGen->enable();

    attackVal = 0.9f;
    attackTime = 0.001f;
    decayVal = 0.5f;
    decayTime = 0.005f;
    sustainVal = 0.1f;
    sustainTime = 0.001f;
    releaseVal = 0.0f;
    releaseTime = 0.0f;
    
    now = 0;
    then = 0;
}

void Sound::update() {
    
    //  this metronome ticks every second (sample-accurate) at 1000Hz
    auto gainParam = mGain->getParam();
    
    now = audio::master()->getNumProcessedSeconds();
    if( now != then ) {
        gainParam->applyRamp( attackVal, attackTime );
        gainParam->appendRamp( decayVal, decayTime );
        gainParam->appendRamp( sustainVal, sustainTime );
        gainParam->appendRamp( releaseVal, releaseTime );
    }
    
    then = now;
}

void Sound::draw() {
}




