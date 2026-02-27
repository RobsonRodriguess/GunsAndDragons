#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

class CharSelect {
private:
    sf::Text title, p1Name, p2Name, confirmText;
    sf::Sprite bgSpr, hunterSpr, halkSpr; 
    sf::RectangleShape darkOverlay;
    int selected; 
    float time;

public:
    CharSelect(const sf::Texture& bgTex, const sf::Texture& t1, const sf::Texture& t2, const sf::Font& font) : 
        title(font), p1Name(font), p2Name(font), confirmText(font), bgSpr(bgTex), hunterSpr(t1), halkSpr(t2) 
    {
        sf::Vector2u texSize = bgTex.getSize(); 
        bgSpr.setScale(sf::Vector2f({1280.f / texSize.x, 720.f / texSize.y})); 
        
        darkOverlay.setSize(sf::Vector2f({1280.f, 720.f}));
        darkOverlay.setFillColor(sf::Color(0, 0, 0, 180)); 

        title.setString("SELECIONE SEU GUERREIRO"); title.setCharacterSize(50); title.setFillColor(sf::Color::White); title.setPosition(sf::Vector2f({300.f, 50.f}));
        p1Name.setString("HUNTER"); p1Name.setCharacterSize(30); p1Name.setPosition(sf::Vector2f({350.f, 450.f}));
        p2Name.setString("HALK"); p2Name.setCharacterSize(30); p2Name.setPosition(sf::Vector2f({850.f, 450.f}));
        confirmText.setString("CLIQUE NO PERSONAGEM PARA CONFIRMAR"); confirmText.setCharacterSize(25); confirmText.setFillColor(sf::Color::Yellow); confirmText.setPosition(sf::Vector2f({350.f, 650.f}));
        
        hunterSpr.setOrigin(sf::Vector2f({t1.getSize().x/2.f, t1.getSize().y/2.f})); 
        hunterSpr.setPosition(sf::Vector2f({400.f, 300.f})); 
        hunterSpr.setScale(sf::Vector2f({0.6f, 0.6f}));
        
        // HALK CORRIGIDO: Escala aumentada de 0.18f para 0.7f!
        halkSpr.setOrigin(sf::Vector2f({t2.getSize().x/2.f, t2.getSize().y/2.f})); 
        halkSpr.setPosition(sf::Vector2f({880.f, 300.f})); 
        halkSpr.setScale(sf::Vector2f({0.7f, 0.7f})); 
        halkSpr.setRotation(sf::degrees(180.f));
        
        selected = 0; time = 0.f;
    }

    int update(sf::RenderWindow& win, bool mouseClicked) {
        time += 0.05f; sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
        if (hunterSpr.getGlobalBounds().contains(m)) { selected = 1; hunterSpr.setColor(sf::Color::White); if (mouseClicked) return 1; } else hunterSpr.setColor(sf::Color(150, 150, 150));
        if (halkSpr.getGlobalBounds().contains(m)) { selected = 2; halkSpr.setColor(sf::Color::White); if (mouseClicked) return 2; } else halkSpr.setColor(sf::Color(150, 150, 150));
        return 0;
    }

    void draw(sf::RenderWindow& win) {
        win.draw(bgSpr); 
        win.draw(darkOverlay);
        win.draw(title); win.draw(hunterSpr); win.draw(halkSpr); win.draw(p1Name); win.draw(p2Name);
        if(std::sin(time*2.f) > 0) win.draw(confirmText);
    }
};