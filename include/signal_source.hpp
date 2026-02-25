#pragma once

#include <cstddef>
#include <vector>

/**
 * @file signal_source.hpp
 * @brief Interface abstrata para fontes de sinal do sistema sonar.
 *
 * Princípio Aberto/Fechado (OCP):
 *   Novas fontes de sinal (WAV, memória compartilhada, rede, etc.) são
 *   adicionadas implementando ISignalSource SEM modificar código existente.
 *
 * Hierarquia prevista:
 *
 *   ISignalSource  (interface pura — este arquivo)
 *   ├── SineGenerator        ← Commit 1
 *   ├── WavFileSource        ← futuro
 *   └── SharedMemorySource   ← futuro
 */

namespace sonar {

/**
 * @brief Interface abstrata para qualquer fonte de sinal acústico.
 *
 * Cada implementação concreta é responsável por preencher um buffer de
 * amostras `float` com valores normalizados no intervalo [-1, 1].
 */
class ISignalSource {
public:
    virtual ~ISignalSource() = default;

    /**
     * @brief Gera `n_samples` amostras e as armazena em `buffer`.
     *
     * @param buffer  Vetor de destino (será redimensionado se necessário).
     * @param n_samples Quantidade de amostras a gerar.
     */
    virtual void generate(std::vector<float>& buffer, std::size_t n_samples) = 0;

    /**
     * @brief Retorna a taxa de amostragem utilizada pela fonte (Hz).
     */
    virtual float sampleRate() const = 0;

    /**
     * @brief Retorna um nome descritivo da fonte (para logging/debug).
     */
    virtual const char* name() const = 0;
};

} // namespace sonar
