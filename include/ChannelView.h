#pragma once

#include "cinder/CinderResources.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

using namespace cinder;
using namespace ci;
using namespace ci::app;
using namespace std;

class ChannelView {
public:
    
    ChannelView();
    
    void setup();
    void draw();
    
    Channel mImageChannel;
    vec2 controlpos;
};
