#include "cinder/app/App.h"
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
    customSurface = loadImage( loadResource( CA_IMAGE ))  ;
    transparentArea( &customSurface, Area( 0, 0, customSurface.getWidth(), customSurface.getHeight() ) );
    
    //manipulateChannel( &baseChannel, Area( 0, 0, baseChannel.getWidth(), baseChannel.getHeight() ) );
}

void ChannelView::update( const vector<ivec2> &cps ) {
    controlPoints = cps;
}

void ChannelView::draw() {
    // todo: add color values of each customChannel & store them in baseColors vector
    // keep in mind x/y offset of customChannels
    
    //...
    
    //  showing enlarged images to see what's happening
    float size = 5;
    
    Area bRect( offset.x + baseChannel.getWidth() * size, offset.y + baseChannel.getHeight() * size, offset.x + baseChannel.getWidth() * size * 2, offset.y + baseChannel.getHeight() * size * 2 );
    gl::draw( gl::Texture2d::create( baseChannel ), bRect );

    for( auto p : controlPoints ) {
        
        //  map values between 0 - 1 for displaying
        GlobalData &gd = GlobalData::get();
        vec2 mapPt = vec2( remap( p.x, 0, gd.gridSize, 0, 1 ), remap( p.y, 0, gd.gridSize, 0, 1 ) );
        
        Area cRect( ( offset.x + mapPt.x * bRect.getWidth() ) , ( offset.y + mapPt.y * bRect.getHeight() ) ,
                    ( offset.x + mapPt.x * bRect.getWidth() ) + customChannel.getWidth() * size,     ( offset.y + mapPt.y * bRect.getHeight() ) + customChannel.getHeight() * size );
        
        gl::enableAlphaBlending();
        gl::draw( gl::Texture2d::create( customSurface ), cRect );
    }
}

float ChannelView::remap(float val, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * ((val - inMin) / (inMax - inMin));
}

void ChannelView::transparentArea( Surface *surface, Area area ) {
    Surface::Iter iter = surface->getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            iter.a() = 100;
        }
    }
}

void ChannelView::manipulateChannel( Channel32f *channel, Area area ) {
    Channel32f::Iter iter = channel->getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            iter.v() = rnd.nextFloat();
        }
    }
}

string ChannelView::getRoundedChannelPixels() {
    string roundedChannelPixels = "";
    Area area( 0, 0, customChannel.getWidth(), customChannel.getHeight() );
    Channel::Iter iter = customChannel.getIter( area );
    while( iter.line() ) {
        roundedChannelPixels.append("\n");
        while( iter.pixel() ) {
            int baseValue = iter.v() / 10;
            roundedChannelPixels.append(to_string(baseValue) + " ");
        }
    }
    return roundedChannelPixels;
}

string ChannelView::getResult() {
    string baseChannelGrayValues = "";
    
    Area area( 0, 0, baseChannel.getWidth(), baseChannel.getHeight() );
    Channel32f::Iter iter = baseChannel.getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            float baseValue = iter.v();
            baseChannelGrayValues.append(to_string(baseValue) + " ");
        }
    }
    
    return baseChannelGrayValues;
}


