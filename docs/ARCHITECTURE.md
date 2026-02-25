# Arquitetura do Sistema Sonar

## Visão Geral

O sistema é composto por **três processos independentes** que se comunicam via
**memória compartilhada POSIX** (`shm_open` / `mmap`):

```
┌─────────────────────────────────┐
│        generator_app            │  Processo 1
│  Thread de geração ─────────►   │  SineGenerator → SharedBuffer
│  Thread de saída   ◄─────────   │                → shm /sonar_signal
└────────────────┬────────────────┘
                 │ lê /sonar_params
                 ▼
┌─────────────────────────────────┐
│         control_app             │  Processo 2
│  (CLI — stdin loop)             │  escreve /sonar_params
└────────────────┬────────────────┘
                 │
                 ▼ /sonar_signal (ring buffer)
┌─────────────────────────────────┐
│         ihm_viewer              │  Processo 3
│  GTKmm + Cairo (60 Hz)          │  lê /sonar_signal → desenha waveform
└─────────────────────────────────┘
```

---

## Hierarquia de Classes — Fontes de Sinal

```
ISignalSource  (interface abstrata — signal_source.hpp)
│
├── SineGenerator          ← Commit 1  [implementado]
│     Parâmetros: freq, amplitude, tipo de onda
│     Formas suportadas: senoide, quadrada, triangular, dente-de-serra
│
├── WavFileSource          ← futuro
│     Lê arquivo WAV via libsndfile, gera amostras normalizadas
│
└── SharedMemorySource     ← futuro
      Lê stream de shm externo (sensor físico, simulador, etc.)
```

### Princípio Aberto/Fechado (OCP)

Cada nova fonte de sinal:
1. **Cria** um novo arquivo `.hpp/.cpp`
2. **Herda** de `ISignalSource`
3. **Não modifica** nenhum código existente

O `generator_app` recebe um `std::unique_ptr<ISignalSource>` — nunca conhece
a implementação concreta:

```cpp
// Exemplo futuro sem mudança no generator_app:
std::unique_ptr<sonar::ISignalSource> src =
    std::make_unique<sonar::WavFileSource>("torpedo.wav");
```

---

## Concorrência — Threads dentro do generator_app

| Thread        | Responsabilidade                          | Sincronização          |
|--------------|-------------------------------------------|------------------------|
| `gen_thread`  | `SineGenerator::generate()` → `push()`   | `SharedBuffer` mutex   |
| `out_thread`  | `pop()` → escreve em shm / stdout         | `condition_variable`   |
| `main thread` | ciclo de vida, shutdown via SIGINT        | `atomic<bool> g_running` |

**Por que threads e não subprocessos aqui?**
Threads compartilham o mesmo espaço de endereçamento, tornando o `SharedBuffer`
eficiente sem serialização. O custo de IPC (shm, pipes) só se justifica entre
processos distintos — que é exatamente o que fazemos nos 3 processos do sistema.

---

## Parâmetros do Sinal — `SignalParams`

Struct POD de 16 bytes mapeada diretamente em `/sonar_params` (shm):

| Campo          | Tipo          | Descrição                      |
|---------------|---------------|-------------------------------|
| `frequency_hz` | `float`       | Frequência fundamental (Hz)   |
| `amplitude`    | `float`       | Amplitude normalizada [0, 1]  |
| `sample_rate`  | `float`       | Taxa de amostragem (Hz)       |
| `waveform`     | `WaveformType`| Senoide/Quadrada/etc.         |

---

## Dependências

| Biblioteca    | Uso                              |
|--------------|----------------------------------|
| C++17 stdlib  | threads, mutex, atomic, chrono   |
| GTKmm 3.0    | IHM (janela + Cairo waveform)    |
| POSIX         | shm_open, mmap (Commit 2)        |
