#include "sonar_window.hpp"
#include <glibmm/main.h>
#include <iostream>

namespace sonar {

// ──────────────────────────────────────────────────────────────────────────────
// WaveformArea (Onda desenhada via Cairo)
// ──────────────────────────────────────────────────────────────────────────────

WaveformArea::WaveformArea()
    : _shm_client(false) // IHM é apenas um cliente de leitura
{
    // Aloca espaço prévio para o buffer de desenho
    _buffer.resize(SHM_SIGNAL_BUFFER_SIZE, 0.0f);

    // Timeout de ~16ms para obter ~60 FPS
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &WaveformArea::on_timeout), 16);
}

WaveformArea::~WaveformArea() {}

bool WaveformArea::on_timeout() {
    // Força o gtkmm a agendar um redesenho deste widget
    auto win = get_window();
    if (win) {
        Gdk::Rectangle rect(0, 0, get_allocation().get_width(), get_allocation().get_height());
        win->invalidate_rect(rect, false);
    }
    return true; // Retornar true mantém o timer rodando
}

bool WaveformArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const double width = allocation.get_width();
    const double height = allocation.get_height();

    // 1. Fundo Escuro Constante (estilo osciloscópio / radar sonar)
    cr->set_source_rgb(0.05, 0.05, 0.1); // Azul muito escuro
    cr->paint();

    // Lê os dados mais recentes da memória compartilhada
    std::size_t head_index = 0;
    if (_shm_client.isValid()) {
        _shm_client.readSignalSamples(_buffer.data(), _buffer.size(), head_index);
    } else {
        // Sem conexão: desenha um texto vermelho avisando
        cr->set_source_rgb(0.9, 0.2, 0.2);
        cr->move_to(width / 2.0 - 100, height / 2.0);
        cr->set_font_size(16.0);
        cr->show_text("Aguardando generator_app (SHM OFF)");
        return true;
    }

    // 2. Desenha a grade (Grid) do osciloscópio
    cr->set_line_width(0.5);
    cr->set_source_rgba(0.2, 0.4, 0.2, 0.5); // Verde translúcido para a grade
    
    // Linha central horizontal (0.0)
    cr->move_to(0, height / 2.0);
    cr->line_to(width, height / 2.0);
    cr->stroke();

    // 3. Desenha a forma de onda do sinal
    cr->set_line_width(2.0);
    cr->set_source_rgb(0.2, 0.9, 0.2); // Verde bilhante estilo Sonar/Phosphor

    cr->begin_new_path();

    // A plotagem varre o eixo X da esquerda (mais antigo) para direita (mais recente)
    const double step_x = width / static_cast<double>(_buffer.size());
    
    for (std::size_t i = 0; i < _buffer.size(); ++i) {
        double x = i * step_x;
        
        // As amostras estão normalizadas entre [-1.0, 1.0]
        // Mapeia +1 para o topo da tela e -1 para o fundo.
        // Convertendo de Amplitude para Pixels:
        double y = (height / 2.0) - (_buffer[i] * (height / 2.5)); // 2.5 deixa uma margem no topo/base
        
        if (i == 0) {
            cr->move_to(x, y);
        } else {
            cr->line_to(x, y);
        }
    }
    
    cr->stroke();

    // 4. Desenha indicadores/textos de head_index e framerate superior
    cr->set_source_rgba(0.5, 0.8, 0.5, 0.8);
    cr->move_to(10, 20);
    cr->set_font_size(12.0);
    cr->show_text("LIVE SONAR FEED - Amostras Lidas: " + std::to_string(head_index));

    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// SonarWindow
// ──────────────────────────────────────────────────────────────────────────────

SonarWindow::SonarWindow() {
    set_title("Desafio Sonar - IHM GTKmm Oscilloscope");
    set_default_size(800, 400);

    // Adiciona a area de desenho nativa na janela
    add(_waveform_area);
    
    // Exibe todos os widgets filhos
    show_all_children();
}

SonarWindow::~SonarWindow() {}

} // namespace sonar
