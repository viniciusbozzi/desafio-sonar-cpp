cmake_minimum_required(VERSION 3.10)
project(SonarSystem VERSION 1.0 LANGUAGES CXX)

# ─── Padrão C++17 ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Dependências externas ────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
find_package(Threads   REQUIRED)          # std::thread / pthreads
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

# ─── Diretórios de include globais ────────────────────────────────────────────
include_directories(include ${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})

# ─── Biblioteca interna: sonar_core ──────────────────────────────────────────
# Contém código compartilhado entre os três executáveis (SineGenerator, etc.)
add_library(sonar_core STATIC
    src/sine_generator.cpp
    src/shared_memory.cpp
    src/wav_file_source.cpp
)
target_include_directories(sonar_core PUBLIC include)
target_link_libraries(sonar_core PUBLIC Threads::Threads rt)

# ─── Executável 1: generator_app (Processo de Geração) ───────────────────────
add_executable(generator_app src/generator_app.cpp)
target_link_libraries(generator_app PRIVATE sonar_core)

# ─── Executável 2: control_app (Processo de Controle CLI) ────────────────────
add_executable(control_app src/control_app.cpp)
target_link_libraries(control_app PRIVATE sonar_core)

# ─── Executável 3: ihm_viewer (Processo de Visualização — IHM) ───────────────
add_executable(ihm_viewer src/ihm_viewer.cpp src/sonar_window.cpp)
target_link_libraries(ihm_viewer PRIVATE sonar_core ${GTKMM_LIBRARIES})

# ─── Mensagem de status no configure ─────────────────────────────────────────
message(STATUS "SonarSystem configurado:")
message(STATUS "  C++ padrão  : ${CMAKE_CXX_STANDARD}")
message(STATUS "  GTKmm       : ${GTKMM_VERSION}")
message(STATUS "  Threads     : ${CMAKE_THREAD_LIBS_INIT}")