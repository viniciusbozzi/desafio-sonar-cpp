#pragma once

#include "shared_memory.hpp"
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <glibmm/timer.h>
#include <vector>

/**
 * @file sonar_window.hpp
 * @brief IHM Principal — Exibição do Sinal Sonar usando GTKmm e Cairo.
 */

namespace sonar {

/**
 * @brief Widget de desenho customizado.
 * Lê a memória compartilhada a 60Hz e redesenha a forma de onda
 * utilizando a biblioteca gráfica vetorial Cairo.
 */
class WaveformArea : public Gtk::DrawingArea {
public:
    WaveformArea();
    virtual ~WaveformArea();

protected:
    // Override do método de desenho principal do GTK
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

private:
    // Timer para solicitar redesenho periódico (60 fps)
    bool on_timeout();

    SonarSharedMemory _shm_client;
    std::vector<float> _buffer;
};

/**
 * @brief Janela principal da aplicação IHM.
 * Contém os widgets e define o layout geral.
 */
class SonarWindow : public Gtk::Window {
public:
    SonarWindow();
    virtual ~SonarWindow();

private:
    WaveformArea _waveform_area;
};

} // namespace sonar
