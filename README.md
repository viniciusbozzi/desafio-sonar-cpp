# Desafio Sonar C++

## 📖 Sobre o Projeto
Este projeto é uma aplicação em C++ desenvolvida para o ambiente Linux, que simula, controla e visualiza sinais típicos de sonar.

O sistema é composto por três executáveis distintos:
- **`generator_app`**: O processo responsável por gerar os sinais (como ondas senoidais) e manipular os cálculos brutos e/ou streaming de dados.
- **`control_app`**: O processo responsável por controlar os parâmetros do gerador de sinais.
- **`ihm_viewer`**: O processo responsável por gerar uma interface gráfica, focada na visualização e análise de comportamento dos sinais gerados.

## 🚀 Requisitos do Sistema
Para compilar e executar este projeto, o seu ambiente Linux deve possuir:
- **Compilador C++**: Compatível com **C++17** (como `g++` ou `clang++`).
- **CMake**: Versão `3.10` ou superior.
- **Build System**: `make`
- **gtkmm-3.0**: Wrapper C++ moderno para a biblioteca de interface gráfica GTK.
- **pkg-config**: Ferramenta necessária para a detecção de módulos do GTK pelo CMake.
- Pacotes padrão de manipulação de *threads* (POSIX Threads).

**Comando rápido para instalação (Ubuntu):**
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libgtkmm-3.0-dev
```

## 🛠️ Como Compilar

Clone o repositório:
```bash
git clone https://github.com/viniciusbozzi/desafio-sonar-cpp.git
```

O projeto utiliza o CMake para criar o ambiente de *build*. O passo a passo é simples:

1. Abra o terminal na raiz do projeto (pasta `desafio-sonar-cpp`).
2. Crie uma pasta auxiliar e entre nela:
   ```bash
   mkdir -p build && cd build
   ```
3. Gere os arquivos de construção informando a localização do `CMakeLists.txt`:
   ```bash
   cmake ..
   ```
4. Inicie a compilação:
   ```bash
   make
   ```
Ao fim da compilação, você verá três arquivos binários dentro da pasta `build`: `generator_app`, `control_app` e `ihm_viewer`.

## 🧪 Como Testar e Executar

Todo comportamento do sistema pode ser iterado rodando cada um dos módulos separadamente. A partir da raiz do projeto:

### 1. Geração de Sinal (generator_app)
O gerador calcula amostras do sinal de sonar. Você pode redirecionar a saída de texto dele direto para um arquivo, para visualizações ou plotagens com ferramentas externas (como Python pandas ou Excel):
```bash
./build/generator_app > meu_audio.csv
```

### 2. Controle de Parâmetros (control_app)
O controle de parâmetros é feito via linha de comando. Você pode alterar os parâmetros do gerador de sinais digitando os valores desejados no terminal:
```bash
./build/control_app
```

### 3. Visualização de Interface (ihm_viewer)
Testar o aspecto visual da interface gráfica é muito direto. Execute o módulo de interface, que utilizará a API do GTK para desenhar a aplicação localmente:
```bash
./build/ihm_viewer
```
*(O visualizador abrirá uma janela nativa no seu ambiente CDE/Gnome/KDE/etc.)*

## 🗂️ Estrutura do Repositório principal
- **`CMakeLists.txt`**: Orienta o CMake a vincular a biblioteca base `sonar_core` e bibliotecas externas como GTKmm e Threads aos nossos binários.
- **`src/`**: O coração da aplicação. Contém os arquivos `.cpp`, divididos entre a biblioteca principal, gerador e visualizador.
- **`include/`**: Bibliotecas cabeçalho `.hpp` com a declaração das nossas classes e interfaces de sistemas.
- **`docs/`**: Manuais, anotações e documentos acessórios da arquitetura base.
