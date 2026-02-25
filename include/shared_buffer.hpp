#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstddef>

/**
 * @file shared_buffer.hpp
 * @brief Buffer circular thread-safe para comunicação produtor/consumidor
 *        dentro do processo generator_app.
 *
 * Concorrência:
 *   - Thread produtora (geração de sinal) chama push().
 *   - Thread consumidora (escrita em shm/stdout) chama pop().
 *   - Sincronização via std::mutex + std::condition_variable.
 *
 * Por que usar um buffer circular em vez de uma fila ilimitada?
 *   - Consumo de memória previsível (importante em sistemas embarcados).
 *   - Política de descarte (overwrite) quando o consumidor não acompanha
 *     o produtor — adequado para sinais de áudio em tempo real.
 */

namespace sonar {

/**
 * @brief Ring buffer thread-safe de capacidade fixa.
 *
 * @tparam T  Tipo dos elementos armazenados.
 *
 * Política quando cheio: sobrescreve amostras mais antigas (produtor
 * nunca bloqueia). O consumidor bloqueia quando o buffer está vazio.
 */
template<typename T>
class SharedBuffer {
public:
    explicit SharedBuffer(std::size_t capacity)
        : _buf(capacity), _capacity(capacity) {}

    /**
     * @brief Insere elementos no buffer (chamado pela thread produtora).
     * Não bloqueia: se cheio, descarta amostras antigas.
     */
    void push(const std::vector<T>& samples) {
        {
            std::lock_guard<std::mutex> lk(_mutex);
            for (const auto& s : samples) {
                _buf[_write % _capacity] = s;
                ++_write;
                if (_write - _read > _capacity) {
                    // Buffer cheio: avança o leitor (descarta mais antigo)
                    ++_read;
                }
            }
        }
        _cv.notify_one();
    }

    /**
     * @brief Retira até `max_count` elementos (chamado pela thread consumidora).
     * Bloqueia até haver pelo menos 1 elemento disponível.
     *
     * @return Amostras disponíveis (pode ser menos que max_count).
     */
    std::vector<T> pop(std::size_t max_count) {
        std::unique_lock<std::mutex> lk(_mutex);
        _cv.wait(lk, [this]{ return _write > _read; });

        std::size_t avail = _write - _read;
        std::size_t count = std::min(avail, max_count);
        std::vector<T> out(count);
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = _buf[_read % _capacity];
            ++_read;
        }
        return out;
    }

    /// Retorna o número de amostras disponíveis para leitura.
    std::size_t available() const {
        std::lock_guard<std::mutex> lk(_mutex);
        return _write - _read;
    }

private:
    std::vector<T>          _buf;
    std::size_t             _capacity;
    std::size_t             _write = 0;
    std::size_t             _read  = 0;
    mutable std::mutex      _mutex;
    std::condition_variable _cv;
};

} // namespace sonar
