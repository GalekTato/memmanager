#!/bin/bash

echo "Building GUI..."
cd gui
qmake6 memmanager_gui.pro
make -j$(nproc)

echo "Building Network (Server and Client)..."
cd ../network
qmake6 memmanager_network.pro
make -j$(nproc)
qmake6 memclient.pro
make -j$(nproc)

echo "Build complete."
