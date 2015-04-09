#pragma once

#include "cinder/CinderResources.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

class ChannelView {
public:
    
    ChannelView();
    
    void setup();
    void update(std::vector<ci::vec2> cps);
    void draw();
    
    std::string getResult();
    
    ci::Channel baseChannel;
    ci::Channel customChannel;
    ci::Surface customSurface;
    
    std::vector<ci::vec2> rawControlPoints;
    std::vector<ci::vec2> mappedControlPoints;
    std::vector<float> baseColors;
    
    ci::vec2 offset;
    void transparentArea( ci::Surface *surface, ci::Area area );
	
};
