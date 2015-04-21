#include "cinder/app/App.h"
#include "GlobalData.h"
#include "Sound.h"

using namespace ci;
using namespace ci::app;
using namespace std;

Sound::Sound(){};

void Sound::setup( audio::Context &ctx ) {

    GlobalData &gd = GlobalData::get();
    int num = gd.mGridSize * gd.mGridSize;
    
    for( int i = 0; i < num; i++ ) {
        ci::audio::GenNodeRef gen;
        ci::audio::GainNodeRef gain;
        
        gen = ctx.makeNode( new audio::GenPhasorNode );
        gen->setFreq( 1 );
        
        gain = ctx.makeNode( new audio::GainNode );
        
        gen >> gain >> ctx.getOutput();
        gen->enable();
        
        gain->setValue( 0.01f );
        gain->enable();
        
        mPhasorGens.push_back( gen );
        mGains.push_back( gain );
    }
}

void Sound::update( vector< int > bpmVals ) {
    for( int i = 0; i < mPhasorGens.size(); i++ ) {
        if( i < bpmVals.size() ) {
            float hertz = 1000 / ( 60000 / ( float )bpmVals[i] );
            mPhasorGens[i]->setFreq( hertz );
        }
    }
}

void Sound::draw() {
}




