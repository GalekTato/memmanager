#!/bin/bash

echo "Building GUI..."
cd gui
qmake6 memmanager_gui.pro
make -j$(nproc)
cd ..

echo "Building Network components..."
cd network
qmake6 memmanager_network.pro
make -j$(nproc)
cd ..

echo "Build complete."
echo "Binaries located at:"
echo "  [OK] GUI compilada:     gui/memmanager_gui"
echo "  [OK] Servidor compilado: network/memserver"
echo "  [OK] Cliente compilado:  network/memclient"
echo "  [OK] Monitor compilado:  network/memmonitor"