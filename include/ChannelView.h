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
    void draw();
    
	ci::Channel mImageChannel;
	ci::vec2 controlpos;
};
