# Metronome

Controller application for Abris Gryllus's digital remake of Ligeti's Poème symphonique.

## Cloning

	git clone git@github.com:gaborpapp/Metronome.git
	git submodule update --init --recursive

Download and extract binary libraries:

	./scripts/install_deps.sh

## Building on OS X with xcode

Build Cinder, either by opening cinder/xcode/cinder.xcodeproj or from the
command line:

	cd cinder/xcode
	xcrun xcodebuild -project cinder.xcodeproj -target cinder -configuration Release
	xcrun xcodebuild -project cinder.xcodeproj -target cinder -configuration Debug

Build the application by opening xcode/Metronome.xcodeproj.
