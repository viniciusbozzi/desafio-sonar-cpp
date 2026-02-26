#include "sonar_window.hpp"
#include <gtkmm/application.h>

int main(int argc, char *argv[]) {
    // Inicializa o loop principal do aplicativo GTK
    auto app = Gtk::Application::create(argc, argv, "org.desafio.sonar.ihm");

    // Instancia a nossa janela customizada para o sonar
    sonar::SonarWindow window;

    // O método run() mostra a janela e bloqueia a execução no loop de eventos 
    // até que ela seja fechada pelo usuário, executando os timers a ~60 fps
    return app->run(window);
}
