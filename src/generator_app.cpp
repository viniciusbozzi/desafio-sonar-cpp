/**
 * @file generator_app.cpp
 * @brief Processo 1 — Gerador de Sinal Sonar
 *
 * Responsabilidade:
 *   Gera amostras de sinal contínuo e as imprime em stdout (formato CSV),
 *   permitindo visualização ou redirecionamento. Na etapa seguinte (Commit 2),
 *   a saída será migrada para memória compartilhada POSIX.
 *
 * Arquitetura de threads neste processo:
 *
 *   main thread ──► controla ciclo de vida, lê stdin (futura expansão)
 *   gen_thread  ──► SineGenerator::generate() → SharedBuffer::push()
 *   out_thread  ──► SharedBuffer::pop() → stdout
 *
 * Por que usar threads aqui e não processos?
 *   - Threads compartilham o espaço de endereçamento, tornando o
 *     SharedBuffer extremamente eficiente (sem serialização de dados).
 *   - O overhead de IPC (pipes/shm) só se justifica ENTRE os 3 processos
 *     do sistema, não inside de cada processo.
 *   - Separação clara de responsabilidades sem custo de cópia de dados.
 *
 * Compilação:
 *   cmake --build build --target generator_app
 *
 * Execução:
 *   ./build/generator_app [frequência_hz] [amplitude] [sample_rate]
 *   Exemplo: ./build/generator_app 440 0.8 44100
 */

#include "sine_generator.hpp"
#include "shared_buffer.hpp"
#include "signal_params.hpp"
#include "shared_memory.hpp"
#include "wav_file_source.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <chrono>
#include <cstdlib>


// ──────────────────────────────────────────────────────────────────────────────
// Variável global de controle de shutdown (sinal SIGINT/SIGTERM)
// ──────────────────────────────────────────────────────────────────────────────
static std::atomic<bool> g_running{true};

static void signal_handler(int /*sig*/) {
    g_running.store(false, std::memory_order_relaxed);
}

// ──────────────────────────────────────────────────────────────────────────────
// Thread de geração: produz amostras e empurra para o SharedBuffer
// ──────────────────────────────────────────────────────────────────────────────
static void generation_thread(sonar::ISignalSource& gen,
                               sonar::SharedBuffer<float>& buffer,
                               std::size_t chunk_size)
{
    std::vector<float> chunk;
    while (g_running.load(std::memory_order_relaxed)) {
        gen.generate(chunk, chunk_size);
        buffer.push(chunk);

        // Simula cadência de produção (chunk/sample_rate segundos)
        const auto period_us = static_cast<long>(
            1'000'000.0 * chunk_size / gen.sampleRate());
        std::this_thread::sleep_for(std::chrono::microseconds(period_us));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Thread de saída: consome amostras do buffer e escreve na Memória Compartilhada
// ──────────────────────────────────────────────────────────────────────────────
static void output_thread(sonar::SharedBuffer<float>& buffer, sonar::SonarSharedMemory& shm) {
    while (g_running.load(std::memory_order_relaxed)) {
        auto samples = buffer.pop(256); // Bloqueia até ter amostras
        if (!samples.empty()) {
            shm.writeSignalSamples(samples.data(), samples.size());
        }
    }
}


// ──────────────────────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Registra handlers de sinal para shutdown gracioso
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Parâmetros iniciais — podem ser substituídos via args da linha de comando
    sonar::SignalParams params;
    if (argc > 1) params.frequency_hz = std::atof(argv[1]);
    if (argc > 2) params.amplitude    = std::atof(argv[2]);
    if (argc > 3) params.sample_rate  = std::atof(argv[3]);

    // Inicializa a Memória Compartilhada como HOST (cria as regiões de shm)
    sonar::SonarSharedMemory shm_host(true);
    
    // Escreve os parâmetros iniciais lidos via linha de comando
    shm_host.writeParams(params);

    std::cerr << "[generator_app] Iniciando gerador de sinal\n"
              << "  Pressione Ctrl+C para encerrar.\n"
              << "  Aguardando conexoes no 'control_app' ou 'ihm_viewer'...\n";

    // Instancia gerador polimorficamente via Interface Abstrata (Princípio Aberto/Fechado OCP)
    std::unique_ptr<sonar::ISignalSource> gen = std::make_unique<sonar::SineGenerator>(params);

    // =========================================================================
    // (Leitura de Arquivo WAV)
    // Para testar a leitura de arquivo, comente a linha do SineGenerator acima e
    // descomente a linha abaixo
    // =========================================================================
    // std::unique_ptr<sonar::ISignalSource> gen = std::make_unique<sonar::WavFileSource>("teste.wav");

    const std::size_t buffer_capacity = static_cast<std::size_t>(params.sample_rate) * 2;
    sonar::SharedBuffer<float> shared_buf(buffer_capacity);

    constexpr std::size_t CHUNK_SIZE = 512; // amostras por bloco de geração

    // Lança threads de geração e saída usando referência ao gerador abstrato
    std::thread gen_thr(generation_thread, std::ref(*gen), std::ref(shared_buf), CHUNK_SIZE);
    std::thread out_thr(output_thread, std::ref(shared_buf), std::ref(shm_host));

    // A Main thread agora se encarrega de ler periodicamente os parâmetros
    // da memória compartilhada e atualizar o SineGenerator dinamicamente.
    while (g_running.load(std::memory_order_relaxed)) {
        sonar::SignalParams p = shm_host.readParams();
        
        // Atualiza os parâmetros do gerador em tempo de execução (se for um oscilador controlável).
        // A Thread Safety já é gerida pelo spinlock atômico dentro do SineGenerator.
        // O pattern dynamic_cast protege fontes futuras 
        if (auto controllable_gen = dynamic_cast<sonar::SineGenerator*>(gen.get())) {
            controllable_gen->setParams(p);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Acorda a thread de saída caso esteja bloqueada no pop vazia
    std::vector<float> empty_wake;
    shared_buf.push(empty_wake);

    gen_thr.join();
    out_thr.join();

    std::cerr << "[generator_app] Encerrado.\n";
    return 0;
}
