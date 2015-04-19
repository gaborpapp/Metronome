#pragma once

#include "cinder/CinderResources.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"
#include "Resources.h"

class ChannelView {
public:
    
    ChannelView();
    
    void setup();
	//! Updates the blob grid coordinates in \a cps. Each blob position is sent as an integer coordinate in the grid.
	void update(const std::vector<ci::ivec2> &cps);
    void draw();
    
    ci::Rand rnd;
    
    std::string getResult();
    std::string getRoundedChannelPixels();
    
    ci::Channel32f baseChannel;
    ci::Channel customChannel;
    ci::Surface customSurface;
    
    std::vector<ci::ivec2> controlPoints;
    std::vector<float> baseColors;
    
    ci::vec2 offset;
    void transparentArea( ci::Surface *surface, ci::Area area );
    void manipulateChannel( ci::Channel32f *channel, ci::Area area );
    
    float remap(float val, float inMin, float inMax, float outMin, float outMax);
    
    ci::params::InterfaceGlRef mParams;
    
    std::vector< int > mBpmValues = { 5, 10, 20, 25 }; // TODO: fill with real values later
};
