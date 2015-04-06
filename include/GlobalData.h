#pragma once

#include "Config.h"

class GlobalData
{
 private:
	GlobalData() {}

 public:
	static GlobalData& get() { static GlobalData data; return data; }

	mndl::ConfigRef mConfig;
};
