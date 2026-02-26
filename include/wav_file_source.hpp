#pragma once

#include "signal_source.hpp"
#include <string>
#include <fstream>
#include <vector>

namespace sonar {

/**
 * @brief Implementação de Fonte de Sinal que lê áudio de um arquivo WAV (OCP).
 *
 * Suporta arquivos WAV simples em formato PCM (16-bit, Mono ou Stereo).
 * Lê os dados do arquivo em modo stream. Quando atinge o final dos dados,
 * retorna automaticamente ao início (Loop infinito) simulando um sonar contínuo.
 */
class WavFileSource : public ISignalSource {
public:
    /**
     * @param filepath Caminho completo ou relativo para o arquivo .wav
     */
    explicit WavFileSource(const std::string& filepath);
    ~WavFileSource() override;

    // ISignalSource interface
    void        generate(std::vector<float>& buffer, std::size_t n_samples) override;
    float       sampleRate() const override;
    const char* name()       const override { return "WavFileSource"; }

private:
    bool parseHeader();
    
    std::string   _filepath;
    std::ifstream _file;
    float         _sample_rate = 44100.0f;
    uint32_t      _data_start_pos = 0;
    uint32_t      _data_size = 0;
    uint16_t      _num_channels = 1;
    uint16_t      _bits_per_sample = 16;
    bool          _valid = false;
};

} // namespace sonar
