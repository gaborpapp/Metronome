#include "cinder/app/App.h"
#include "cinder/ip/Fill.h"
#include "GlobalData.h"
#include "ChannelView.h"

using namespace ci;
using namespace ci::app;
using namespace std;

ChannelView::ChannelView(){};

void ChannelView::setup() {
    offset = vec2(getWindowWidth() / 4, getWindowHeight() / 2 ) ;
    customChannel = Surface8u( loadImage( loadResource( C_IMAGE ) ) ).getChannelRed();
    baseChannel = loadImage( loadResource( BASE_IMAGE ) ) ;
    bpmChannel = loadImage( loadResource( BASE_IMAGE ) ) ;
    
    GlobalData &gd = GlobalData::get();
    mParams = params::InterfaceGl::create("BPM", ivec2(200,400));
    mParams->setPosition(ivec2(16,332));
    for( size_t i = 0; i < mBpmValues.size(); i++) {
        mParams->addParam( "Bpm #" + to_string(i), &mBpmValues[i]).min( 5 ).max( 220 );
        gd.mConfig->addVar( "ChannelView.Bpm" + to_string(i), &mBpmValues[i], mBpmValues[i] );
    }
}

void ChannelView::update( const vector<ivec2> &cps ) {
    controlPoints = cps;
    if( cps.size() > 0 ) {
		ip::fill( &baseChannel, 0.0f );
		ip::fill( &bpmChannel, 0.0f );

        //  add customChannel colors to baseChannel
        for( auto tp : controlPoints ) {
            vec2 p = vec2(baseChannel.getWidth() - tp.x, baseChannel.getHeight() - tp.y );
            Area customArea(p.x, p.y, p.x + baseChannel.getWidth(), p.y + baseChannel.getHeight() );
            Channel::Iter iter = customChannel.getIter( customArea );
        
            while( iter.line() ) {
                while( iter.pixel() ) {
                    ivec2 setpos(iter.x() - p.x , iter.y() - p.y ) ;
                    baseChannel.setValue( setpos, baseChannel.getValue(setpos) + iter.v() );
                }
            }
        }
    
        //  add corresponding bpm values to bpmChannel
        for( auto tp : controlPoints ) {
            vec2 p = vec2(bpmChannel.getWidth() - tp.x, bpmChannel.getHeight() - tp.y );
            Area customArea(p.x, p.y, p.x + bpmChannel.getWidth(), p.y + bpmChannel.getHeight() );
            Channel::Iter iter = customChannel.getIter( customArea );
        
            while( iter.line() ) {
                while( iter.pixel() ) {
                    ivec2 setpos(iter.x() - p.x , iter.y() - p.y ) ;
                    bpmChannel.setValue( setpos, bpmChannel.getValue(setpos) + mBpmValues[iter.v()/10 - 1] );
                }
            }
        }
    } else {
		ip::fill( &bpmChannel, 60.0f );
    }
}

float ChannelView::remap( float val, float inMin, float inMax, float outMin, float outMax ) {
    return outMin + ( outMax - outMin ) * ( ( val - inMin ) / ( inMax - inMin ) );
}

string ChannelView::getRawResultAsString() {
    string baseChannelGrayValues = "";
    
    Area area( 0, 0, baseChannel.getWidth(), baseChannel.getHeight() );
    Channel32f::Iter iter = baseChannel.getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            float baseValue = iter.v() / 10;
            baseChannelGrayValues.append( to_string( ( int )baseValue ) + " " );
        }
    }
    return baseChannelGrayValues;
}

string ChannelView::getBpmResultAsString() {
    string bpmValues = "";
    
    Area area( 0, 0, bpmChannel.getWidth(), bpmChannel.getHeight() );
    Channel32f::Iter iter = bpmChannel.getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            int bpmValue = iter.v();
            bpmValues.append( to_string( bpmValue ) + " " );
        }
    }
    return bpmValues;
}


std::vector< int > ChannelView::getRawResultAsVector() {
    std::vector< int > rawResult;
    Area area( 0, 0, baseChannel.getWidth(), baseChannel.getHeight() );
    Channel32f::Iter iter = baseChannel.getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            float baseValue = iter.v() / 10;
            rawResult.push_back( ( int )baseValue );
        }
    }
    return rawResult;
}

std::vector< int > ChannelView::getBpmResultAsVector() {
    std::vector< int > bpmResult;
    
    Area area( 0, 0, bpmChannel.getWidth(), bpmChannel.getHeight() );
    Channel32f::Iter iter = bpmChannel.getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            float val = iter.v();
            bpmResult.push_back( ( int )val );
        }
    }
    return bpmResult;
}



