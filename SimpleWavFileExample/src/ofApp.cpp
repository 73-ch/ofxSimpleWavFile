#include "ofMain.h"
#include "ofxSimpleWavFile.h"

class ofApp : public ofBaseApp{

    public:
    void setup() {
        const int fs = 48000;
        const int size = fs * 3;
        const int fade = 0.1;
        std::vector<double> signal(size);

        for (int i = 0; i < signal.size(); i++) {
            signal[i] = sin((double) i / fs * 100 * 2.0 * M_PI);
            signal[i] *= fmin((double)i / fs / fade, 1.0) * fmin((double)(size - i) / fs / fade, 1.0);
        }

        ofxSimpleWavFile::PCMData pcm = {
            fs,
            16,
            std::vector<std::vector<double>>{signal}
        };

        ofxSimpleWavFile::WavFileData wav_data(pcm);

        wav_data.write("440.wav");
        
        ofxSimpleWavFile::WavFileData wav_data2("100.wav");
        
        auto read_data = wav_data2.getPCMData();
        
        ofLogNotice() << "Channel: " << read_data.getChannel();
        ofLogNotice() << "Length: " << read_data.getLength();
        wav_data2.write("100-2.wav");
        
        
    }
    void update() {}
    void draw(){}
};


int main( ){
    ofSetupOpenGL(1024,768,OF_WINDOW);            // <-------- setup the GL context

    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());

}
