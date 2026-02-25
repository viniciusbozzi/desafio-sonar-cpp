#include "sine_generator.hpp"
#include <cmath>
#include <stdexcept>

namespace sonar {

// ──────────────────────────────────────────────────────────────────────────────
// Constantes
// ──────────────────────────────────────────────────────────────────────────────
static constexpr double TWO_PI = 2.0 * M_PI;

// ──────────────────────────────────────────────────────────────────────────────
// Construtor
// ──────────────────────────────────────────────────────────────────────────────
SineGenerator::SineGenerator(const SignalParams& params)
    : _params(params), _phase(0.0)
{}

// ──────────────────────────────────────────────────────────────────────────────
// generate()
// ──────────────────────────────────────────────────────────────────────────────
void SineGenerator::generate(std::vector<float>& buffer, std::size_t n_samples) {
    buffer.resize(n_samples);

    // Adquire cópia local dos parâmetros (lock-free via atomic_flag spinlock)
    while (_lock.test_and_set(std::memory_order_acquire)) { /* spin */ }
    SignalParams p = _params;
    _lock.clear(std::memory_order_release);

    if (p.sample_rate <= 0.0f || p.frequency_hz <= 0.0f) {
        throw std::invalid_argument("sample_rate e frequency_hz devem ser positivos");
    }

    // Incremento de fase por amostra
    const double phase_increment = TWO_PI * p.frequency_hz / p.sample_rate;

    for (std::size_t i = 0; i < n_samples; ++i) {
        buffer[i] = sample(static_cast<float>(_phase), p.waveform, p.amplitude);

        _phase += phase_increment;
        if (_phase >= TWO_PI) {
            _phase -= TWO_PI; // Mantém fase em [0, 2π) para evitar drift de ponto flutuante
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// sample() — gera valor para um instante de fase
// ──────────────────────────────────────────────────────────────────────────────
float SineGenerator::sample(float phase, WaveformType type, float amplitude) const {
    float value = 0.0f;
    switch (type) {
        case WaveformType::Sine:
            value = std::sin(phase);
            break;
        case WaveformType::Square:
            // Onda quadrada: +1 na primeira metade do período, -1 na segunda
            value = (phase < static_cast<float>(M_PI)) ? 1.0f : -1.0f;
            break;
        case WaveformType::Triangle:
            // Onda triangular por linearização da fase normalizada [0, 1)
            {
                float t = phase / static_cast<float>(TWO_PI); // [0, 1)
                value = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
            }
            break;
        case WaveformType::Sawtooth:
            // Dente-de-serra: rampa de -1 a +1 ao longo do período
            value = (phase / static_cast<float>(M_PI)) - 1.0f;
            break;
    }
    return amplitude * value;
}

// ──────────────────────────────────────────────────────────────────────────────
// setParams() / getParams()
// ──────────────────────────────────────────────────────────────────────────────
void SineGenerator::setParams(const SignalParams& params) {
    while (_lock.test_and_set(std::memory_order_acquire)) { /* spin */ }
    _params = params;
    _lock.clear(std::memory_order_release);
}

SignalParams SineGenerator::getParams() const {
    while (_lock.test_and_set(std::memory_order_acquire)) { /* spin */ }
    SignalParams copy = _params;
    _lock.clear(std::memory_order_release);
    return copy;
}

float SineGenerator::sampleRate() const {
    while (_lock.test_and_set(std::memory_order_acquire)) { /* spin */ }
    float sr = _params.sample_rate;
    _lock.clear(std::memory_order_release);
    return sr;
}

} // namespace sonar
