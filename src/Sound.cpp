#include "cinder/app/App.h"
#include "GlobalData.h"
#include "Sound.h"

using namespace ci;
using namespace ci::app;
using namespace std;

Sound::Sound(){};

void Sound::setup( audio::Context &ctx ) {
    mGain = ctx.makeNode( new audio::GainNode );
    mPhasorGen = ctx.makeNode( new audio::GenPhasorNode );
    mPhasorGen->setFreq( 1 );
    mPhasorGen >> mGain >> ctx.getOutput();
    mPhasorGen->enable();
}

void Sound::update( float bpm) {
    float hertz = 1000 / ( 60000 / bpm );
    mPhasorGen->setFreq( hertz );
}

void Sound::draw() {
}




