#include "shared_memory.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream>

// Headers POSIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sonar {

// Nomes fixos dos segmentos no SO Linux POSIX
static const char* SHM_NAME_PARAMS = "/sonar_params";
static const char* SHM_NAME_SIGNAL = "/sonar_signal";

// ──────────────────────────────────────────────────────────────────────────────
// PosixShmSegment
// ──────────────────────────────────────────────────────────────────────────────
PosixShmSegment::PosixShmSegment(const std::string& name, std::size_t size, bool create, bool read_only)
    : _name(name), _size(size) 
{
    int oflag = read_only ? O_RDONLY : O_RDWR;
    if (create) {
        // O_CREAT: cria se não existir.
        oflag |= O_CREAT;
    }

    mode_t mode = 0666; // Permissões de leitura e gravação
    _fd = shm_open(_name.c_str(), oflag, mode);
    
    if (_fd == -1) {
        std::cerr << "[shm] Erro ao abrir shm_open para: " << _name << "\n";
        return;
    }

    if (create) {
        // Redimensiona o segmento de memória recém-criado
        if (ftruncate(_fd, _size) == -1) {
            std::cerr << "[shm] Erro no ftruncate para: " << _name << "\n";
            close(_fd);
            return;
        }
    }

    // Mapeia a memória para o espaço de endereçamento do processo
    int prot = PROT_READ;
    if (!read_only) {
        prot |= PROT_WRITE;
    }

    _ptr = mmap(nullptr, _size, prot, MAP_SHARED, _fd, 0);
    if (_ptr == MAP_FAILED) {
        std::cerr << "[shm] Erro no mmap para: " << _name << "\n";
        close(_fd);
        _ptr = nullptr;
        return;
    }

    // shm_open fd não é mais necessário após mmap
    close(_fd);
    _valid = true;
}

PosixShmSegment::~PosixShmSegment() {
    if (_valid && _ptr != nullptr && _ptr != MAP_FAILED) {
        munmap(_ptr, _size);
    }
}

void PosixShmSegment::unlink(const std::string& name) {
    shm_unlink(name.c_str());
}

// ──────────────────────────────────────────────────────────────────────────────
// SonarSharedMemory
// ──────────────────────────────────────────────────────────────────────────────
SonarSharedMemory::SonarSharedMemory(bool is_host)
    : _is_host(is_host),
      _shm_params(SHM_NAME_PARAMS, sizeof(SignalParams), is_host, false), // read/write para ambos agora, para control e generator (ambos precisam ler e escrever)
      _shm_signal(SHM_NAME_SIGNAL, sizeof(SharedSignalData), is_host, false) // ihm_viewer lê, generator salva
{
    if (!_shm_params.isValid() || !_shm_signal.isValid()) {
        std::cerr << "[SonarSharedMemory] Aviso: falha ao inicializar memória. Processo host executando?\n";
    } else {
        _params_ptr = static_cast<SignalParams*>(_shm_params.ptr());
        _signal_ptr = static_cast<SharedSignalData*>(_shm_signal.ptr());
        
        if (_is_host && _params_ptr) {
            // Inicializa com parâmetros default
            new (_params_ptr) SignalParams(); 
            // Inicializa construtor de atomic data
            new (_signal_ptr) SharedSignalData();
        }
    }
}

SonarSharedMemory::~SonarSharedMemory() {
    if (_is_host) {
        // O Host é responsável por remover o segmento do OS 
        // (Isso é chamado apenas ao encerrar o generator_app)
        PosixShmSegment::unlink(SHM_NAME_PARAMS);
        PosixShmSegment::unlink(SHM_NAME_SIGNAL);
    }
}

void SonarSharedMemory::writeParams(const SignalParams& params) {
    if (_params_ptr) {
        // Evita dataraces simples em tempo real usando atomic lock-free copy ou memcpy c/ barreiras
        // para cenários fora da crítica real time.
        // Adicionando barreira para ordering
        std::atomic_thread_fence(std::memory_order_acquire);
        std::memcpy(_params_ptr, &params, sizeof(SignalParams));
        std::atomic_thread_fence(std::memory_order_release);
    }
}

SignalParams SonarSharedMemory::readParams() const {
    SignalParams ret;
    if (_params_ptr) {
        std::atomic_thread_fence(std::memory_order_acquire);
        std::memcpy(&ret, _params_ptr, sizeof(SignalParams));
        std::atomic_thread_fence(std::memory_order_release);
    }
    return ret;
}

void SonarSharedMemory::writeSignalSamples(const float* samples, std::size_t count) {
    if (!_signal_ptr) return;

    // Acessa valor currente de atomic via load rel/acq
    std::size_t head = _signal_ptr->head_index.load(std::memory_order_acquire);

    for (std::size_t i = 0; i < count; ++i) {
        _signal_ptr->buffer[head % SHM_SIGNAL_BUFFER_SIZE] = samples[i];
        ++head;
    }

    _signal_ptr->head_index.store(head, std::memory_order_release);
}

void SonarSharedMemory::readSignalSamples(float* out_buffer, std::size_t out_capacity, std::size_t& out_head_index) const {
    if (!_signal_ptr) return;

    std::size_t head = _signal_ptr->head_index.load(std::memory_order_acquire);
    out_head_index = head;

    std::size_t capacity_to_read = std::min(out_capacity, SHM_SIGNAL_BUFFER_SIZE);
    
    // Calcula o ponto de partida do histórico 
    // Pega as informações logo atrás do head atual
    std::size_t start_idx = (head >= capacity_to_read) ? (head - capacity_to_read) : 0;
    
    if (head < capacity_to_read) {
         // O buffer gerou muito pouco elemento até agora
         capacity_to_read = head; 
         start_idx = 0;
    }

    for (std::size_t i = 0; i < capacity_to_read; ++i) {
        out_buffer[i] = _signal_ptr->buffer[(start_idx + i) % SHM_SIGNAL_BUFFER_SIZE];
    }
}

} // namespace sonar
