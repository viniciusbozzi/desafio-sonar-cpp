#include <gtkmm.h>

int main(int argc, char *argv[]) {
    // Inicializa o loop principal do aplicativo GTK
    // O identificador "org.desafio.sonar.ihm" é um padrão de nomenclatura (Reverse DNS)
    auto app = Gtk::Application::create(argc, argv, "org.desafio.sonar.ihm");

    // Instancia a janela principal
    Gtk::Window window;
    
    // Configurações básicas da janela
    window.set_default_size(800, 600);
    window.set_title("Desafio Sonar - IHM Visualizador (Teste gtkmm)");

    // O método run() mostra a janela e bloqueia a execução até que ela seja fechada
    return app->run(window);
}
