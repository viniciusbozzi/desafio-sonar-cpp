/**
 * @file control_app.cpp
 * @brief Processo 2 — Aplicativo de Controle Sonar via CLI
 *
 * Responsabilidade:
 *   Fornece uma IHM por linha de comandos para alterar
 *   os parâmetros do sinal em tempo real.
 *
 * Comunicação:
 *   Lê e escreve na memória compartilhada `/sonar_params`
 *   via classe `SonarSharedMemory` (não é Host).
 *
 * Compilação:
 *   cmake --build build --target control_app
 *
 * Execução:
 *   ./build/control_app
 */

#include "shared_memory.hpp"
#include "signal_params.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

void printHelp() {
    std::cout << "\n=== Comandos Disponiveis ===\n"
              << "freq <valor>      : Altera a frequencia (Hz). Ex: freq 440\n"
              << "amp  <valor>      : Altera a amplitude [0.0 - 1.0]. Ex: amp 0.8\n"
              << "onda <tipo>       : Altera o tipo de onda (0=Seno, 1=Quadrada, 2=Triangulo, 3=Dente de serra)\n"
              << "status            : Exibe os parametros atuais\n"
              << "help              : Mostra esta ajuda\n"
              << "quit / exit       : Sai do aplicativo de controle\n"
              << "============================\n\n";
}

void printStatus(const sonar::SignalParams& p) {
    std::cout << "\n[STATUS ATUAL]\n"
              << " Frequência : " << p.frequency_hz << " Hz\n"
              << " Amplitude  : " << p.amplitude << "\n"
              << " Sample Rate: " << p.sample_rate << " Hz\n"
              << " Onda       : ";
    
    switch (p.waveform) {
        case sonar::WaveformType::Sine:     std::cout << "Senoide\n"; break;
        case sonar::WaveformType::Square:   std::cout << "Quadrada\n"; break;
        case sonar::WaveformType::Triangle: std::cout << "Triangular\n"; break;
        case sonar::WaveformType::Sawtooth: std::cout << "Dente-de-serra\n"; break;
        default: std::cout << "Desconhecida\n"; break;
    }
    std::cout << "----------------------------\n";
}

int main() {
    std::cout << "Iniciando Sonar Control App...\n";

    // Instancia interface de shm como CLIENTE (is_host = false)
    sonar::SonarSharedMemory shm_client(false);

    if (!shm_client.isValid()) {
        std::cerr << "ERRO FATAL: O generator_app não está rodando.\n"
                  << "Não foi possível conectar à Memória Compartilhada do Sonar.\n"
                  << "Inicie o ./generator_app primeiro.\n";
        return 1;
    }

    std::cout << "Conectado à Memória Compartilhada do Gerador com sucesso!\n";
    sonar::SignalParams params = shm_client.readParams();
    printStatus(params);

    printHelp();

    // Loop de REPL (Read-Eval-Print Loop)
    std::string line;
    while (true) {
        std::cout << "sonar> ";
        if (!std::getline(std::cin, line)) {
            break; // EOF ou erro
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "quit" || command == "exit") {
            break;
        } 
        else if (command == "help") {
            printHelp();
        } 
        else if (command == "status") {
            params = shm_client.readParams();
            printStatus(params);
        }
        else if (command == "freq") {
            float val;
            if (iss >> val && val > 0.0f) {
                params = shm_client.readParams();
                params.frequency_hz = val;
                shm_client.writeParams(params);
                std::cout << "Frequencia alterada para " << val << " Hz\n";
            } else {
                std::cout << "Erro: valor invalido. Exemplo: freq 440\n";
            }
        }
        else if (command == "amp") {
            float val;
            if (iss >> val && val >= 0.0f && val <= 1.0f) {
                params = shm_client.readParams();
                params.amplitude = val;
                shm_client.writeParams(params);
                std::cout << "Amplitude alterada para " << val << "\n";
            } else {
                std::cout << "Erro: valor invalido (deve ser entre 0.0 e 1.0). Exemplo: amp 0.5\n";
            }
        }
        else if (command == "onda") {
            int val;
            if (iss >> val && val >= 0 && val <= 3) {
                params = shm_client.readParams();
                params.waveform = static_cast<sonar::WaveformType>(val);
                shm_client.writeParams(params);
                std::cout << "Tipo de onda alterado com sucesso.\n";
            } else {
                std::cout << "Erro: valor invalido. (0=Seno, 1=Quad, 2=Trian, 3=Serra)\n";
            }
        }
        else {
            std::cout << "Comando nao reconhecido: " << command << ". Digite 'help' para ajuda.\n";
        }
    }

    std::cout << "Encerrando Control App.\n";
    return 0;
}
