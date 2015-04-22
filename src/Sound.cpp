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
        ci::audio::FilterBandPassNodeRef filt;
        ci::audio::GainNodeRef gain;
        
        gen = ctx.makeNode( new audio::GenPhasorNode );
        gen->setFreq( 1 );
        
        filt = ctx.makeNode( new audio::FilterBandPassNode );
        filt->setQ( 4 );
        filt->setGain( 1.0f );
        filt->enable();
        
        gain = ctx.makeNode( new audio::GainNode );
        
        gen >> filt >> gain >> ctx.getOutput();
        gen->enable();
        
        gain->setValue( 0.1f );
        gain->enable();
        
        mPhasorGens.push_back( gen );
        mGains.push_back( gain );
        mFilters.push_back( filt );
    }
}

void Sound::update( vector< int > bpmVals ) {
    for( int i = 0; i < mPhasorGens.size(); i++ ) {
        if( i < bpmVals.size() ) {
            float hertz = 1000 / ( 60000 / ( float )bpmVals[i] );
            mPhasorGens[i]->setFreq( hertz );
            mFilters[i]->setCenterFreq( bpmVals[i] * 40 );
        }
    }
}

void Sound::draw() {
}

void Sound::sync() {
    for( auto gen : mPhasorGens ) {
        gen->setPhase(0);
    }
}




