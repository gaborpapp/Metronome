#include "ChannelView.h"

ChannelView::ChannelView(){};

void ChannelView::setup() {
    controlpos = vec2(getWindowWidth() / 2, getWindowHeight() / 2);
    mImageChannel = loadImage( app::loadResource( RES_IMAGE ) ) ;
}

void ChannelView::draw() {
    float size = 0.5;
    
    Area rect( controlpos.x - mImageChannel.getWidth() / 2 * size, controlpos.y - mImageChannel.getHeight() / 2 * size,
              controlpos.x + mImageChannel.getWidth() * size, controlpos.y + mImageChannel.getHeight() * size );
    gl::draw( gl::Texture2d::create( mImageChannel ), rect );
}

