#include "cinder/app/App.h"

#include "ChannelView.h"

using namespace ci;
using namespace ci::app;
using namespace std;

ChannelView::ChannelView(){};

void ChannelView::setup() {
    offset = vec2(getWindowWidth() / 4, getWindowHeight() / 2 ) ;
    customChannel = loadImage( loadResource( C_IMAGE ) ) ;
    baseChannel = loadImage( loadResource( BASE_IMAGE ) ) ;
    customSurface = loadImage( loadResource( CA_IMAGE ))  ;
    transparentArea( &customSurface, Area( 0, 0, customSurface.getWidth(), customSurface.getHeight() ) );
}

void ChannelView::update( vector<vec2> cps ) {
    vector<vec2>tmpPts = cps;
    rawControlPoints = cps;
    
    // map incoming coordinates from 0 - 1 to 0  - 10
    for( auto &p : tmpPts) {
        p.x =  p.x * 10;
        p.y =  p.y * 10;
    }
    
    mappedControlPoints = tmpPts;
}

void ChannelView::draw() {
    // todo: add color values of each customChannel & store them in baseColors vector
    // keep in mind x/y offset of customChannels
    
    //...
    
    //  showing enlarged images to see what's happening
    float size = 5;
    
    Area bRect( offset.x + baseChannel.getWidth() * size, offset.y + baseChannel.getHeight() * size, offset.x + baseChannel.getWidth() * size * 2, offset.y + baseChannel.getHeight() * size * 2 );
    gl::draw( gl::Texture2d::create( baseChannel ), bRect );
    
    for( auto p : rawControlPoints ) {
        Area cRect( ( offset.x + p.x * bRect.getWidth() ) , ( offset.y + p.y * bRect.getHeight() ) ,
                    ( offset.x + p.x * bRect.getWidth() ) + customChannel.getWidth() * size,     ( offset.y + p.y * bRect.getHeight() ) + customChannel.getHeight() * size );
        
        
        gl::enableAlphaBlending();
        gl::draw( gl::Texture2d::create( customSurface ), cRect );
    }
}

void ChannelView::transparentArea( Surface *surface, Area area ) {
    Surface::Iter iter = surface->getIter( area );
    while( iter.line() ) {
        while( iter.pixel() ) {
            iter.a() = 100;
        }
    }
}


string ChannelView::getResult() {
    string result = "";
    
    //  todo: map baseColor values to bpm values & convert to string
    
    return result;
}

