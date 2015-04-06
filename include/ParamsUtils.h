#pragma once

#include "Config.h"

namespace mndl { namespace params {

void showAllParams( bool visible, bool alwaysHideHelp = true );

void addParamsLayoutVars( const mndl::ConfigRef &config );
void readParamsLayout();
void writeParamsLayout();

} } // namespace mndl::params
