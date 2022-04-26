#ifndef SignalExporter_hpp
#define SignalExporter_hpp

#include <array>
#include <vector>
#include <fstream>

namespace ofxSimpleWavFile {

struct PCMData {
//    PCMData(){};
//    PCMData(int fs, int bits, std::vector<std::vector<double>> & signals):fs(fs),bits(bits),signals(signals){};
    
    int fs;
    int bits;
    std::vector<std::vector<double>> signals;

    [[nodiscard]] int getChannel() const {
        return static_cast<int>(signals.size());
    }

    [[nodiscard]] int getLength() const {
        return !signals.empty() ? static_cast<int>(signals[0].size()) : 0;
    }
};

#define RIFF_CHUNK_ID_BYTE 4
#define RIFF_CHUNK_SIZE_BYTE 4
#define FILE_FORMAT_TYPE_BYTE 4
#define FMT_CHUNK_ID_BYTE 4
#define FMT_CHUNK_SIZE_BYTE 4
#define WAVE_FORMAT_TYPE_BYTE 2
#define CHANNEL_BYTE 2
#define SAMPLES_PER_SEC_BYTE 4
#define BYTES_PER_SEC_BYTE 4
#define BLOCK_SIZE_BYTE 2
#define BITS_PER_SAMPLE_BYTE 2
#define DATA_CHUNK_ID_BYTE 4
#define DATA_CHUNK_SIZE_BYTE 4


struct WavFileData {

private:
    // wav file data
    std::string riff_chunk_ID = {'R', 'I', 'F', 'F'};
    uint32_t riff_chunk_size;
    std::string file_format_type = {'W', 'A', 'V', 'E'};
    std::string fmt_chunk_ID = {'f', 'm', 't', ' '};
    uint32_t fmt_chunk_size;
    uint16_t wave_format_type;
    uint16_t channel;
    uint32_t samples_per_sec;
    uint32_t bytes_per_sec;
    uint16_t block_size;
uint16_t bits_per_sample;
    std::string data_chunk_ID = {'d', 'a', 't', 'a'};
    uint32_t data_chunk_size;
    
    // internal data
    std::filesystem::path file_path;
    bool is_data_set = false;
    PCMData pcm_data;

public:
    WavFileData():pcm_data(){};

    explicit WavFileData(const std::string& path ):pcm_data() {
        read(path);
    }

    explicit WavFileData(const PCMData &pcm):pcm_data() {
        pcm_data = pcm;

        riff_chunk_size = 36 + pcm.getLength() * 2 * pcm.getChannel();

        fmt_chunk_size = 16;
        wave_format_type = 1;
        channel = pcm.getChannel();
        samples_per_sec = pcm.fs;
        bytes_per_sec = pcm.fs * pcm.bits / 8;
        block_size = pcm.bits / 8;
        bits_per_sample = pcm.bits;

        data_chunk_size = pcm.getLength() * 4 * channel;

        is_data_set = true;
    }

    bool write(const std::filesystem::path &_file_path) {
        if (!is_data_set) {
            std::cerr << "data not set, can't write file." << std::endl;
            return false;
        }

        file_path = _file_path;

        try {
            ofFile file(file_path, ofFile::WriteOnly, true);

            writeHeader(file);

            if (bits_per_sample == 8) {
                writeData<char>(file);
            } else if (bits_per_sample == 16) {
                writeData<short>(file);
            } else {
                std::cerr << "bits_per_sample(" << bits_per_sample << ") is not supported." << std::endl;
                throw;
            }

            file.close();

            return true;
        } catch (...) {
            std::cerr << "write file failed." << std::endl;
            return false;
        }
    }

    bool read(const std::filesystem::path &_file_path) {
        file_path = _file_path;

        try {
            ofFile file(file_path, ofFile::ReadOnly, true);

            if (!file.is_open()) {
                std::cerr << "file open failed, check file path" << std::endl;
                throw;
            }

            readHeader(file);

            pcm_data = {
                    static_cast<int>(samples_per_sec),
                    static_cast<int>(bits_per_sample),
                    std::vector<std::vector<double>>()
            };

            pcm_data.signals.resize(channel);
            
            ofLogNotice() <<data_chunk_size / channel;

            for (auto &chan: pcm_data.signals) {
                chan.resize(static_cast<int>(data_chunk_size / channel / 4));
            }

            if (bits_per_sample == 8) {
                readData<char>(file);
            } else if (bits_per_sample == 16) {
                readData<short>(file);
            } else {
                std::cerr << "bits_per_sample(" << bits_per_sample << ") is not supported." << std::endl;
                throw;
            }

            is_data_set = true;

            file.close();

        } catch (...) {
            std::cerr << "failed to read wav file." << std::endl;
            is_data_set = false;
        }

        return is_data_set;
    }
    
    const PCMData& getPCMData() const {
        return pcm_data;
    }
    
    PCMData& getPCMData() {
        return pcm_data;
    }

private:
    void writeHeader(ofFile &file) const {
        // riff chunk header
        file.write(&riff_chunk_ID[0], RIFF_CHUNK_ID_BYTE);
        file.write(reinterpret_cast<const char *>(&riff_chunk_size), RIFF_CHUNK_SIZE_BYTE);
        file.write(&file_format_type[0], FILE_FORMAT_TYPE_BYTE);

        // fmt chunk header
        file.write(&fmt_chunk_ID[0], FMT_CHUNK_ID_BYTE);
        file.write(reinterpret_cast<const char *>(&fmt_chunk_size), FMT_CHUNK_SIZE_BYTE);

        // fmt chunk data
        file.write(reinterpret_cast<const char *>(&wave_format_type), WAVE_FORMAT_TYPE_BYTE);
        file.write(reinterpret_cast<const char *>(&channel), CHANNEL_BYTE);
        file.write(reinterpret_cast<const char *>(&samples_per_sec), SAMPLES_PER_SEC_BYTE);
        file.write(reinterpret_cast<const char *>(&bytes_per_sec), BYTES_PER_SEC_BYTE);
        file.write(reinterpret_cast<const char *>(&block_size), BLOCK_SIZE_BYTE);
        file.write(reinterpret_cast<const char *>(&bits_per_sample), BITS_PER_SAMPLE_BYTE);

        // data chunk header
        file.write(&data_chunk_ID[0], DATA_CHUNK_ID_BYTE);
        file.write(reinterpret_cast<const char *>(&data_chunk_size), DATA_CHUNK_SIZE_BYTE);
    }

    template<class T>
    void writeData(ofFile &file) const {
        short data;
        int size = sizeof(T);
        int max = 1 << (size * 8);
        double max_d = max;
        int half = max >> 1;
        double s;

        for (int i = 0; i < pcm_data.getLength(); ++i) {
            for (int j = 0; j < channel; ++j) {
                s = (pcm_data.signals[j][i] + 1.0) / 2.0 * max_d;

                if (s > max_d - 1.0) {
                    s = max_d - 1.0;
                } else if (s < 0.0) {
                    s = 0.0;
                }

                data = static_cast<T>(static_cast<int>(s + 0.5) - half);
                file.write(reinterpret_cast<const char *>(&data), size);
            }
        }
    }

    void readHeader(ofFile &file) {
        file.read(&riff_chunk_ID[0], RIFF_CHUNK_ID_BYTE);
        file.read(reinterpret_cast<char *>(&riff_chunk_size), RIFF_CHUNK_SIZE_BYTE);
        file.read(&file_format_type[0], FILE_FORMAT_TYPE_BYTE);
        file.read(&fmt_chunk_ID[0], FMT_CHUNK_ID_BYTE);
        file.read(reinterpret_cast<char *>(&fmt_chunk_size), FMT_CHUNK_SIZE_BYTE);
        file.read(reinterpret_cast<char *>(&wave_format_type), WAVE_FORMAT_TYPE_BYTE);
        file.read(reinterpret_cast<char *>(&channel), CHANNEL_BYTE);
        file.read(reinterpret_cast<char *>(&samples_per_sec), SAMPLES_PER_SEC_BYTE);
        file.read(reinterpret_cast<char *>(&bytes_per_sec), BYTES_PER_SEC_BYTE);
        file.read(reinterpret_cast<char *>(&block_size), BLOCK_SIZE_BYTE);
        file.read(reinterpret_cast<char *>(&bits_per_sample), BITS_PER_SAMPLE_BYTE);
        while(true) {
            file.read(&data_chunk_ID[0], DATA_CHUNK_ID_BYTE);
            file.read(reinterpret_cast<char *>(&data_chunk_size), DATA_CHUNK_SIZE_BYTE);

            if (data_chunk_ID == "data") break;

            file.seekg(data_chunk_size, file.cur);

            if (file.tellg() == file.end) {
                std::cerr << "file format error" << std::endl;
                throw;
            }
        }
    }

    template<class T>
    void readData(ofFile &file) {
        T data;
        int size = sizeof(T);
        double half = 1 << (size * 8 - 1);

        for (int i = 0; i < pcm_data.getLength(); ++i) {
            for (int j = 0; j < pcm_data.getChannel(); ++j) {
                file.read(reinterpret_cast<char *>(&data), size);

//            s = (double)((int)data + 32768) - 0.5;
                pcm_data.signals[j][i] = (double) data / half;
            }
        }
    }
};
}

#endif /* SignalExporter_hpp */
