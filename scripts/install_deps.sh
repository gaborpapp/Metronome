#!/bin/bash

# Cinder-NI-libs from bitbucket.org
curl -L -C - -O https://bitbucket.org/gaborpapp/cinder-libs/downloads/Cinder-NI-lib-2.3.0.4.tar.gz
tar zxvf Cinder-NI-lib-2.3.0.4.tar.gz -C blocks/Cinder-NI

curl -L -C - -O https://bitbucket.org/gaborpapp/cinder-libs/downloads/Cinder-OpenCV-lib-2.4.9.zip
unzip Cinder-OpenCV-lib-2.4.9.zip -d blocks/
