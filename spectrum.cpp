
//COMPILE WITH: g++ spectrum.cpp -o spectrum -std=c++11 `pkg-config --cflags --libs gstreamermm-1.0`

#include <gstreamermm.h>
#include <iostream>
#include <glibmm.h>
#include <iomanip>

using namespace Gst;
using Glib::RefPtr;


class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    void play(const std::string& fileName);
private:
    void init();
    bool on_bus_message(const RefPtr<Bus>&, const RefPtr<Message>& message);
    void on_decoder_pad_added(const RefPtr<Pad>& pad);
    void decode_spectrum(const RefPtr<Message> &message);
    RefPtr<Glib::MainLoop> main_loop;
    RefPtr<Pipeline> pipeline;
    RefPtr<FileSrc> filesrc;
    RefPtr<Element> decoder, audiosink, spectrum;
    int bands;
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
    bands = 11;
    filesrc = FileSrc::create();
    decoder = ElementFactory::create_element("decodebin");
    audiosink = ElementFactory::create_element("autoaudiosink");
    spectrum = ElementFactory::create_element("spectrum");

    if (!filesrc || !decoder || !audiosink || !spectrum)
    {
        throw std::runtime_error("One element couldn't be created.");
    }

    spectrum->property("post-messages",true);
    spectrum->property("bands", bands);
    spectrum->property("threshold", -80);
    spectrum->property("interval", 1000000000);

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
    Gst::ValueList mags;
    Glib::Value<float> mag;
    s.get_field("magnitude", mags);

    for (int i = 0; i < bands; i++)
    {
        mags.get(i, mag);
    }
    //std::cout << s.to_string() << std::endl << std::endl << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
      std::cout << "Usage: " << argv[0] << " <audio filename>" << std::endl;
      return 1;
    }

    init(argc, argv);
    SpectrumAnalyzer player;

    try
    {
        player.play(argv[1]);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << "Runtime error: " << err.what() << std::endl;
    }

    return 0;
}
