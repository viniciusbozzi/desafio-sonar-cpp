#pragma once

#include "signal_params.hpp"
#include <cstddef>
#include <string>
#include <atomic>

/**
 * @file shared_memory.hpp
 * @brief Wrapper C++ Orientado a Objetos para Memória Compartilhada POSIX.
 *
 * Encapsula a complexidade do shm_open, mmap e limpeza.
 * Gerencia duas áreas de memória:
 *  1. Parâmetros do sinal (SignalParams) - lido/escrito atomicamente
 *  2. Sinal mais recente gerado (buffer array) - lido pela IHM para visualização
 */

namespace sonar {

// Tamanho fixo do buffer visível para a IHM (amostras).
// O gerador escreve as amostras mais recentes aqui de forma circular.
constexpr std::size_t SHM_SIGNAL_BUFFER_SIZE = 1024;

/**
 * @brief Estrutura residente na memória compartilhada para gerenciar os dados exibidos.
 */
struct SharedSignalData {
    // Índice indicando até onde o gerador escreveu (para IHM alinhar o desenho).
    // atomic para evitar race conditions básicas de leitura/escrita.
    std::atomic<std::size_t> head_index{0};
    
    // Buffer circular mantendo as últimas amostras geradas.
    float buffer[SHM_SIGNAL_BUFFER_SIZE] = {0.0f};
};

/**
 * @brief Classe Base para acesso a um segmento genérico de memória POSIX.
 * Usa RAII para mapear/desmapear a memória.
 */
class PosixShmSegment {
public:
    /**
     * @brief Abre ou cria um segmento de memória compartilhada.
     * @param name Nome POSIX do segmento (ex: "/sonar_params")
     * @param size Tamanho necessário em bytes
     * @param create Se verdadeiro, tenta criar a memória (O_CREAT)
     * @param read_only Se verdadeiro, mapeia apenas leitura
     */
    PosixShmSegment(const std::string& name, std::size_t size, bool create, bool read_only);
    ~PosixShmSegment();

    // Impede cópia para evitar múltiplos desmapeamentos acidentais.
    PosixShmSegment(const PosixShmSegment&) = delete;
    PosixShmSegment& operator=(const PosixShmSegment&) = delete;

    void* ptr() const { return _ptr; }
    bool isValid() const { return _valid; }

    static void unlink(const std::string& name);

private:
    std::string _name;
    std::size_t _size;
    void* _ptr = nullptr;
    int _fd = -1;
    bool _valid = false;
};

/**
 * @brief Facade de alto nível para as áreas de memória do sistema Sonar.
 */
class SonarSharedMemory {
public:
    /**
     * @param is_host Se verdadeiro, este processo é dono da memória (cria na carga e exclui na saída). 
     * O `generator_app` é o host. O `control_app` e `ihm_viewer` não são hosts (is_host=false).
     */
    explicit SonarSharedMemory(bool is_host);
    ~SonarSharedMemory();

    // Métodos para acesso seguro aos parâmetros
    void writeParams(const SignalParams& params);
    SignalParams readParams() const;

    // Métodos para acesso ao sinal (buffer circular para IHM)
    // O gerador escreve em blocos.
    void writeSignalSamples(const float* samples, std::size_t count);
    
    // A IHM lê os dados do buffer circular e o index atual
    void readSignalSamples(float* out_buffer, std::size_t out_capacity, std::size_t& out_head_index) const;

private:
    bool _is_host;
    PosixShmSegment _shm_params;
    PosixShmSegment _shm_signal;

    SignalParams* _params_ptr = nullptr;
    SharedSignalData* _signal_ptr = nullptr;
};

} // namespace sonar
