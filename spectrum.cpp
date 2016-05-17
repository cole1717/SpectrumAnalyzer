
//COMPILE WITH: g++ spectrum.cpp -o spectrum -std=c++11 -L/home/pi/git/rpi-rgb-led-matrix-master/lib -lrgbmatrix -lrt -lm -lpthread `pkg-config --cflags --libs gstreamermm-1.0`
//sudo chown root spectrum
//sudo chmod u+s spectrum

//LED: g++ in/out FILES -L/home/pi/git/rpi-rgb-led-matrix-master/lib -lrgbmatrix -lrt -lm -lpthread -std=c++11

#define _POSIX_C_SOURCE 200112L

#include </home/pi/git/rpi-rgb-led-matrix-master/include/led-matrix.h>
#include <gstreamermm.h>
#include <iostream>
#include <glibmm.h>
#include <iomanip>
#include <unistd.h>

//NEED THESE?
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

using namespace Gst;
using namespace rgb_matrix;
using Glib::RefPtr;


class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    void addCanvas(Canvas *can);
    void play(const std::string& fileName);
    void setBepis(Canvas *canvas);
private:
    void init();
    bool on_bus_message(const RefPtr<Bus>&, const RefPtr<Message>& message);
    void on_decoder_pad_added(const RefPtr<Pad>& pad);
    void decode_spectrum(const RefPtr<Message> &message);
    void drawMag(float height, int x);
    void drawStraightLine(Canvas *canvas, int x1, int y1, int x2, int y2, int R, int G, int B);
    RefPtr<Glib::MainLoop> main_loop;
    RefPtr<Pipeline> pipeline;
    RefPtr<FileSrc> filesrc;
    RefPtr<Element> decoder, audiosink, spectrum;
    Canvas *canvas;
    int bands;
    float factor;
    int threshold;
};

SpectrumAnalyzer::SpectrumAnalyzer()
{
    main_loop = Glib::MainLoop::create();
    pipeline = Pipeline::create();
    pipeline->get_bus()->add_watch(sigc::mem_fun(*this, &SpectrumAnalyzer::on_bus_message));
}

void SpectrumAnalyzer::play(const std::string &fileName)
{
    init();
    filesrc->property_location() = fileName;
    pipeline->set_state(STATE_PLAYING);
    main_loop->run();
    pipeline->set_state(STATE_NULL);
}

void SpectrumAnalyzer::init()
{
    bands = 36;
    threshold = -80;
    factor = abs(threshold / 32);

    filesrc = FileSrc::create();
    decoder = ElementFactory::create_element("decodebin");
    audiosink = ElementFactory::create_element("pulsesink");
    spectrum = ElementFactory::create_element("spectrum");

    if (!filesrc || !decoder || !audiosink || !spectrum)
    {
        throw std::runtime_error("One element couldn't be created.");
    }

    spectrum->property("post-messages",true);
    spectrum->property("bands", bands);
    spectrum->property("threshold", threshold);
    spectrum->property("interval", 50000000);

    //Glib::ustring name = "snd_usb_audio";

    //audiosink->property("device", name);

    pipeline->add(filesrc)->add(decoder)->add(audiosink)->add(spectrum);
    decoder->signal_pad_added().connect(sigc::mem_fun(*this, &SpectrumAnalyzer::on_decoder_pad_added));

    filesrc->link(decoder);
    spectrum->link(audiosink);
}

bool SpectrumAnalyzer::on_bus_message(const RefPtr<Bus> &, const RefPtr<Message> &message)
{
    switch(message->get_message_type())
    {
    case Gst::MESSAGE_ELEMENT:
        decode_spectrum(message); //Check if spectrum?
        break;
    case Gst::MESSAGE_EOS:
        std::cout << "\nEnd of stream!" << std::endl;
        main_loop->quit();
        return false;
    case Gst::MESSAGE_ERROR:
        std::cerr << "Error. " << RefPtr<MessageError>::cast_static(message)->parse_debug() << std::endl;
        main_loop->quit();
        return false;
    default:
        break;
    }

    return true;
}

void SpectrumAnalyzer::on_decoder_pad_added(const RefPtr<Pad>& pad)
{
    try
    {
        spectrum->set_state(STATE_PLAYING);
        pad->link(spectrum->get_static_pad("sink"));
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << "Cannot link to decoder. Error: " << err.what() << std::endl;
    }
}

void SpectrumAnalyzer::decode_spectrum(const RefPtr<Message> &message)
{
    Gst::Structure s = message->get_structure();
    float height;
    float magnitude;
    float fakeFactor;
    Gst::ValueList mags;
    s.get_field("magnitude", mags);

    canvas->Clear();

    for (int i = bands - 4; i > 0; i--)
    {
        Glib::Value<float> mag;
        mags.get(i - 1, mag);
        magnitude = floor (abs(threshold) + mag.get());

        if (magnitude < 20) fakeFactor = factor * 0.7;
        else if (magnitude >= 15 && magnitude < 35) fakeFactor = factor; //SHOULD DO A RATIO NOT SUBTRACTION
        else fakeFactor = factor * 1.2;

        height = floor(magnitude / fakeFactor);
        drawMag(height, 32 - i);//(3 * i) - 3);
        drawMag(height, 32 - i);//(3 * i) - 2);
    }
    //std::cout << s.to_string() << std::endl << std::endl << std::endl;
}

void SpectrumAnalyzer::drawMag(float height, int x)
{
    int h = int(height);
    for (int i = 0; i < h; i++)
    {
        if (i < 9)
            canvas->SetPixel(x, i, 0, 200, 0);
        else if (i >= 9 && i < 16)
            canvas->SetPixel(x, i, 255, 225, 0);
        else
            canvas->SetPixel(x, i, 255, 0, 0);
    }
}

void SpectrumAnalyzer::addCanvas(Canvas *can)
{
    canvas = can;
}

void SpectrumAnalyzer::drawStraightLine(Canvas *canvas, int x1, int y1, int x2, int y2, int R, int G, int B) {
    //correct coordinates
    y1 = 31 - y1;
    y2 = 31 - y2;
    //int temp = 0;
    int yDiff = abs(y2 - y1);
    int xDiff = abs(x2 - x1);

    //draw the line
    if (x1 == x2) {
        for (int i = 0; i <= yDiff; i++) {
            canvas->SetPixel(x1, y1 - i, R, G, B);
            //usleep(100);
        }
    }
    else if (y1 == y2) {
        for (int i = 0; i <= xDiff; i++) {
            canvas->SetPixel(x1 + i, y1, R, G, B);
            //usleep(100);
        }
    }
    else {
        std::cout << "straight lines only please." << std::endl;
    }
}

void SpectrumAnalyzer::setBepis(Canvas *canvas) {
    //B
    drawStraightLine(canvas,3,13,3,23,255,0,0);
    drawStraightLine(canvas,3,19,3,23,255,0,0);
    drawStraightLine(canvas,3,13,3,17,255,0,0);
    drawStraightLine(canvas,3,18,7,18,255,0,0);
    drawStraightLine(canvas,3,13,8,13,255,0,0);
    drawStraightLine(canvas,8,13,8,17,255,0,0);
    drawStraightLine(canvas,8,19,8,23,255,0,0);
    drawStraightLine(canvas,3,23,8,23,255,0,0);
    //E
    drawStraightLine(canvas,10,13,10,23,0,255,0);
    drawStraightLine(canvas,10,13,15,13,0,255,0);
    drawStraightLine(canvas,10,18,15,18,0,255,0);
    drawStraightLine(canvas,10,23,15,23,0,255,0);
    //P
    drawStraightLine(canvas,17,13,17,23,0,0,255);
    drawStraightLine(canvas,21,18,21,23,0,0,255);
    drawStraightLine(canvas,17,23,21,23,0,0,255);
    drawStraightLine(canvas,17,18,21,18,0,0,255);
    //I
    drawStraightLine(canvas,23,13,23,23,0,255,0);
    //S
    drawStraightLine(canvas,25,13,29,13,255,0,0);
    drawStraightLine(canvas,25,18,29,18,255,0,0);
    drawStraightLine(canvas,25,23,29,23,255,0,0);
    drawStraightLine(canvas,25,18,25,23,255,0,0);
    drawStraightLine(canvas,29,13,29,18,255,0,0);
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
      std::cout << "Usage: " << argv[0] << " <audio filename>" << std::endl;
      return 1;
    }

    uid_t real = getuid();

    GPIO io;
    if (!io.Init())
      return 1;

    int rows = 32;
    int chain = 1;
    int parallel = 1;
    RGBMatrix *canvas = new RGBMatrix(&io, rows, chain, parallel);
    canvas->SetBrightness(60);

    seteuid(real);

    init(argc, argv);
    SpectrumAnalyzer player;
    player.addCanvas(canvas);
    //player.setBepis(canvas);

    try
    {
        player.play(argv[1]);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << "Runtime error: " << err.what() << std::endl;
    }

    canvas->Clear();
    delete canvas;

    return 0;
}
