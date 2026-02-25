#pragma once

#include "signal_source.hpp"
#include "signal_params.hpp"
#include <atomic>
#include <cmath>

/**
 * @file sine_generator.hpp
 * @brief Gerador de sinal senoidal contínuo (e outras formas de onda).
 *
 * Implementa ISignalSource de forma thread-safe.
 * Mantém estado de fase entre chamadas successivas para garantir
 * continuidade do sinal (sem descontinuidade ao cruzar fronteiras de buffer).
 */

namespace sonar {

/**
 * @brief Gerador de sinal baseado em fórmulas matemáticas.
 *
 * Suporta senoide, onda quadrada, triangular e dente-de-serra,
 * configuráveis em tempo real via setParams() (thread-safe).
 *
 * Uso:
 * @code
 *   SineGenerator gen;
 *   std::vector<float> buf;
 *   gen.generate(buf, 1024);
 * @endcode
 */
class SineGenerator : public ISignalSource {
public:
    explicit SineGenerator(const SignalParams& params = SignalParams{});

    // ISignalSource interface
    void        generate(std::vector<float>& buffer, std::size_t n_samples) override;
    float       sampleRate() const override;
    const char* name()       const override { return "SineGenerator"; }

    /**
     * @brief Atualiza os parâmetros do gerador de forma thread-safe.
     * Pode ser chamado de qualquer thread enquanto generate() roda em outra.
     */
    void setParams(const SignalParams& params);

    /// Retorna cópia dos parâmetros atuais.
    SignalParams getParams() const;

private:
    /// Gera uma única amostra para o instante de fase `phase` [0, 2π).
    float sample(float phase, WaveformType type, float amplitude) const;

    // Os parâmetros são atualizáveis atomicamente.
    // Usamos um mutex leve (spin seria alternativa em RT).
    mutable std::atomic_flag _lock = ATOMIC_FLAG_INIT;
    SignalParams _params;

    /// Fase acumulada [0, 2π) — preservada entre chamadas.
    double _phase = 0.0;
};

} // namespace sonar
