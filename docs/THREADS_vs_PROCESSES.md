# Concorrência: Threads vs Processos no Sistema Sonar

Este documento serve como justificativa técnica para as decisões arquiteturais tomadas durante o desenvolvimento do desafio, especialmente focando na diferença entre **Threads** e **Processos** e onde cada estratégia se adequa melhor.

---

## 1. Processos (Multi-Processing)

O Projeto foi deliberadamente arquitetado em **TRÊS PROCESSOS independentes** (`generator_app`, `control_app`, e `ihm_viewer`). 

### Diferenças Conceituais
- **Processos** são instâncias de execução separadas que operam em seus próprios espaços de endereçamento de memória virtual.
- Eles não compartilham memória automaticamente; portanto, um *segmentation fault* na `IHM` jamais derrubará o `generator_app`.

### Justificativa de Uso
No contexto de sistemas críticos, **separação de responsabilidade e resiliência** são fundamentais:
1. **Segurança e Estabilidade:** A interface gráfica não pode travar o processamento duro de dados matemáticos do sinal sonar em hipótese alguma.
2. **Distribuição:** No mundo real, os processos nem sempre rodam na mesma máquina. O gerador poderia estar rodando no hardware de captura e enviando os dados para a sala de controle (IHM) em um cluster distante (a transição de Shm IPC para Rede/DDS teria que ser adaptada, mas não é um problema).

### IPC (Inter-Process Communication)
Para permitir que esses espaços separados de controle compartilhem a mesma "Fonte de Verdade", optou-se pela **Memória Compartilhada POSIX** (`shm_open`, `mmap`). Essa é uma forma rápida de IPC (Overhead de Microsegundos), essencial para streamings de áudio/sinais visuais a 60 FPS, sem o custo de cópia de *pipes* locais, sockets ou arquivos.

---

## 2. Threads (Multi-Threading)

Apesar da forte separação em processos na escala global, internamente no `generator_app` optamos por usar **Múltiplas Threads**.

### Diferenças Conceituais
- **Threads** operam _dentro_ do espaço de um mesmo Processo e compartilham nativamente toda a heap memory, variáveis estáticas e descritores de arquivo.

### Justificativa de Uso
Dentro do `generator_app` nós separamos a Geração de Sinal da Publicação em IPC usando duas threads:
1. `gen_thread`: Faz cálculos trigonométricos puros com base em ponteiros de fase e salva as predições num `SharedBuffer` (Ring Buffer local ao processo).
2. `out_thread`: Captura as previsões do array e comita as mudanças usando POSIX SHM. 

Aqui, se fosse usado processos, obrigaria o Sistema Operacional a serializar ou mapear milhares de amostras flutuantes em memória intermediária. Com Threads, apenas empurramos um ponteiro e um tamanho num vector, travando momentaneamente por um _Mutex_ e acendemos um _Condition Variable_ (`std::condition_variable`).

### Sincronização
Ao compartilhar recursos entre threads/processos sempre nos precavemos das corridas (**Race Conditions**):
- **Entre Threads**: Protegido por um `std::mutex` associado a uma `std::condition_variable` no `shared_buffer.hpp` com operação O(1) e lock seguro local.
- **Entre Processos**: O acesso atômico à Memória Compartilhada no buffer de leitura e escrita requer barreiras em C++ modernas `std::atomic<bool>` / `std::memory_order_acquire / release` para garantir integridade do leitor IHM com o array desenhado (visto em `SonarSharedMemory`).
