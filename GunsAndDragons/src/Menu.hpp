#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <string>

// ========================================================
// PARTICLE - Versão LEVE (partículas roxas sutis como na sua arte)
// ========================================================
class Particle {
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    int lifetime, maxLifetime;

    Particle(sf::Vector2f pos, sf::Color color, float spd = 1.f) {
        shape.setSize({4.5f, 4.5f});
        shape.setFillColor(color);
        shape.setPosition(pos);

        float angle = (std::rand() % 360) * 3.14159265f / 180.f;
        float speed = ((std::rand() % 50 + 35) / 10.f) * spd;

        velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        maxLifetime = 30 + std::rand() % 25;
        lifetime = maxLifetime;
    }

    bool update() {
        shape.move(velocity);
        lifetime--;

        sf::Color c = shape.getFillColor();
        c.a = static_cast<std::uint8_t>(255 * (static_cast<float>(lifetime) / maxLifetime));
        shape.setFillColor(c);

        return lifetime <= 0;
    }
};

// ========================================================
// MAIN MENU - LIMPO, FIEL À SUA ARTE, MENOS POLUIÇÃO
// ========================================================
class MainMenu {
private:
    sf::Text title, titleShadow;
    sf::Text btnPlay, btnControls, btnExit;
    sf::Text highScoreText, insertCoinText, versionText;

    sf::Sprite bgSpr;
    sf::RectangleShape darkOverlay;
    sf::RectangleShape vignette;

    std::vector<Particle> particles;

    float time = 0.f;
    float glowTime = 0.f;
    float introTimer = 0.f;
    bool introFinished = false;

    void setupButton(sf::Text& btn, const std::string& txt, float yPos) {
        btn.setString(txt);
        btn.setCharacterSize(48);
        btn.setFillColor(sf::Color(255, 230, 180));
        btn.setOutlineColor(sf::Color(0, 0, 0));
        btn.setOutlineThickness(4.f);
        btn.setStyle(sf::Text::Bold);

        sf::FloatRect bounds = btn.getLocalBounds();
        btn.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        btn.setPosition({640.f, yPos});
    }

public:
    MainMenu(const sf::Texture& bgTex, const sf::Font& font)
        : title(font), titleShadow(font),
          btnPlay(font), btnControls(font), btnExit(font),
          highScoreText(font), insertCoinText(font), versionText(font),
          bgSpr(bgTex)
    {
        sf::Vector2u texSize = bgTex.getSize();
        bgSpr.setScale({1280.f / texSize.x, 720.f / texSize.y});

        darkOverlay.setSize({1280.f, 720.f});
        darkOverlay.setFillColor(sf::Color(0, 0, 0, 70));

        vignette.setSize({1280.f, 720.f});
        vignette.setFillColor(sf::Color(0, 0, 0, 110));

        // TÍTULO IGUALZINHO DA SUA PRINT
        title.setString("GUNS AND DRAGONS");
        title.setCharacterSize(98);
        title.setFillColor(sf::Color(255, 235, 180));
        title.setOutlineColor(sf::Color(80, 0, 0));
        title.setOutlineThickness(8.f);
        title.setStyle(sf::Text::Bold);
        title.setPosition({640.f - title.getLocalBounds().size.x / 2.f, 85.f});

        titleShadow = title;
        titleShadow.setFillColor(sf::Color(0, 0, 0, 160));
        titleShadow.setOutlineThickness(0.f);
        titleShadow.setPosition(title.getPosition() + sf::Vector2f(6.f, 8.f));

        setupButton(btnPlay, "INICIAR CACADA", 380.f);
        setupButton(btnControls, "CONTROLES", 480.f);
        setupButton(btnExit, "SAIR DO JOGO", 580.f);

        highScoreText.setString("HIGH SCORE  45280");
        highScoreText.setCharacterSize(26);
        highScoreText.setFillColor(sf::Color(255, 215, 0));
        highScoreText.setPosition({85.f, 665.f});

        insertCoinText.setString("INSERT COIN");
        insertCoinText.setCharacterSize(26);
        insertCoinText.setFillColor(sf::Color(255, 60, 60));
        insertCoinText.setPosition({520.f, 665.f});

        versionText.setString("v1.337  1987");
        versionText.setCharacterSize(22);
        versionText.setFillColor(sf::Color(100, 255, 100));
        versionText.setPosition({980.f, 667.f});
    }

    int update(sf::RenderWindow& window, bool mouseClicked) {
        float dt = 0.016f;
        time += dt;
        glowTime += dt * 2.5f;

        if (!introFinished) {
            introTimer += dt * 2.8f;
            float ease = std::min(introTimer, 1.f);
            title.setPosition({640.f - title.getLocalBounds().size.x / 2.f, 85.f - 300.f * (1.f - ease)});
            titleShadow.setPosition(title.getPosition() + sf::Vector2f(6.f, 8.f));
            if (ease >= 1.f) introFinished = true;
            return 0;
        }

        // Partículas roxas bem leves (igual sua arte)
        if (std::rand() % 7 == 0) {
            particles.emplace_back(
                sf::Vector2f(static_cast<float>(std::rand() % 1280), static_cast<float>(std::rand() % 720)),
                sf::Color(180, 50, 255, 220), 0.7f
            );
        }

        for (size_t i = 0; i < particles.size(); ) {
            if (particles[i].update()) particles.erase(particles.begin() + i);
            else i++;
        }

        // HOVER
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        int clicked = 0;

        auto handle = [&](sf::Text& b, int id) {
            if (b.getGlobalBounds().contains(mouse)) {
                b.setFillColor(sf::Color(255, 180, 60));
                float scale = 1.08f + std::sin(glowTime * 6.f) * 0.04f;
                b.setScale({scale, scale});
                if (mouseClicked) clicked = id;
            } else {
                b.setFillColor(sf::Color(255, 230, 180));
                b.setScale({1.f, 1.f});
            }
        };

        handle(btnPlay, 1);
        handle(btnControls, 2);
        handle(btnExit, 3);

        return clicked;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(bgSpr);
        window.draw(darkOverlay);

        // scanlines suaves
        for (int y = 0; y < 720; y += 6) {
            sf::RectangleShape line({1280.f, 2.f});
            line.setPosition({0.f, static_cast<float>(y)});
            line.setFillColor(sf::Color(0, 0, 0, 18));
            window.draw(line);
        }

        for (auto& p : particles) window.draw(p.shape);

        window.draw(titleShadow);
        window.draw(title);

        window.draw(btnPlay);
        window.draw(btnControls);
        window.draw(btnExit);

        window.draw(highScoreText);
        if (static_cast<int>(time * 6.f) % 2 == 0) window.draw(insertCoinText);
        window.draw(versionText);

        window.draw(vignette);
    }
};

// ========================================================
// CONTROLS SCREEN - PAINEL NEON FODA + LAYOUT LIMPO
// ========================================================
class ControlsScreen {
private:
    sf::Sprite bgSpr;
    sf::RectangleShape overlay;
    sf::RectangleShape panel;
    sf::Text title;
    std::vector<sf::Text> lines;
    sf::Text backText;

    std::vector<Particle> particles;
    float time = 0.f;

public:
    ControlsScreen(const sf::Texture& bgTex, const sf::Font& font)
        : bgSpr(bgTex),
          title(font),      // ← CORRIGIDO PARA SFML 3.0
          backText(font)    // ← CORRIGIDO PARA SFML 3.0
    {
        sf::Vector2u texSize = bgTex.getSize();
        bgSpr.setScale({1280.f / texSize.x, 720.f / texSize.y});

        overlay.setSize({1280.f, 720.f});
        overlay.setFillColor(sf::Color(0, 0, 0, 160));

        panel.setSize({760.f, 480.f});
        panel.setPosition({260.f, 120.f});
        panel.setFillColor(sf::Color(20, 15, 35, 240));
        panel.setOutlineColor(sf::Color(180, 80, 255));
        panel.setOutlineThickness(8.f);

        title.setString("CONTROLES");
        title.setCharacterSize(72);
        title.setFillColor(sf::Color(255, 180, 60));
        title.setOutlineColor(sf::Color(80, 0, 120));
        title.setOutlineThickness(6.f);
        title.setStyle(sf::Text::Bold);
        title.setPosition({640.f - title.getLocalBounds().size.x / 2.f, 155.f});

        // LINHAS DE CONTROLE
        std::vector<std::string> controls = {
            "[W A S D] - Mover",
            "[MOUSE] - Atirar",
            "[ESPAÇO] - Dash",
            "[F] - ROTAÇÃO",
            "[Q] - ESPECIAL",
            "[R] - SINALIZADOR",
            "[TAB] - VOLTAR"
        };

        float y = 260.f;
        for (const auto& txt : controls) {
            sf::Text line(font);               // ← já passa o font (SFML 3)
            line.setString(txt);
            line.setCharacterSize(34);
            line.setFillColor(sf::Color(220, 220, 255));
            line.setPosition({340.f, y});
            lines.push_back(line);
            y += 52.f;
        }

        backText.setString("CLIQUE AQUI OU PRESSIONE TAB PARA VOLTAR");
        backText.setCharacterSize(28);
        backText.setFillColor(sf::Color(255, 100, 100));
        backText.setPosition({640.f - backText.getLocalBounds().size.x / 2.f, 615.f});
    }

    void update(float dt) {
        time += dt;

        if (std::rand() % 9 == 0) {
            particles.emplace_back(sf::Vector2f(260.f + std::rand() % 760, 120.f + std::rand() % 480),
                                   sf::Color(160, 60, 255, 180), 0.6f);
        }

        for (size_t i = 0; i < particles.size(); ) {
            if (particles[i].update()) particles.erase(particles.begin() + i);
            else i++;
        }
    }

    bool isBackPressed(sf::Vector2f mousePos, bool mouseClicked) {
        return mouseClicked && backText.getGlobalBounds().contains(mousePos);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(bgSpr);
        window.draw(overlay);
        window.draw(panel);

        for (auto& p : particles) window.draw(p.shape);

        window.draw(title);
        for (auto& line : lines) window.draw(line);
        window.draw(backText);
    }
};