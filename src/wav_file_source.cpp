#include "wav_file_source.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace sonar {

// Estrutura simplificada do cabeçalho WAV Canônico (44 bytes)
#pragma pack(push, 1)
struct WavHeader {
    char     riff_tag[4];       // "RIFF"
    uint32_t riff_size;
    char     wave_tag[4];       // "WAVE"
    char     fmt_tag[4];        // "fmt "
    uint32_t fmt_size;
    uint16_t audio_format;      // 1 = PCM sem compactação
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_tag[4];       // "data"
    uint32_t data_size;
};
#pragma pack(pop)

WavFileSource::WavFileSource(const std::string& filepath) : _filepath(filepath) {
    _file.open(filepath, std::ios::binary);
    if (!_file.is_open()) {
        std::cerr << "[WavFileSource] Erro ao abrir o arquivo: " << filepath << "\n";
        return;
    }
    
    _valid = parseHeader();
    if (!_valid) {
        std::cerr << "[WavFileSource] Arquivo WAV invalido ou nao suportado. (Use PCM 16-bits canônico)\n";
    }
}

WavFileSource::~WavFileSource() {
    if (_file.is_open()) {
        _file.close();
    }
}

bool WavFileSource::parseHeader() {
    WavHeader header;
    if (!_file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader))) {
        return false;
    }
    
    if (std::strncmp(header.riff_tag, "RIFF", 4) != 0 || 
        std::strncmp(header.wave_tag, "WAVE", 4) != 0) {
        return false;
    }
    
    if (header.audio_format != 1) { // 1 == PCM Incomprimido
        std::cerr << "[WavFileSource] Suporta apenas arquivos WAV PCM.\n";
        return false;
    }
    
    _sample_rate = static_cast<float>(header.sample_rate);
    _num_channels = header.num_channels;
    _bits_per_sample = header.bits_per_sample;
    
    // Procura por chunks dinâmicos caso o "data" nao esteja na sub-posicao 36 fixa
    if (std::strncmp(header.data_tag, "data", 4) != 0) {
       // Wav chunking can ser complexo. Para fins didáticos do desafio,
       // vamos forçar leitura de um WaveHeader perfeitamente empacotado.
       std::cerr << "[WavFileSource] Aviso: chunking dinamico ignorado.\n";
    }
    
    _data_size = header.data_size;
    _data_start_pos = _file.tellg();
    return true;
}

void WavFileSource::generate(std::vector<float>& buffer, std::size_t n_samples) {
    buffer.resize(n_samples);
    
    if (!_valid || !_file.is_open()) {
        // Envia silencio (0.0f) em caso de erro no arquivo
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        return;
    }

    // Le as amostras do disco decodificando PCM
    for (std::size_t i = 0; i < n_samples; ++i) {
        // Se a leitura chegou ao final da secao 'data' do arquivo, faça loop
        if (_file.eof() || _file.tellg() >= (_data_start_pos + _data_size)) {
            _file.clear(); // Reseta flag EOF do std::ifstream
            _file.seekg(_data_start_pos, std::ios::beg); // Rebubina a "Fita" K7
        }

        if (_bits_per_sample == 16) {
            int16_t sample_int = 0;
            _file.read(reinterpret_cast<char*>(&sample_int), sizeof(int16_t));
            
            // Normaliza o int16 (-32768 a 32767) para o nosso padrão de float do sonar (-1.0 a 1.0)
            buffer[i] = static_cast<float>(sample_int) / 32768.0f;
            
            // Se for arquivo estéreo, pulamos o canal auditivo DIREITO (Avança ponteiro) 
            // no sonar queremos um mix simples Mono.
            if (_num_channels == 2) {
                _file.ignore(sizeof(int16_t));
            }
        } 
        else if (_bits_per_sample == 8) {
            uint8_t sample_int = 0;
            _file.read(reinterpret_cast<char*>(&sample_int), sizeof(uint8_t));
            // WAV de 8bits tem offset sem sinal 0~255
            buffer[i] = (static_cast<float>(sample_int) - 128.0f) / 128.0f;
            if (_num_channels == 2) _file.ignore(sizeof(uint8_t));
        }
        else {
            // Outros sample bits ex. 24 ou 32 bits nao suportados na classe basica 
            buffer[i] = 0.0f;
        }
    }
}

float WavFileSource::sampleRate() const {
    return _sample_rate;
}

} // namespace sonar
