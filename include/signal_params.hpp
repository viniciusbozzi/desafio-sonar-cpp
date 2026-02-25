#pragma once

#include <cstdint>

/**
 * @file signal_params.hpp
 * @brief Parâmetros do sinal compartilhados entre os três processos.
 *
 * Esta struct é mapeada diretamente em memória compartilhada POSIX
 * (Commit 2). Deve conter apenas tipos trivialmente copiáveis (POD).
 */

namespace sonar {

/// Tipos de forma de onda suportados.
enum class WaveformType : uint8_t {
    Sine     = 0,  ///< Senoide (padrão)
    Square   = 1,  ///< Onda quadrada
    Triangle = 2,  ///< Onda triangular
    Sawtooth = 3,  ///< Dente-de-serra
};

/**
 * @brief Parâmetros configuráveis do sinal gerado.
 *
 * Gravados pelo control_app e lidos pelo generator_app em tempo real.
 * Tamanho fixo intencional para mapeamento direto em shm.
 */
struct SignalParams {
    float        frequency_hz  = 440.0f;           ///< Frequência fundamental (Hz)
    float        amplitude     = 0.8f;             ///< Amplitude [0, 1]
    float        sample_rate   = 44100.0f;         ///< Taxa de amostragem (Hz)
    WaveformType waveform      = WaveformType::Sine;
    uint8_t      _padding[3]   = {};               ///< Alinhamento explícito
};

static_assert(sizeof(SignalParams) == 16, "SignalParams deve ter 16 bytes");

} // namespace sonar
