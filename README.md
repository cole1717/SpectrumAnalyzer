# SpectrumAnalyzer
The pi analyzes audio in real time from an auxiliary cord input and outputs through speakers as well as a visual representation of the audio via RGB LED matrix. 

Requires github.com/hzeller/rpi-rgb-led-matrix and gstreamer-1.0 on pi. 

compile with:

g++ spectrum.cpp -o spectrum -std=c++11 -L/home/pi/git/rpi-rgb-led-matrix-master/lib -lrgbmatrix -lrt -lm -lpthread \`pkg-config --cflags --libs gstreamermm-1.0\`

sudo chown root spectrum

sudo chmod u+s spectrum


Video demo: https://drive.google.com/open?id=0B0yjZfQZ-Uw3Wl9jUi1xOV9YbEk
