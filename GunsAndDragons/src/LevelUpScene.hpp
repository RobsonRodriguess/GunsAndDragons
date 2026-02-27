#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>

// ========================================================
// PARTICLE - Magia dourada/roxa (igual sua arte)
// ========================================================
class LevelUpParticle {
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    int lifetime, maxLifetime;

    LevelUpParticle(sf::Vector2f pos, sf::Color color) {
        shape.setSize({6.5f, 6.5f});
        shape.setFillColor(color);
        shape.setPosition(pos);

        float angle = (std::rand() % 360) * 3.14159265f / 180.f;
        float speed = 1.6f + (std::rand() % 70) / 25.f;

        velocity = {std::cos(angle) * speed, std::sin(angle) * speed - 2.2f};
        maxLifetime = 55 + std::rand() % 45;
        lifetime = maxLifetime;
    }

    bool update() {
        shape.move(velocity);
        velocity.y += 0.045f;
        lifetime--;

        sf::Color c = shape.getFillColor();
        c.a = static_cast<std::uint8_t>(255 * (static_cast<float>(lifetime) / maxLifetime));
        shape.setFillColor(c);

        return lifetime <= 0;
    }
};

// ========================================================
// LEVEL UP SCENE - EXATAMENTE IGUAL À SUA PRINT + COMPILAÇÃO PERFEITA
// ========================================================
class LevelUpScene {
private:
    sf::Text title, titleShadow;
    sf::RectangleShape overlay;

    struct Card {
        sf::RectangleShape mainRect;
        sf::RectangleShape border;
        sf::RectangleShape accent;        // barra roxa esquerda
        sf::Text titleText;
        sf::Text descText;
        int type;
        float hover = 0.f;
        Card(const sf::Font& f) : titleText(f), descText(f) {}
    };

    std::vector<Card> cards;
    std::vector<LevelUpParticle> particles;
    const sf::Font& font;

    float timer = 0.f;

public:
    LevelUpScene(const sf::Font& f) 
        : font(f), title(f), titleShadow(f)
    {
        overlay.setSize({1280.f, 720.f});
        overlay.setFillColor(sf::Color(8, 4, 28, 235));

        // TÍTULO IGUALZINHO DA SUA PRINT
        title.setString("EVOLUCAO!");
        title.setCharacterSize(105);
        title.setFillColor(sf::Color(255, 230, 0));
        title.setOutlineColor(sf::Color(140, 0, 255));
        title.setOutlineThickness(14.f);
        title.setStyle(sf::Text::Bold);
        title.setPosition({640.f, 58.f});
        title.setOrigin({title.getLocalBounds().size.x / 2.f, 0.f});

        titleShadow = title;
        titleShadow.setFillColor(sf::Color(0, 0, 0, 160));
        titleShadow.setOutlineThickness(0.f);
        titleShadow.setPosition(title.getPosition() + sf::Vector2f(9.f, 11.f));
    }

    void generateCards() {
        cards.clear();

        struct Option { std::string t; std::string d; int type; };
        std::vector<Option> opts = {
            {"RECARGA", "-35% Tempo de Recarga", 2},
            {"TANK",   "+100 HP Maximo", 4},
            {"TITAN",  "Fica grande", 5}
        };

        for (int i = 0; i < 3; i++) {
            Card c(font);
            c.type = opts[i].type;

            float x = 165.f + i * 340.f;

            // Borda amarela grossa
            c.border.setSize({292.f, 412.f});
            c.border.setPosition({x - 6.f, 245.f});
            c.border.setFillColor(sf::Color::Transparent);
            c.border.setOutlineColor(sf::Color(255, 215, 0));
            c.border.setOutlineThickness(10.f);

            // Corpo roxo escuro (igual sua arte)
            c.mainRect.setSize({280.f, 400.f});
            c.mainRect.setPosition({x, 250.f});
            c.mainRect.setFillColor(sf::Color(32, 18, 62, 255));

            // Barra roxa esquerda
            c.accent.setSize({14.f, 370.f});
            c.accent.setPosition({x + 8.f, 265.f});
            c.accent.setFillColor(sf::Color(170, 50, 255, 0));

            // Textos
            c.titleText.setString(opts[i].t);
            c.titleText.setCharacterSize(37);
            c.titleText.setFillColor(sf::Color(255, 235, 90));
            c.titleText.setStyle(sf::Text::Bold);
            c.titleText.setPosition({x + 28.f, 272.f});

            c.descText.setString(opts[i].d);
            c.descText.setCharacterSize(23);
            c.descText.setFillColor(sf::Color(205, 205, 255));
            c.descText.setPosition({x + 28.f, 340.f});

            cards.push_back(std::move(c));
        }
    }

    int update(sf::RenderWindow& window, bool mouseClicked) {
        timer += 0.016f;
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        // Partículas mágicas
        if (std::rand() % 4 == 0) {
            sf::Color col = (std::rand() % 2) ? sf::Color(255, 220, 80, 255) : sf::Color(170, 70, 255, 255);
            particles.emplace_back(sf::Vector2f(80.f + std::rand() % 1120, 140.f + std::rand() % 400), col);
        }
        for (size_t i = 0; i < particles.size(); ) {
            if (particles[i].update()) particles.erase(particles.begin() + i);
            else ++i;
        }

        int chosen = -1;

        for (int i = 0; i < 3; i++) {
            Card& c = cards[i];
            bool hover = c.mainRect.getGlobalBounds().contains(mouse);

            if (hover) {
                c.hover = 1.0f;
                c.accent.setFillColor(sf::Color(170, 50, 255, 255));
                if (mouseClicked) chosen = i;
            } else {
                c.hover = 0.0f;
                c.accent.setFillColor(sf::Color(170, 50, 255, 0));
            }

            float scale = 1.0f + c.hover * 0.085f;
            c.mainRect.setScale({scale, scale});
            c.border.setScale({scale, scale});
            c.accent.setScale({scale, scale});
            c.titleText.setScale({scale, scale});
            c.descText.setScale({scale, scale});
        }

        return chosen;
    }

    // === CORRIGIDO: agora aceita exatamente os 3 parâmetros que o main.cpp chama ===
    void draw(sf::RenderWindow& window, sf::Sprite /*playerSpr*/, sf::Vector2f /*pScale*/) {
        window.draw(overlay);

        for (auto& p : particles) window.draw(p.shape);

        window.draw(titleShadow);
        window.draw(title);

        for (auto& c : cards) {
            window.draw(c.border);
            window.draw(c.mainRect);
            window.draw(c.accent);
            window.draw(c.titleText);
            window.draw(c.descText);
        }
    }

    int getCardType(int idx) {
        return (idx >= 0 && idx < 3) ? cards[idx].type : -1;
    }
};