#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <string>
#include <deque>
#include <algorithm>

#include "Menu.hpp"
#include "CharSelect.hpp"
#include "LevelUpScene.hpp"

const float PI = 3.14159265f;

enum GameState {
    STATE_MENU,
    STATE_CHAR_SELECT,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_CONTROLS,
    STATE_LEVEL_UP
};

enum EnemyType {
    E_NORMAL,
    E_FAST,
    E_ARMORED,
    E_COLOSSAL,
    E_TOXIC,
    E_GHOST,
    E_ICE,
    E_SUMMONER,
    E_NECROMANCER,
    E_WORM
};

float getDistance(sf::Vector2f p1, sf::Vector2f p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

float distToSegment(sf::Vector2f p, sf::Vector2f v, sf::Vector2f w) {
    float l2 = (v.x - w.x) * (v.x - w.x) + (v.y - w.y) * (v.y - w.y);
    if (l2 == 0.0f) {
        return getDistance(p, v);
    }
    float t = std::max(0.0f, std::min(1.0f, ((p.x - v.x) * (w.x - v.x) + (p.y - v.y) * (w.y - v.y)) / l2));
    sf::Vector2f projection = sf::Vector2f({v.x + t * (w.x - v.x), v.y + t * (w.y - v.y)});
    return getDistance(p, projection);
}

sf::Vector2f getScreenShake(float& shakeTimer) {
    if (shakeTimer > 0) {
        shakeTimer -= 1.0f;
        float ox = (std::rand() % 10 - 5) * (shakeTimer / 10.f);
        float oy = (std::rand() % 10 - 5) * (shakeTimer / 10.f);
        return sf::Vector2f({ox, oy});
    }
    return sf::Vector2f({0.f, 0.f});
}

class AudioManager {
public:
    sf::Music bgm;
    sf::SoundBuffer shootBuf;
    sf::SoundBuffer explodeBuf;
    sf::SoundBuffer levelBuf;
    std::vector<sf::Sound> activeSounds;

    bool init() {
        bool ok = true;
        if (!bgm.openFromFile("../assets/music.ogg")) ok = false;
        if (!shootBuf.loadFromFile("../assets/shoot.wav")) ok = false;
        bgm.setLooping(true);
        bgm.setVolume(20.f);
        return ok;
    }

    void playSound(const sf::SoundBuffer& buffer, float volume = 50.f, float pitch = 1.f) {
        activeSounds.erase(std::remove_if(activeSounds.begin(), activeSounds.end(),
            [](const sf::Sound& s) { return s.getStatus() == sf::Sound::Status::Stopped; }),
            activeSounds.end());
        sf::Sound sound(buffer);
        sound.setVolume(volume);
        sound.setPitch(pitch);
        sound.play();
        activeSounds.push_back(sound);
    }
};

class AchievementManager {
public:
    struct Ach {
        std::string name;
        bool unlocked;
        std::string desc;
    };
    
    std::vector<Ach> achs;
    std::deque<std::string> queue;
    float displayTimer;

    AchievementManager() {
        achs.push_back({"PRIMEIRO SANGUE", false, ""});
        achs.push_back({"EXTERMINADOR", false, ""});
        achs.push_back({"A GRANDE MURALHA", false, ""});
        achs.push_back({"COLOSSAL CAIDO", false, ""});
        achs.push_back({"PIROMANIACO", false, ""});
        achs.push_back({"INTOCAVEL", false, ""});
        achs.push_back({"LENDARIO", false, ""});
        displayTimer = 0;
    }

    void check(int kills, int bosses, int combo, float time) {
        if (!achs[0].unlocked && kills >= 1) unlock(0);
        if (!achs[1].unlocked && kills >= 100) unlock(1);
        if (!achs[2].unlocked && kills >= 500) unlock(2);
        if (!achs[3].unlocked && bosses >= 1) unlock(3);
        if (!achs[5].unlocked && time >= 300.f) unlock(5);
        if (!achs[6].unlocked && combo >= 50) unlock(6);
    }

    void unlock(int id) {
        if (!achs[id].unlocked) {
            achs[id].unlocked = true;
            queue.push_back("CONQUISTA: " + achs[id].name);
        }
    }

    void render(sf::RenderWindow& win, const sf::Font& font) {
        if (displayTimer > 0) {
            displayTimer -= 1.0f;
            sf::RectangleShape bg(sf::Vector2f({450.f, 60.f}));
            bg.setFillColor(sf::Color(255, 200, 0, 240));
            bg.setOutlineThickness(3.f);
            bg.setOutlineColor(sf::Color::White);
            
            float yPos = (displayTimer > 160) ? -60.f + (180.f - displayTimer) * 4.f : ((displayTimer < 20) ? displayTimer * 3.f : 20.f);
            bg.setPosition(sf::Vector2f({800.f, yPos}));
            
            sf::Text t(font, queue.front(), 22);
            t.setFillColor(sf::Color::Black);
            t.setPosition(sf::Vector2f({bg.getPosition().x + 20.f, bg.getPosition().y + 15.f}));
            
            win.draw(bg);
            win.draw(t);
        } else if (!queue.empty()) {
            displayTimer = 180.f;
        }
        
        if (displayTimer <= 0 && !queue.empty()) {
            queue.pop_front();
        }
    }
};

// A CLASSE PARTICLE FOI REMOVIDA DAQUI POIS JÁ EXISTE NO MENU.HPP!

class FloatingText {
public:
    sf::Text txt;
    int lifetime;
    sf::Vector2f velocity;

    FloatingText(sf::Vector2f pos, std::string msg, const sf::Font& font, sf::Color color, int size = 20) : txt(font) {
        txt.setString(msg);
        txt.setCharacterSize(size);
        txt.setFillColor(color);
        txt.setOutlineColor(sf::Color::Black);
        txt.setOutlineThickness(2.f);
        
        float ox = (std::rand() % 40) - 20.f;
        float oy = (std::rand() % 40) - 20.f;
        txt.setPosition(sf::Vector2f({pos.x + ox, pos.y + oy}));
        velocity = sf::Vector2f({0.f, -2.5f});
        lifetime = 45;
    }

    bool update() {
        txt.move(velocity);
        lifetime--;
        
        sf::Color c = txt.getFillColor();
        sf::Color o = txt.getOutlineColor();
        c.a = static_cast<std::uint8_t>(255 * ((float)lifetime / 45));
        o.a = c.a;
        
        txt.setFillColor(c);
        txt.setOutlineColor(o);
        
        return lifetime <= 0;
    }
};

class DashTrail {
public:
    sf::Sprite sprite;
    int lifetime;

    DashTrail(const sf::Texture& tex, sf::Vector2f pos, sf::Angle rot, sf::Vector2f scale) : sprite(tex) {
        sprite.setOrigin(sf::Vector2f({tex.getSize().x / 2.f, tex.getSize().y / 2.f}));
        sprite.setPosition(pos);
        sprite.setRotation(rot);
        sprite.setScale(scale);
        sprite.setColor(sf::Color(150, 150, 255, 100));
        lifetime = 15;
    }

    bool update() {
        lifetime--;
        sf::Color c = sprite.getColor();
        c.a = static_cast<std::uint8_t>(100 * ((float)lifetime / 15));
        sprite.setColor(c);
        return lifetime <= 0;
    }
};

class BloodDecal {
public:
    sf::CircleShape shape;

    BloodDecal(sf::Vector2f pos) {
        shape.setRadius((std::rand() % 15 + 10) / 1.f);
        shape.setOrigin(sf::Vector2f({shape.getRadius(), shape.getRadius()}));
        shape.setPosition(pos);
        shape.setFillColor(sf::Color(100 + (std::rand() % 50), 0, 0, 180));
        shape.setScale(sf::Vector2f({1.f, 0.5f}));
        shape.setRotation(sf::degrees(std::rand() % 360));
    }
};

class RainDrop {
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;

    RainDrop(sf::Vector2f camCenter) {
        shape.setSize(sf::Vector2f({2.f, 35.f}));
        shape.setFillColor(sf::Color(150, 150, 255, 120));
        float rx = camCenter.x + (std::rand() % 4000 - 2000);
        float ry = camCenter.y - 1200.f - (std::rand() % 800);
        shape.setPosition(sf::Vector2f({rx, ry}));
        velocity = sf::Vector2f({-10.f, 45.f + (std::rand() % 20)});
        shape.setRotation(sf::degrees(12.f));
    }

    bool update(sf::Vector2f camCenter) {
        shape.move(velocity);
        return (shape.getPosition().y > camCenter.y + 1200.f);
    }
};

class Bullet {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    sf::Vector2f startPos;
    float damage;
    bool piercing;
    bool isEnemyBullet;
    int type;
    float maxDist;
    float animTimer;

    Bullet(sf::Vector2f pos, sf::Vector2f dir, float speed, float dmg, bool pierce, float sz, sf::Color color, int t = 0, bool enemy = false) {
        shape.setRadius(sz);
        shape.setFillColor(color);
        shape.setOrigin(sf::Vector2f({sz, sz}));
        shape.setPosition(pos);
        velocity = dir * speed;
        damage = dmg;
        piercing = pierce;
        type = t;
        startPos = pos;
        isEnemyBullet = enemy;
        animTimer = 0.f;
        
        if (t == 5) {
            maxDist = 400.f;
        } else {
            maxDist = 2500.f;
        }
    }
    
    void updateAnim() {
        animTimer += 0.2f;
        if (type == 5) {
            float s = 1.f + std::sin(animTimer) * 0.5f;
            shape.setScale(sf::Vector2f({s, s}));
        }
    }
};

class LaserBeam {
public:
    sf::RectangleShape shape;
    int lifetime;

    LaserBeam(sf::Vector2f start, sf::Vector2f end) {
        float len = getDistance(start, end);
        float ang = std::atan2(end.y - start.y, end.x - start.x) * 180.f / PI;
        shape.setSize(sf::Vector2f({len, 12.f}));
        shape.setOrigin(sf::Vector2f({0.f, 6.f}));
        shape.setPosition(start);
        shape.setRotation(sf::degrees(ang));
        shape.setFillColor(sf::Color::Red);
        lifetime = 30;
    }

    bool update() {
        lifetime--;
        sf::Color c = shape.getFillColor();
        c.a = static_cast<std::uint8_t>(255 * ((float)lifetime / 30));
        shape.setFillColor(c);
        return lifetime <= 0;
    }
};

class Mine {
public:
    sf::CircleShape shape;
    sf::CircleShape pulse;
    sf::Vector2f pos;
    float timer;

    Mine(sf::Vector2f p) {
        pos = p;
        timer = 0;
        shape.setRadius(12.f);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(sf::Vector2f({12.f, 12.f}));
        shape.setPosition(pos);
        
        pulse.setRadius(12.f);
        pulse.setFillColor(sf::Color::Transparent);
        pulse.setOutlineThickness(3.f);
        pulse.setOutlineColor(sf::Color::Red);
        pulse.setOrigin(sf::Vector2f({12.f, 12.f}));
        pulse.setPosition(pos);
    }

    void update() {
        timer += 0.15f;
        pulse.setRadius(12.f + std::sin(timer) * 8.f);
        pulse.setOrigin(sf::Vector2f({pulse.getRadius(), pulse.getRadius()}));
    }
};

class Meteor {
public:
    sf::Vector2f pos;
    int timer;
    sf::CircleShape shadow;

    Meteor(sf::Vector2f p) : pos(p) {
        timer = 120;
        shadow.setFillColor(sf::Color(255, 0, 0, 100));
    }

    bool update() {
        timer--;
        float s = 200.f - timer * 1.5f;
        if (s < 0.f) s = 0.f;
        shadow.setRadius(s);
        shadow.setOrigin(sf::Vector2f({s, s}));
        shadow.setPosition(pos);
        return timer <= 0;
    }
};

class BlackHole {
public:
    sf::Vector2f pos;
    int timer;
    sf::CircleShape core;
    sf::CircleShape aura;

    BlackHole(sf::Vector2f p) : pos(p) {
        timer = 240;
        core.setRadius(10.f);
        core.setFillColor(sf::Color::Black);
        aura.setRadius(30.f);
        aura.setFillColor(sf::Color(150, 0, 255, 150));
    }

    bool update() {
        timer--;
        aura.setRadius(30.f + std::sin(timer * 0.5f) * 15.f);
        aura.setOrigin(sf::Vector2f({aura.getRadius(), aura.getRadius()}));
        aura.setPosition(pos);
        
        core.setOrigin(sf::Vector2f({10.f, 10.f}));
        core.setPosition(pos);
        
        return timer <= 0;
    }
};

class FlareSignal {
public:
    sf::Vector2f pos;
    int timer;
    sf::CircleShape glow;
    sf::RectangleShape beam;

    FlareSignal(sf::Vector2f p) {
        pos = p;
        timer = 300;
        glow.setRadius(60.f);
        glow.setFillColor(sf::Color(0, 255, 0, 100));
        glow.setOrigin(sf::Vector2f({60.f, 60.f}));
        glow.setPosition(pos);
        
        beam.setSize(sf::Vector2f({8.f, 2000.f}));
        beam.setOrigin(sf::Vector2f({4.f, 2000.f}));
        beam.setPosition(pos);
        beam.setFillColor(sf::Color(0, 255, 0, 150));
    }

    bool update() {
        timer--;
        glow.setRadius(60.f + std::sin(timer * 0.2f) * 15.f);
        glow.setOrigin(sf::Vector2f({glow.getRadius(), glow.getRadius()}));
        
        beam.setSize(sf::Vector2f({8.f + std::cos(timer * 0.5f) * 4.f, 2000.f}));
        beam.setOrigin(sf::Vector2f({beam.getSize().x / 2.f, 2000.f}));
        
        return timer <= 0;
    }
};

class Pickup {
public:
    sf::CircleShape shape;
    int type;
    int lifetime;
    float animTimer;

    Pickup(sf::Vector2f pos, int t) {
        type = t;
        shape.setRadius(16.f);
        shape.setOrigin(sf::Vector2f({16.f, 16.f}));
        shape.setPosition(pos);
        shape.setOutlineThickness(3.f);
        shape.setOutlineColor(sf::Color::White);
        
        if (type == 0) shape.setFillColor(sf::Color::Green);
        else if (type == 1) shape.setFillColor(sf::Color::Cyan);
        else if (type == 2) shape.setFillColor(sf::Color::Yellow);
        else if (type == 3) shape.setFillColor(sf::Color::Magenta);
        else if (type == 4) shape.setFillColor(sf::Color(255, 100, 0));
        else if (type == 5) shape.setFillColor(sf::Color(255, 215, 0));
        else if (type == 6) shape.setFillColor(sf::Color::White);
        
        lifetime = 1500;
        animTimer = 0.f;
    }

    bool update() {
        lifetime--;
        animTimer += 0.1f;
        float scale = 1.f + 0.15f * std::sin(animTimer);
        shape.setScale(sf::Vector2f({scale, scale}));
        
        sf::Color c = shape.getFillColor();
        if (lifetime < 180 && (lifetime / 10) % 2 == 0) {
            c.a = 50;
        } else {
            c.a = 220;
        }
        shape.setFillColor(c);
        
        return lifetime <= 0;
    }
};

class Pet {
public:
    sf::Sprite sprite;
    sf::Vector2f pos;
    int fireCooldown;
    float angle;
    float animTimer;

    Pet(const sf::Texture& tex) : sprite(tex) {
        sprite.setOrigin(sf::Vector2f({tex.getSize().x / 2.f, tex.getSize().y / 2.f}));
        sprite.setScale(sf::Vector2f({0.18f, 0.18f}));
        sprite.setColor(sf::Color::Cyan);
        pos = sf::Vector2f({0.f, 0.f});
        fireCooldown = 0;
        angle = 0.f;
        animTimer = 0.f;
    }

    void update(sf::Vector2f playerPos, std::vector<Bullet>& bullets, sf::Vector2f targetPos, bool hasTarget) {
        angle += 0.05f;
        animTimer += 0.2f;
        
        sf::Vector2f targetOrbit = sf::Vector2f({playerPos.x + std::cos(angle) * 80.f, playerPos.y + std::sin(angle) * 80.f});
        pos.x += (targetOrbit.x - pos.x) * 0.15f;
        pos.y += (targetOrbit.y - pos.y) * 0.15f;
        
        float scaleMod = 0.18f + std::sin(animTimer) * 0.02f;
        sprite.setScale(sf::Vector2f({scaleMod, scaleMod}));
        sprite.setPosition(pos);
        
        if (fireCooldown > 0) {
            fireCooldown--;
        }
        
        if (hasTarget) {
            sf::Vector2f dir = targetPos - pos;
            float rotAngle = std::atan2(dir.y, dir.x) * 180.f / PI;
            sprite.setRotation(sf::degrees(rotAngle - 90.f));
            
            if (fireCooldown <= 0) {
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len != 0.f) dir /= len;
                bullets.push_back(Bullet(pos, dir, 25.f, 15.f, false, 6.f, sf::Color::Cyan, 7, false));
                fireCooldown = 35;
            }
        } else {
            sprite.setRotation(sf::degrees(std::cos(animTimer * 0.5f) * 10.f));
        }
    }
};

class AllyTurret {
public:
    sf::Sprite sprite;
    int lifetime;
    int fireCooldown;
    float animTimer;

    AllyTurret(sf::Vector2f pos, const sf::Texture& tex) : sprite(tex) {
        sprite.setOrigin(sf::Vector2f({tex.getSize().x / 2.f, tex.getSize().y / 2.f}));
        sprite.setPosition(pos);
        sprite.setScale(sf::Vector2f({0.3f, 0.3f}));
        sprite.setColor(sf::Color(100, 200, 255));
        lifetime = 1800;
        fireCooldown = 0;
        animTimer = 0.f;
    }

    bool update(std::vector<Bullet>& bullets, sf::Vector2f targetPos, bool hasTarget, AudioManager& audio) {
        lifetime--;
        animTimer += 0.1f;
        
        float scaleMod = 0.3f + std::sin(animTimer) * 0.01f;
        sprite.setScale(sf::Vector2f({scaleMod, scaleMod}));
        
        if (fireCooldown > 0) {
            fireCooldown--;
        }
        
        if (hasTarget) {
            sf::Vector2f dir = targetPos - sprite.getPosition();
            float angle = std::atan2(dir.y, dir.x) * 180.f / PI;
            sprite.setRotation(sf::degrees(angle + 90.f));
            
            if (fireCooldown <= 0) {
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len != 0.f) dir /= len;
                bullets.push_back(Bullet(sprite.getPosition(), dir, 25.f, 12.f, false, 5.f, sf::Color::Cyan, 0, false));
                if (audio.shootBuf.getDuration().asSeconds() > 0) {
                    audio.playSound(audio.shootBuf, 15.f, 1.5f);
                }
                fireCooldown = 20;
            }
        }
        return lifetime <= 0;
    }
};

class Dragon {
public:
    sf::Sprite sprite;
    float speed;
    float hp;
    float maxHp;
    int type;
    int fireCooldown;
    float freezeTimer;
    int bossPhase;
    float animTimer;
    sf::Vector2f baseScale;

    Dragon(sf::Vector2f pos, const sf::Texture& tex, int t, float hpMult) : sprite(tex) {
        sprite.setOrigin(sf::Vector2f({tex.getSize().x / 2.f, tex.getSize().y / 2.f}));
        sprite.setPosition(pos);
        type = t;
        fireCooldown = 90;
        freezeTimer = 0.f;
        bossPhase = 1;
        animTimer = (std::rand() % 100) * 0.1f;
        
        if (type == E_COLOSSAL) {
            baseScale = sf::Vector2f({3.8f, 3.8f});
            sprite.setColor(sf::Color(255, 50, 50));
            speed = 0.9f;
            maxHp = 700.f * hpMult;
        } else if (type == E_TOXIC) {
            baseScale = sf::Vector2f({0.6f, 0.6f});
            sprite.setColor(sf::Color(50, 255, 50));
            speed = 2.2f;
            maxHp = 35.f * hpMult;
            fireCooldown = 120;
        } else if (type == E_GHOST) {
            baseScale = sf::Vector2f({0.55f, 0.55f});
            sprite.setColor(sf::Color(200, 200, 200, 80));
            speed = 8.5f;
            maxHp = 15.f * hpMult;
        } else if (type == E_ICE) {
            baseScale = sf::Vector2f({0.75f, 0.75f});
            sprite.setColor(sf::Color(50, 200, 255));
            speed = 1.6f;
            maxHp = 65.f * hpMult;
            fireCooldown = 100;
        } else if (type == E_SUMMONER) {
            baseScale = sf::Vector2f({0.85f, 0.85f});
            sprite.setColor(sf::Color(200, 50, 255));
            speed = 1.2f;
            maxHp = 90.f * hpMult;
            fireCooldown = 200;
        } else if (type == E_NECROMANCER) {
            baseScale = sf::Vector2f({0.8f, 0.8f});
            sprite.setColor(sf::Color(50, 0, 100));
            speed = 1.4f;
            maxHp = 120.f * hpMult;
            fireCooldown = 300;
        } else if (type == E_WORM) {
            baseScale = sf::Vector2f({0.7f, 0.7f});
            sprite.setColor(sf::Color(150, 100, 50));
            speed = 4.0f;
            maxHp = 40.f * hpMult;
            fireCooldown = 150;
        } else if (type == E_ARMORED) {
            baseScale = sf::Vector2f({0.95f, 0.95f});
            sprite.setColor(sf::Color(100, 100, 100));
            speed = 1.7f;
            maxHp = 90.f * hpMult;
        } else if (type == E_FAST) {
            baseScale = sf::Vector2f({0.45f, 0.45f});
            sprite.setColor(sf::Color(255, 150, 150));
            speed = 6.8f + (std::rand() % 20) / 10.f;
            maxHp = 18.f * hpMult;
        } else {
            baseScale = sf::Vector2f({0.65f, 0.65f});
            sprite.setColor(sf::Color::White);
            speed = 3.2f + (std::rand() % 15) / 10.f;
            maxHp = 25.f * hpMult;
        }
        
        sprite.setScale(baseScale);
        hp = maxHp;
    }

    void handlePhases() {
        if (type == E_COLOSSAL) {
            if (hp < maxHp * 0.3f && bossPhase == 2) {
                bossPhase = 3;
                speed = 2.0f;
                sprite.setColor(sf::Color(255, 0, 0));
                fireCooldown = 40;
            } else if (hp < maxHp * 0.6f && bossPhase == 1) {
                bossPhase = 2;
                speed = 1.4f;
                sprite.setColor(sf::Color(255, 100, 50));
                fireCooldown = 70;
            }
        }
    }
    
    void updateAnimation() {
        animTimer += 0.15f;
        float stretch = std::sin(animTimer) * 0.05f;
        float squash = std::cos(animTimer) * 0.05f;
        sprite.setScale(sf::Vector2f({baseScale.x + stretch, baseScale.y + squash}));
    }
};

class Player {
public:
    sf::Sprite sprite;
    sf::Vector2f pos;
    float maxHp;
    float hp;
    int level;
    float exp;
    float expNeeded;
    float ultCharge;
    int dashCooldown;
    int dashTime;
    sf::Vector2f dashDir;
    int meleeCooldown;
    int iFrames;
    int activeWeaponDrop;
    int dropAmmo;
    float freezeTimer;
    float dmgMult;
    float spdMult;
    float cdMult;
    bool hasPet;
    bool titanMode;
    int flares;
    float animTimer;
    sf::Vector2f baseScale;
    float titanTimer;
    int charClass;
    float rotOffset;
    
    Player(const sf::Texture& tex, int cClass = 0) : sprite(tex) {
        charClass = cClass;
        sprite.setOrigin(sf::Vector2f({tex.getSize().x / 2.f, tex.getSize().y / 2.f}));
        
        // AQUI ESTÁ A CORREÇÃO DE TAMANHO E ROTAÇÃO DO HALK!
        if (charClass == 1) {
            baseScale = sf::Vector2f({0.70f, 0.70f}); // Tamanho do Halk Corrigido
            rotOffset = 180.f; // Virado para atirar pro lado certo
            maxHp = 300.f;
            spdMult = 0.7f;
            dmgMult = 2.0f;
            sprite.setColor(sf::Color::White);
        } else {
            baseScale = sf::Vector2f({0.25f, 0.25f}); // Tamanho do Caçador Normal
            rotOffset = 0.f;
            maxHp = 100.f;
            spdMult = 1.0f;
            dmgMult = 1.0f;
            sprite.setColor(sf::Color::White);
        }
        
        sprite.setScale(baseScale);
        pos = sf::Vector2f({0.f, 0.f});
        hp = maxHp;
        level = 1;
        exp = 0.f;
        expNeeded = 50.f;
        ultCharge = 0.f;
        dashCooldown = 0;
        dashTime = 0;
        dashDir = sf::Vector2f({0.f, 0.f});
        meleeCooldown = 0;
        iFrames = 0;
        activeWeaponDrop = 0;
        dropAmmo = 0;
        freezeTimer = 0.f;
        cdMult = 1.f;
        hasPet = false;
        titanMode = false;
        flares = 3;
        animTimer = 0.f;
        titanTimer = 0.f;
    }

    void takeDamage(float dmg, float& shakeTimer, std::vector<FloatingText>& fTexts, const sf::Font& font) {
        if (dashTime > 0 || iFrames > 0 || titanMode) return;
        hp -= dmg;
        iFrames = 40;
        shakeTimer = 25.f;
        fTexts.push_back(FloatingText(pos, "-" + std::to_string((int)dmg), font, sf::Color::Red, 35));
    }

    void updateAnimationAndSprite() {
        animTimer += 0.2f;
        float stretch = 0.f;
        float squash = 0.f;
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            stretch = std::sin(animTimer) * 0.02f;
            squash = std::cos(animTimer) * 0.02f;
        }
        
        if (titanMode) {
            titanTimer--;
            baseScale = sf::Vector2f({1.3f, 1.3f}); // Fica ainda maior no titan mode
            sprite.setColor(sf::Color(255, 200, 200));
            if (titanTimer <= 0.f) {
                titanMode = false;
                baseScale = (charClass == 1) ? sf::Vector2f({0.70f, 0.70f}) : sf::Vector2f({0.25f, 0.25f});
                sprite.setColor(sf::Color::White);
            }
        } else if (freezeTimer > 0) {
            freezeTimer--;
            sprite.setColor(sf::Color(100, 200, 255));
        } else if (iFrames > 0) {
            iFrames--;
            if ((iFrames / 5) % 2 == 0) {
                sprite.setColor(sf::Color(255, 0, 0, 200));
            } else {
                sprite.setColor(sf::Color::White);
            }
        } else {
            sprite.setColor(sf::Color::White);
        }
        
        sprite.setScale(sf::Vector2f({baseScale.x + stretch, baseScale.y + squash}));
        sprite.setPosition(pos);
        
        if (meleeCooldown > 0) {
            meleeCooldown--;
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "GUNS AND DRAGONS", sf::Style::Default);
    window.setFramerateLimit(60);

    AudioManager audio;
    (void)audio.init();
    AchievementManager achManager;

    sf::Texture playerIdleTex;
    sf::Texture playerShootTex;
    sf::Texture dragonTex;
    sf::Texture telaTex;
    sf::Texture halkTex;
    
    (void)playerIdleTex.loadFromFile("../assets/standing-still.png");
    (void)playerShootTex.loadFromFile("../assets/shooting.png");
    (void)dragonTex.loadFromFile("../assets/RedDragon.png");
    (void)telaTex.loadFromFile("../assets/TELA.png");
    (void)halkTex.loadFromFile("../assets/halk.png");
    
    sf::Font font;
    (void)font.openFromFile("../assets/arial.ttf");
    
    MainMenu menuInterativo(telaTex, font);
    CharSelect charSelector(telaTex, playerIdleTex, halkTex, font);
    LevelUpScene levelUpScene(font);

    sf::View camera(sf::FloatRect({0, 0}, {1280, 720}));
    sf::View uiCamera(sf::FloatRect({0, 0}, {1280, 720}));
    
    Player player(playerIdleTex, 0);
    GameState currentState = STATE_MENU;
    Pet pet(dragonTex);

    std::vector<Bullet> bullets;
    std::vector<Dragon> dragons;
    std::vector<Particle> particles;
    std::vector<FloatingText> fTexts;
    std::vector<Pickup> pickups;
    std::vector<Mine> mines;
    std::vector<LaserBeam> lasers;
    std::deque<BloodDecal> bloodPuddles;
    std::vector<RainDrop> rainSystem;
    std::vector<AllyTurret> allies;
    std::vector<FlareSignal> flares;
    std::vector<DashTrail> dashTrails;
    std::vector<Meteor> meteors;
    std::vector<BlackHole> blackHoles;

    int fireCooldown = 0;
    int shootSpriteTimer = 0;
    float shakeTimer = 0.f;
    float dayTime = 0.f;
    
    int currentWave = 1;
    int enemiesToSpawn = 10;
    int enemiesSpawned = 0;
    int waveBreakTimer = 0;
    float enemyHealthMultiplier = 1.0f;
    
    int currentScore = 0;
    int highScore = 0;
    int currentCombo = 0;
    int comboTimer = 0;
    int totalBossesDefeated = 0;
    float playTimeSeconds = 0.f;
    int meteorTimer = 1800;

    sf::Text uiText(font);
    uiText.setCharacterSize(22);
    
    sf::Text waveAlertText(font);
    waveAlertText.setCharacterSize(70);
    waveAlertText.setFillColor(sf::Color::Red);
    waveAlertText.setPosition(sf::Vector2f({400.f, 200.f}));
    
    sf::Text gameOverText(font);
    gameOverText.setString(" VOCE MORREU.");
    gameOverText.setCharacterSize(80);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(sf::Vector2f({250.f, 250.f}));
    
    sf::Text controlsTitleText(font, "CONTROLES\n[W A S D] - Mover\n[MOUSE] - Atirar\n[ESPACO] - Dash\n[F] - LAMINAS\n[Q] - ESPECIAL\n[R] - SINALIZADOR\n[TAB] VOLTAR", 30);
    controlsTitleText.setPosition(sf::Vector2f({200.f, 100.f}));

    sf::Sprite gameBg(telaTex);
    sf::Vector2u tSize = telaTex.getSize();
    gameBg.setScale(sf::Vector2f({6000.f / tSize.x, 6000.f / tSize.y}));
    gameBg.setPosition(sf::Vector2f({-3000.f, -3000.f}));
    gameBg.setColor(sf::Color(100, 100, 100)); // Fundo mais visível

    if (audio.bgm.getDuration().asSeconds() > 0) {
        audio.bgm.play();
    }

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (currentState == STATE_GAME_OVER && keyEvent->code == sf::Keyboard::Key::Space) {
                    currentState = STATE_MENU;
                }
                if (currentState == STATE_MENU && keyEvent->code == sf::Keyboard::Key::Tab) {
                    currentState = STATE_CONTROLS;
                }
                if (currentState == STATE_CONTROLS && keyEvent->code == sf::Keyboard::Key::Tab) {
                    currentState = STATE_MENU;
                }

                if (currentState == STATE_PLAYING) {
                    if (keyEvent->code == sf::Keyboard::Key::Space && player.dashCooldown <= 0 && player.freezeTimer <= 0 && !player.titanMode) {
                        player.dashTime = 12;
                        player.dashCooldown = 60 * player.cdMult;
                        player.dashDir = sf::Vector2f({0.f, 0.f});
                        
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) player.dashDir.y -= 1.f;
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) player.dashDir.y += 1.f;
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) player.dashDir.x -= 1.f;
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) player.dashDir.x += 1.f;
                        
                        float length = std::sqrt(player.dashDir.x * player.dashDir.x + player.dashDir.y * player.dashDir.y);
                        if (length != 0.f) player.dashDir /= length;
                        
                        if (audio.shootBuf.getDuration().asSeconds() > 0) {
                            audio.playSound(audio.shootBuf, 20.f, 0.5f);
                        }
                        achManager.unlock(5);
                    }
                    
                    if (keyEvent->code == sf::Keyboard::Key::F && player.meleeCooldown <= 0 && player.freezeTimer <= 0 && !player.titanMode) {
                        player.meleeCooldown = 90 * player.cdMult;
                        shakeTimer = 15.f;
                        
                        for (int pt = 0; pt < 50; pt++) {
                            particles.push_back(Particle(player.pos, sf::Color::White, 4.f));
                        }
                        
                        if (audio.shootBuf.getDuration().asSeconds() > 0) {
                            audio.playSound(audio.shootBuf, 80.f, 2.5f);
                        }
                        
                        for (auto& dragon : dragons) {
                            if (getDistance(player.pos, dragon.sprite.getPosition()) < 220.f) {
                                dragon.hp -= 150.f * player.dmgMult;
                                fTexts.push_back(FloatingText(dragon.sprite.getPosition(), std::to_string((int)(150 * player.dmgMult)), font, sf::Color::White, 35));
                                
                                sf::Vector2f pushDir = dragon.sprite.getPosition() - player.pos;
                                float pushLen = std::sqrt(pushDir.x * pushDir.x + pushDir.y * pushDir.y);
                                if (pushLen > 0.f) {
                                    dragon.sprite.move((pushDir / pushLen) * 150.f);
                                }
                                
                                for (int pt = 0; pt < 15; pt++) {
                                    particles.push_back(Particle(dragon.sprite.getPosition(), sf::Color::Red));
                                }
                            }
                        }
                    }
                    
                    if (keyEvent->code == sf::Keyboard::Key::Q && player.ultCharge >= 100 && player.freezeTimer <= 0 && !player.titanMode) {
                        player.ultCharge = 0;
                        shakeTimer = 60.f;
                        fTexts.push_back(FloatingText(player.pos, "PODER MAXIMO!", font, sf::Color::Red, 50));
                        
                        for (auto& dragon : dragons) {
                            dragon.hp -= 500.f * player.dmgMult;
                            fTexts.push_back(FloatingText(dragon.sprite.getPosition(), std::to_string((int)(500 * player.dmgMult)), font, sf::Color::Cyan, 40));
                        }
                        
                        for (int pt = 0; pt < 300; pt++) {
                            particles.push_back(Particle(player.pos, sf::Color::Cyan, 3.5f));
                        }
                        
                        if (audio.shootBuf.getDuration().asSeconds() > 0) {
                            audio.playSound(audio.shootBuf, 100.f, 0.2f);
                        }
                    }
                    
                    if (keyEvent->code == sf::Keyboard::Key::R && player.flares > 0 && player.freezeTimer <= 0 && !player.titanMode) {
                        player.flares--;
                        flares.push_back(FlareSignal(player.pos));
                        fTexts.push_back(FloatingText(player.pos, "SINALIZADOR DISPARADO!", font, sf::Color::Green, 30));
                    }
                }
            }
        }

        if (currentState == STATE_MENU) {
            int action = menuInterativo.update(window, sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
            if (action == 1) {
                currentState = STATE_CHAR_SELECT;
            } else if (action == 2) {
                currentState = STATE_CONTROLS;
            } else if (action == 3) {
                window.close();
            }
        } else if (currentState == STATE_CHAR_SELECT) {
            int ch = charSelector.update(window, sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
            if (ch > 0) {
                currentState = STATE_PLAYING;
                
                if (ch == 1) {
                    player = Player(playerIdleTex, 0);
                } else {
                    player = Player(halkTex, 1);
                }
                
                currentScore = 0;
                currentCombo = 0;
                currentWave = 1;
                enemiesToSpawn = 15;
                enemiesSpawned = 0;
                enemyHealthMultiplier = 1.0f;
                playTimeSeconds = 0.f;
                totalBossesDefeated = 0;
                meteorTimer = 1800;
                
                dragons.clear();
                bullets.clear();
                particles.clear();
                fTexts.clear();
                pickups.clear();
                mines.clear();
                lasers.clear();
                bloodPuddles.clear();
                rainSystem.clear();
                flares.clear();
                allies.clear();
                dashTrails.clear();
                meteors.clear();
                blackHoles.clear();
                dayTime = 0.f;
            }
        } else if (currentState == STATE_LEVEL_UP) {
            int cardIdx = levelUpScene.update(window, sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
            if (cardIdx != -1) {
                int type = levelUpScene.getCardType(cardIdx);
                if (type == 0) player.dmgMult += 0.35f;
                else if (type == 1) player.spdMult += 0.30f;
                else if (type == 2) player.cdMult *= 0.65f;
                else if (type == 3) player.hp = player.maxHp;
                else if (type == 4) { player.maxHp += 80.f; player.hp += 80.f; }
                else if (type == 5) { player.titanMode = true; player.titanTimer = 400.f; } // TITAN CURTO E GROSSO
                
                currentState = STATE_PLAYING;
            }
        } else if (currentState == STATE_PLAYING) {
            dayTime += 0.0003f;
            playTimeSeconds += 0.016f;
            
            achManager.check(currentScore, totalBossesDefeated, currentCombo, playTimeSeconds);
            
            if (comboTimer > 0) {
                comboTimer--;
            } else {
                currentCombo = 0;
            }
            
            if (fireCooldown > 0) {
                fireCooldown--;
            }

            meteorTimer--;
            if (meteorTimer <= 0) {
                for (int i = 0; i < 5; i++) {
                    meteors.push_back(Meteor(sf::Vector2f({player.pos.x + (std::rand() % 2000 - 1000), player.pos.y + (std::rand() % 2000 - 1000)})));
                }
                meteorTimer = 1800;
                fTexts.push_back(FloatingText(player.pos, "CHUVA DE METEOROS!", font, sf::Color::Red, 50));
            }

            for (size_t i = 0; i < meteors.size(); ) {
                if (meteors[i].update()) {
                    shakeTimer = 50.f;
                    for (int pt = 0; pt < 100; pt++) {
                        particles.push_back(Particle(meteors[i].pos, sf::Color(255, 100, 0), 3.f));
                    }
                    if (getDistance(player.pos, meteors[i].pos) < 250.f) {
                        player.takeDamage(100.f, shakeTimer, fTexts, font);
                    }
                    for (auto& dragon : dragons) {
                        if (getDistance(dragon.sprite.getPosition(), meteors[i].pos) < 300.f) {
                            dragon.hp -= 500.f;
                            fTexts.push_back(FloatingText(dragon.sprite.getPosition(), "-500", font, sf::Color::Yellow, 40));
                        }
                    }
                    meteors.erase(meteors.begin() + i);
                } else {
                    i++;
                }
            }
            
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fireCooldown <= 0 && player.dashTime <= 0 && !player.titanMode) {
                sf::Vector2f mouseWorldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), camera);
                sf::Vector2f direction = mouseWorldPos - player.pos;
                
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length != 0.f) direction /= length;
                
                float angleRad = std::atan2(direction.y, direction.x);
                float cosA = std::cos(angleRad);
                float sinA = std::sin(angleRad);
                
                sf::Vector2f leftGun(player.pos.x + 20.f * cosA - 15.f * sinA, player.pos.y + 20.f * sinA + 15.f * cosA);
                sf::Vector2f rightGun(player.pos.x + 20.f * cosA + 15.f * sinA, player.pos.y + 20.f * sinA - 15.f * cosA);

                if (player.activeWeaponDrop == 1) {
                    fireCooldown = 2 * player.cdMult;
                    player.dropAmmo--;
                    float spreadAngle = angleRad + ((std::rand() % 50 - 25) / 100.f);
                    sf::Vector2f spreadDir(std::cos(spreadAngle), std::sin(spreadAngle));
                    bullets.push_back(Bullet(leftGun, spreadDir, 22.f, 6.f * player.dmgMult, true, 8.f, sf::Color(255, 100, 0), 5, false));
                    if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 10.f, 2.0f);
                    achManager.unlock(4);
                } else if (player.activeWeaponDrop == 2) {
                    fireCooldown = 60 * player.cdMult;
                    player.dropAmmo--;
                    shakeTimer = 20.f;
                    bullets.push_back(Bullet(player.pos, direction, 18.f, 300.f * player.dmgMult, false, 14.f, sf::Color::Red, 6, false));
                    if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 80.f, 0.5f);
                } else if (player.activeWeaponDrop == 3) {
                    fireCooldown = 45 * player.cdMult;
                    player.dropAmmo--;
                    shakeTimer = 30.f;
                    sf::Vector2f endPoint(player.pos.x + direction.x * 2500.f, player.pos.y + direction.y * 2500.f);
                    lasers.push_back(LaserBeam(player.pos, endPoint));
                    if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 90.f, 3.0f);
                    
                    for (auto& dragon : dragons) {
                        if (distToSegment(dragon.sprite.getPosition(), player.pos, endPoint) < 80.f) {
                            dragon.hp -= 400.f * player.dmgMult;
                            fTexts.push_back(FloatingText(dragon.sprite.getPosition(), std::to_string((int)(400 * player.dmgMult)), font, sf::Color::White, 30));
                            for (int pt = 0; pt < 20; pt++) particles.push_back(Particle(dragon.sprite.getPosition(), sf::Color::Red));
                        }
                    }
                } else if (player.activeWeaponDrop == 4) {
                    fireCooldown = 20 * player.cdMult;
                    player.dropAmmo--;
                    mines.push_back(Mine(player.pos));
                } else if (player.activeWeaponDrop == 5) {
                    fireCooldown = 120 * player.cdMult;
                    player.dropAmmo--;
                    blackHoles.push_back(BlackHole(player.pos));
                } else {
                    float bSize = (player.charClass == 1) ? 8.f : 4.f;
                    float bDmg = (player.charClass == 1) ? 15.f : 8.f;
                    
                    if (player.level == 1) {
                        fireCooldown = 15 * player.cdMult;
                        bullets.push_back(Bullet(leftGun, direction, 32.f, bDmg * player.dmgMult, false, bSize, sf::Color::Yellow, 0, false));
                        bullets.push_back(Bullet(rightGun, direction, 32.f, bDmg * player.dmgMult, false, bSize, sf::Color::Yellow, 0, false));
                        if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 40.f, 1.2f);
                    } else if (player.level == 2) {
                        fireCooldown = 30 * player.cdMult;
                        shakeTimer = 6.f;
                        for (int i = -3; i <= 3; i++) {
                            float sAngle = angleRad + (i * 0.12f);
                            sf::Vector2f sDir(std::cos(sAngle), std::sin(sAngle));
                            bullets.push_back(Bullet(player.pos, sDir, 30.f, (bDmg - 2.f) * player.dmgMult, false, bSize, sf::Color(255, 150, 0), 0, false));
                        }
                        if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 60.f, 0.8f);
                    } else if (player.level == 3) {
                        fireCooldown = 4 * player.cdMult;
                        shakeTimer = 3.f;
                        bullets.push_back(Bullet(leftGun, direction, 38.f, (bDmg - 4.f) * player.dmgMult, false, bSize - 1.f, sf::Color::Yellow, 0, false));
                        bullets.push_back(Bullet(rightGun, direction, 38.f, (bDmg - 4.f) * player.dmgMult, false, bSize - 1.f, sf::Color::Yellow, 0, false));
                        if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 30.f, 1.5f);
                    } else {
                        fireCooldown = 35 * player.cdMult;
                        shakeTimer = 15.f;
                        bullets.push_back(Bullet(leftGun, direction, 55.f, (bDmg * 8.f) * player.dmgMult, true, bSize + 6.f, sf::Color::Cyan, 0, false));
                        bullets.push_back(Bullet(rightGun, direction, 55.f, (bDmg * 8.f) * player.dmgMult, true, bSize + 6.f, sf::Color::Cyan, 0, false));
                        if (audio.shootBuf.getDuration().asSeconds() > 0) audio.playSound(audio.shootBuf, 80.f, 0.6f);
                    }
                }
                
                if (player.dropAmmo <= 0) {
                    player.activeWeaponDrop = 0;
                }
                
                particles.push_back(Particle(leftGun, sf::Color::Yellow));
                particles.push_back(Particle(rightGun, sf::Color::Yellow));
                
                if (player.charClass == 1) {
                    player.sprite.setTexture(halkTex);
                } else {
                    player.sprite.setTexture(playerShootTex);
                }
                
                shootSpriteTimer = 8;
            }

            float currentSpeed = (player.freezeTimer > 0 ? 3.5f : 8.0f) * player.spdMult;
            
            if (player.titanMode) {
                currentSpeed = 12.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) player.pos.y -= currentSpeed;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) player.pos.y += currentSpeed;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) player.pos.x -= currentSpeed;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) player.pos.x += currentSpeed;
                
                shakeTimer = 2.f;
                for (auto& dragon : dragons) {
                    if (getDistance(player.pos, dragon.sprite.getPosition()) < 200.f) {
                        dragon.hp -= 10.f;
                        particles.push_back(Particle(dragon.sprite.getPosition(), sf::Color::Red));
                    }
                }
            } else {
                if (player.dashTime > 0) {
                    currentSpeed = 28.0f;
                    player.dashTime--;
                    player.pos += player.dashDir * currentSpeed;
                    particles.push_back(Particle(player.pos, sf::Color(150, 150, 150)));
                    dashTrails.push_back(DashTrail((player.charClass == 1) ? halkTex : playerIdleTex, player.pos, player.sprite.getRotation(), player.sprite.getScale()));
                } else {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) player.pos.y -= currentSpeed;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) player.pos.y += currentSpeed;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) player.pos.x -= currentSpeed;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) player.pos.x += currentSpeed;
                }
            }
            
            if (player.dashCooldown > 0) {
                player.dashCooldown--;
            }
            
            if (shootSpriteTimer > 0) {
                shootSpriteTimer--;
                if (shootSpriteTimer <= 0) {
                    if (player.charClass == 1) {
                        player.sprite.setTexture(halkTex);
                    } else {
                        player.sprite.setTexture(playerIdleTex);
                    }
                }
            }
            
            player.updateAnimationAndSprite();
            
            sf::Vector2f msPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), camera);
            float rotationAngle = std::atan2(msPos.y - player.pos.y, msPos.x - player.pos.x) * 180.f / PI;
            if (!player.titanMode) {
                player.sprite.setRotation(sf::degrees(rotationAngle + 90.f + player.rotOffset));
            }
            
            camera.setCenter(player.pos + getScreenShake(shakeTimer));

            if (player.hasPet) {
                sf::Vector2f targetPos;
                bool hasTarget = false;
                float minDistance = 99999.f;
                for (auto& dragon : dragons) {
                    float distance = getDistance(player.pos, dragon.sprite.getPosition());
                    if (distance < 800.f && distance < minDistance) {
                        minDistance = distance;
                        targetPos = dragon.sprite.getPosition();
                        hasTarget = true;
                    }
                }
                pet.update(player.pos, bullets, targetPos, hasTarget);
            }

            if (std::rand() % 2 == 0) {
                rainSystem.push_back(RainDrop(camera.getCenter()));
            }
            for (size_t i = 0; i < rainSystem.size(); ) {
                if (rainSystem[i].update(camera.getCenter())) {
                    rainSystem.erase(rainSystem.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < dashTrails.size(); ) {
                if (dashTrails[i].update()) {
                    dashTrails.erase(dashTrails.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < flares.size(); ) {
                if (flares[i].update()) {
                    shakeTimer = 80.f;
                    for (int b = 0; b < 15; b++) {
                        sf::Vector2f expPos(flares[i].pos.x + (std::rand() % 1000 - 500), flares[i].pos.y + (std::rand() % 1000 - 500));
                        for (int pt = 0; pt < 80; pt++) particles.push_back(Particle(expPos, sf::Color(255, 100, 0), 4.f));
                        
                        for (auto& dragon : dragons) {
                            if (getDistance(expPos, dragon.sprite.getPosition()) < 300.f) {
                                dragon.hp -= 350.f;
                                fTexts.push_back(FloatingText(dragon.sprite.getPosition(), "-350", font, sf::Color::Red, 45));
                            }
                        }
                    }
                    if (audio.shootBuf.getDuration().asSeconds() > 0) {
                        audio.playSound(audio.shootBuf, 100.f, 0.2f);
                        audio.playSound(audio.shootBuf, 100.f, 0.3f);
                    }
                    flares.erase(flares.begin() + i);
                } else {
                    i++;
                }
            }

            for (size_t i = 0; i < mines.size(); ) {
                mines[i].update();
                bool exploded = false;
                for (auto& dragon : dragons) {
                    if (getDistance(mines[i].pos, dragon.sprite.getPosition()) < 60.f) {
                        exploded = true;
                        shakeTimer = 35.f;
                        for (int pt = 0; pt < 80; pt++) particles.push_back(Particle(mines[i].pos, sf::Color(255, 100, 0), 3.f));
                        
                        for (auto& d2 : dragons) {
                            if (getDistance(mines[i].pos, d2.sprite.getPosition()) < 250.f) {
                                d2.hp -= 350.f * player.dmgMult;
                                fTexts.push_back(FloatingText(d2.sprite.getPosition(), std::to_string((int)(350 * player.dmgMult)), font, sf::Color::Yellow, 30));
                            }
                        }
                        if (audio.shootBuf.getDuration().asSeconds() > 0) {
                            audio.playSound(audio.shootBuf, 100.f, 0.4f);
                        }
                        break;
                    }
                }
                
                if (exploded) {
                    mines.erase(mines.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < blackHoles.size(); ) {
                if (blackHoles[i].update()) {
                    shakeTimer = 60.f;
                    for (int pt = 0; pt < 150; pt++) {
                        particles.push_back(Particle(blackHoles[i].pos, sf::Color(150, 0, 255), 5.f));
                    }
                    for (auto& dragon : dragons) {
                        if (getDistance(blackHoles[i].pos, dragon.sprite.getPosition()) < 400.f) {
                            dragon.hp -= 1000.f * player.dmgMult;
                            fTexts.push_back(FloatingText(dragon.sprite.getPosition(), "-1000", font, sf::Color::Magenta, 40));
                        }
                    }
                    blackHoles.erase(blackHoles.begin() + i);
                } else {
                    for (auto& dragon : dragons) {
                        if (getDistance(blackHoles[i].pos, dragon.sprite.getPosition()) < 300.f) {
                            sf::Vector2f pullDir = blackHoles[i].pos - dragon.sprite.getPosition();
                            float pullLen = std::sqrt(pullDir.x * pullDir.x + pullDir.y * pullDir.y);
                            if (pullLen > 0.f) {
                                dragon.sprite.move((pullDir / pullLen) * 5.f);
                            }
                        }
                    }
                    i++;
                }
            }

            for (size_t i = 0; i < lasers.size(); ) {
                if (lasers[i].update()) {
                    lasers.erase(lasers.begin() + i);
                } else {
                    i++;
                }
            }

            for (size_t i = 0; i < allies.size(); ) {
                sf::Vector2f targetPos;
                bool hasTarget = false;
                float minDistance = 99999.f;
                for (auto& dragon : dragons) {
                    float distance = getDistance(allies[i].sprite.getPosition(), dragon.sprite.getPosition());
                    if (distance < 800.f && distance < minDistance) {
                        minDistance = distance;
                        targetPos = dragon.sprite.getPosition();
                        hasTarget = true;
                    }
                }
                
                if (allies[i].update(bullets, targetPos, hasTarget, audio)) {
                    allies.erase(allies.begin() + i);
                } else {
                    i++;
                }
            }

            if (enemiesSpawned < enemiesToSpawn) {
                if (std::rand() % std::max(5, 35 - (currentWave * 3)) == 0) {
                    float rx = player.pos.x + (std::rand() % 4000 - 2000);
                    float ry = player.pos.y + (std::rand() % 4000 - 2000);
                    
                    if (getDistance(sf::Vector2f({rx, ry}), player.pos) > 1000.f) {
                        int type = E_NORMAL;
                        int randomVal = std::rand() % 100;
                        
                        if (enemiesSpawned == enemiesToSpawn - 1 && currentWave % 5 == 0) {
                            type = E_COLOSSAL;
                        } else if (randomVal > 95) {
                            type = E_NECROMANCER;
                        } else if (randomVal > 88) {
                            type = E_WORM;
                        } else if (randomVal > 80) {
                            type = E_SUMMONER;
                        } else if (randomVal > 72) {
                            type = E_ARMORED;
                        } else if (randomVal > 64) {
                            type = E_ICE;
                        } else if (randomVal > 56) {
                            type = E_TOXIC;
                        } else if (randomVal > 48) {
                            type = E_GHOST;
                        } else if (randomVal > 30) {
                            type = E_FAST;
                        }
                        
                        dragons.push_back(Dragon(sf::Vector2f({rx, ry}), dragonTex, type, enemyHealthMultiplier));
                        enemiesSpawned++;
                    }
                }
            } else if (dragons.empty()) {
                if (waveBreakTimer == 0) {
                    waveBreakTimer = 180;
                    if (currentWave == 10) {
                        achManager.unlock(2);
                    }
                }
                waveBreakTimer--;
                
                if (waveBreakTimer <= 0) {
                    currentWave++;
                    enemiesToSpawn += 6;
                    enemiesSpawned = 0;
                    enemyHealthMultiplier *= 1.35f;
                    fTexts.push_back(FloatingText(player.pos, "WAVE " + std::to_string(currentWave), font, sf::Color::Yellow, 50));
                }
            }

            for (size_t i = 0; i < fTexts.size(); ) {
                if (fTexts[i].update()) {
                    fTexts.erase(fTexts.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < particles.size(); ) {
                if (particles[i].update()) {
                    particles.erase(particles.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < bullets.size(); ) {
                bullets[i].shape.move(bullets[i].velocity);
                bullets[i].updateAnim();
                bool deleted = false;
                
                if (getDistance(bullets[i].shape.getPosition(), bullets[i].startPos) > bullets[i].maxDist) {
                    deleted = true;
                }
                
                if (bullets[i].isEnemyBullet && getDistance(bullets[i].shape.getPosition(), player.pos) < 25.f) {
                    player.takeDamage(bullets[i].damage, shakeTimer, fTexts, font);
                    deleted = true;
                    
                    if (bullets[i].type == 7) {
                        player.freezeTimer = 180;
                    }
                    
                    for (int pt = 0; pt < 5; pt++) {
                        particles.push_back(Particle(player.pos, sf::Color::Red));
                    }
                }
                
                if (deleted) {
                    bullets.erase(bullets.begin() + i);
                } else {
                    i++;
                }
            }

            for (size_t i = 0; i < pickups.size(); ) {
                if (getDistance(pickups[i].shape.getPosition(), player.pos) < 55.f) {
                    if (pickups[i].type == 0) {
                        player.hp = std::min(player.maxHp, player.hp + 40.f);
                        fTexts.push_back(FloatingText(player.pos, "+40 HP", font, sf::Color::Green));
                    } else if (pickups[i].type == 1) {
                        player.ultCharge = std::min(100.f, player.ultCharge + 30.f);
                        fTexts.push_back(FloatingText(player.pos, "ULT CHARGE", font, sf::Color::Cyan));
                    } else if (pickups[i].type == 2) {
                        player.exp += 20 * (1 + (currentCombo / 5));
                        fTexts.push_back(FloatingText(player.pos, "+EXP", font, sf::Color::Yellow));
                    } else if (pickups[i].type == 3) {
                        allies.push_back(AllyTurret(player.pos, playerIdleTex));
                        fTexts.push_back(FloatingText(player.pos, "SUPORTE!", font, sf::Color::Magenta));
                    } else if (pickups[i].type == 5) {
                        player.hasPet = true;
                        fTexts.push_back(FloatingText(player.pos, "PET EQUIPADO!", font, sf::Color(255, 215, 0)));
                    } else {
                        player.activeWeaponDrop = (std::rand() % 5) + 1;
                        if (player.activeWeaponDrop == 1) {
                            player.dropAmmo = 300;
                        } else if (player.activeWeaponDrop == 4) {
                            player.dropAmmo = 15;
                        } else if (player.activeWeaponDrop == 5) {
                            player.dropAmmo = 3;
                        } else {
                            player.dropAmmo = 15;
                        }
                        fTexts.push_back(FloatingText(player.pos, "ARMA NOVA!", font, sf::Color(255, 100, 0)));
                    }
                    pickups.erase(pickups.begin() + i);
                } else {
                    if (pickups[i].update()) {
                        pickups.erase(pickups.begin() + i);
                    } else {
                        i++;
                    }
                }
            }

            if (player.exp >= player.expNeeded) {
                player.exp -= player.expNeeded;
                player.level++;
                player.expNeeded *= 1.8f;
                
                currentState = STATE_LEVEL_UP;
                levelUpScene.generateCards();
            }

            for (size_t i = 0; i < dragons.size(); ) {
                dragons[i].handlePhases();
                dragons[i].updateAnimation();

                sf::Vector2f dir = player.pos - dragons[i].sprite.getPosition();
                float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (length != 0.f) {
                    dir /= length;
                }
                
                float currentSpeed = dragons[i].speed;
                if (dragons[i].freezeTimer > 0) {
                    dragons[i].freezeTimer--;
                    currentSpeed *= 0.5f;
                    dragons[i].sprite.setColor(sf::Color::Cyan);
                } else if (dragons[i].type != E_GHOST && dragons[i].type != E_TOXIC && dragons[i].type != E_COLOSSAL && dragons[i].type != E_SUMMONER && dragons[i].type != E_NECROMANCER && dragons[i].type != E_WORM) {
                    dragons[i].sprite.setColor(sf::Color::White);
                }
                
                if (dragons[i].type == E_WORM) {
                    if (std::rand() % 100 == 0) {
                        dragons[i].sprite.setPosition(sf::Vector2f({player.pos.x + (std::rand() % 400 - 200), player.pos.y + (std::rand() % 400 - 200)}));
                        for (int pt = 0; pt < 30; pt++) {
                            particles.push_back(Particle(dragons[i].sprite.getPosition(), sf::Color(100, 50, 20), 3.f));
                        }
                    }
                }
                
                dragons[i].sprite.move(dir * currentSpeed);
                float rotationAngle = std::atan2(dir.y, dir.x) * 180.f / PI;
                dragons[i].sprite.setRotation(sf::degrees(rotationAngle - 90.f));

                if (dragons[i].type == E_COLOSSAL || dragons[i].type == E_TOXIC || dragons[i].type == E_ICE || dragons[i].type == E_SUMMONER || dragons[i].type == E_NECROMANCER) {
                    dragons[i].fireCooldown--;
                    if (dragons[i].fireCooldown <= 0) {
                        if (dragons[i].type == E_COLOSSAL) {
                            bullets.push_back(Bullet(dragons[i].sprite.getPosition(), dir, 15.f, 25.f, false, 15.f, sf::Color(255, 100, 0), 8, true));
                            dragons[i].fireCooldown = (dragons[i].bossPhase == 3) ? 60 : 120;
                        } else if (dragons[i].type == E_TOXIC) {
                            bullets.push_back(Bullet(dragons[i].sprite.getPosition(), dir, 10.f, 15.f, false, 8.f, sf::Color::Green, 9, true));
                            dragons[i].fireCooldown = 90;
                        } else if (dragons[i].type == E_ICE) {
                            bullets.push_back(Bullet(dragons[i].sprite.getPosition(), dir, 12.f, 10.f, false, 10.f, sf::Color::Cyan, 7, true));
                            dragons[i].fireCooldown = 100;
                        } else if (dragons[i].type == E_SUMMONER) {
                            float spawnDist = 150.f;
                            sf::Vector2f spawnPos(dragons[i].sprite.getPosition().x + (std::rand() % 200 - 100), dragons[i].sprite.getPosition().y + (std::rand() % 200 - 100));
                            dragons.push_back(Dragon(spawnPos, dragonTex, E_FAST, enemyHealthMultiplier * 0.5f));
                            for (int pt = 0; pt < 20; pt++) {
                                particles.push_back(Particle(spawnPos, sf::Color::Magenta, 2.f));
                            }
                            dragons[i].fireCooldown = 180;
                        } else if (dragons[i].type == E_NECROMANCER) {
                            if (!bloodPuddles.empty()) {
                                sf::Vector2f revivePos = bloodPuddles.back().shape.getPosition();
                                dragons.push_back(Dragon(revivePos, dragonTex, E_GHOST, enemyHealthMultiplier));
                                bloodPuddles.pop_back();
                                for (int pt = 0; pt < 40; pt++) {
                                    particles.push_back(Particle(revivePos, sf::Color(50, 0, 100), 4.f));
                                }
                            }
                            dragons[i].fireCooldown = 150;
                        }
                    }
                }
                
                float hitDistance = (dragons[i].type == E_COLOSSAL) ? 120.f : ((dragons[i].type == E_ARMORED) ? 60.f : 40.f);
                if (length < hitDistance) {
                    float damageVal = (dragons[i].type == E_COLOSSAL) ? 30.f : ((dragons[i].type == E_ARMORED) ? 15.f : 10.f);
                    player.takeDamage(damageVal, shakeTimer, fTexts, font);
                }
                
                if (dragons[i].hp <= 0) {
                    int goreAmount = (dragons[i].type == E_COLOSSAL) ? 100 : 20;
                    for (int pt = 0; pt < goreAmount; pt++) {
                        particles.push_back(Particle(dragons[i].sprite.getPosition(), sf::Color::Red));
                    }
                    
                    bloodPuddles.push_back(BloodDecal(dragons[i].sprite.getPosition()));
                    if (bloodPuddles.size() > 200) {
                        bloodPuddles.pop_front();
                    }
                    
                    int randomLoot = std::rand() % 100;
                    if (dragons[i].type == E_COLOSSAL) {
                        pickups.push_back(Pickup(dragons[i].sprite.getPosition(), 3));
                        pickups.push_back(Pickup(sf::Vector2f({dragons[i].sprite.getPosition().x + 30.f, dragons[i].sprite.getPosition().y}), 5));
                        pickups.push_back(Pickup(sf::Vector2f({dragons[i].sprite.getPosition().x - 30.f, dragons[i].sprite.getPosition().y}), 4));
                        shakeTimer = 30.f;
                        totalBossesDefeated++;
                        if (dragons[i].bossPhase == 3) {
                            achManager.unlock(3);
                        }
                    } else {
                        if (randomLoot > 95) {
                            pickups.push_back(Pickup(dragons[i].sprite.getPosition(), 4));
                        } else if (randomLoot > 88) {
                            pickups.push_back(Pickup(dragons[i].sprite.getPosition(), 0));
                        } else if (randomLoot > 80) {
                            pickups.push_back(Pickup(dragons[i].sprite.getPosition(), 1));
                        } else {
                            pickups.push_back(Pickup(dragons[i].sprite.getPosition(), 2));
                        }
                    }
                    
                    currentScore += (dragons[i].type == E_COLOSSAL) ? 50 : 1;
                    currentCombo++;
                    comboTimer = 240;
                    dragons.erase(dragons.begin() + i);
                } else {
                    i++;
                }
            }

            for (size_t i = 0; i < bullets.size(); ) {
                bool bulletDestroyed = false;
                if (!bullets[i].isEnemyBullet) {
                    for (size_t j = 0; j < dragons.size(); ) {
                        float hitBox = (dragons[j].type == E_COLOSSAL) ? 120.f : 40.f;
                        if (getDistance(bullets[i].shape.getPosition(), dragons[j].sprite.getPosition()) < hitBox) {
                            dragons[j].hp -= bullets[i].damage;
                            fTexts.push_back(FloatingText(dragons[j].sprite.getPosition(), std::to_string((int)bullets[i].damage), font, sf::Color::White));
                            
                            if (bullets[i].type == 7) {
                                dragons[j].freezeTimer = 120;
                            }
                            
                            if (!bullets[i].piercing) {
                                bulletDestroyed = true;
                            }
                            
                            for (int pt = 0; pt < 4; pt++) {
                                particles.push_back(Particle(dragons[j].sprite.getPosition(), sf::Color::Red));
                            }
                            
                            if (bulletDestroyed) break;
                        }
                        j++;
                    }
                }
                
                if (bulletDestroyed) {
                    bullets.erase(bullets.begin() + i);
                } else {
                    i++;
                }
            }
            
            if (player.hp <= 0) {
                currentState = STATE_GAME_OVER;
                if (currentScore > highScore) {
                    highScore = currentScore;
                }
            }
        }

        if (currentState == STATE_GAME_OVER) {
            window.clear(sf::Color(50, 0, 0));
        } else {
            window.clear(sf::Color(10, 10, 10));
        }
        
        if (currentState == STATE_PLAYING || currentState == STATE_GAME_OVER || currentState == STATE_LEVEL_UP) {
            window.setView(camera);
            
            window.draw(gameBg);

            for (auto& decal : bloodPuddles) window.draw(decal.shape);
            for (auto& mine : mines) { window.draw(mine.shape); window.draw(mine.pulse); }
            for (auto& flare : flares) { window.draw(flare.beam); window.draw(flare.glow); }
            for (auto& bh : blackHoles) { window.draw(bh.aura); window.draw(bh.core); }
            for (auto& pickup : pickups) window.draw(pickup.shape);
            if (player.hasPet) window.draw(pet.sprite);
            for (auto& ally : allies) window.draw(ally.sprite);
            for (auto& dragon : dragons) window.draw(dragon.sprite);
            for (auto& bullet : bullets) window.draw(bullet.shape);
            for (auto& laser : lasers) window.draw(laser.shape);
            for (auto& particle : particles) window.draw(particle.shape);
            if (player.hp > 0) window.draw(player.sprite);
            for (auto& trail : dashTrails) window.draw(trail.sprite);
            for (auto& meteor : meteors) window.draw(meteor.shadow);
            for (auto& drop : rainSystem) window.draw(drop.shape);
            
            for (auto& floatText : fTexts) window.draw(floatText.txt);
        }

        window.setView(uiCamera);
        
        if (currentState == STATE_MENU) {
            menuInterativo.draw(window);
        } else if (currentState == STATE_CHAR_SELECT) {
            charSelector.draw(window);
        } else if (currentState == STATE_CONTROLS) {
            window.clear(sf::Color(20, 20, 30));
            window.draw(controlsTitleText);
        } else if (currentState == STATE_LEVEL_UP) {
            window.clear(sf::Color::Black);
            levelUpScene.draw(window, player.sprite, player.baseScale);
        } else if (currentState == STATE_PLAYING) {
            sf::RectangleShape barBg(sf::Vector2f({300.f, 15.f}));
            barBg.setFillColor(sf::Color(50, 50, 50, 200));
            barBg.setOutlineThickness(2.f);
            barBg.setOutlineColor(sf::Color::Black);
            
            barBg.setPosition(sf::Vector2f({20.f, 20.f}));
            window.draw(barBg);
            
            sf::RectangleShape hpBar(sf::Vector2f({300.f * (std::max(0.f, player.hp) / player.maxHp), 15.f}));
            hpBar.setFillColor(sf::Color::Red);
            hpBar.setPosition(sf::Vector2f({20.f, 20.f}));
            window.draw(hpBar);
            
            barBg.setPosition(sf::Vector2f({20.f, 40.f}));
            window.draw(barBg);
            
            sf::RectangleShape expBar(sf::Vector2f({300.f * (player.exp / player.expNeeded), 15.f}));
            expBar.setFillColor(sf::Color::Yellow);
            expBar.setPosition(sf::Vector2f({20.f, 40.f}));
            window.draw(expBar);
            
            barBg.setSize(sf::Vector2f({200.f, 10.f}));
            barBg.setPosition(sf::Vector2f({20.f, 60.f}));
            window.draw(barBg);
            
            sf::RectangleShape ultBar(sf::Vector2f({200.f * (player.ultCharge / 100.f), 10.f}));
            ultBar.setFillColor(sf::Color::Cyan);
            ultBar.setPosition(sf::Vector2f({20.f, 60.f}));
            window.draw(ultBar);
            
            barBg.setPosition(sf::Vector2f({20.f, 75.f}));
            window.draw(barBg);
            
            sf::RectangleShape dashBar(sf::Vector2f({200.f * (1.f - (float)player.dashCooldown / (60 * player.cdMult)), 10.f}));
            dashBar.setFillColor(sf::Color::White);
            dashBar.setPosition(sf::Vector2f({20.f, 75.f}));
            window.draw(dashBar);
            
            uiText.setFillColor(sf::Color::White);
            uiText.setString("Lv: " + std::to_string(player.level) + " | Score: " + std::to_string(currentScore) + " | Flares: " + std::to_string(player.flares));
            uiText.setPosition(sf::Vector2f({20.f, 95.f}));
            window.draw(uiText);
            
            if (player.activeWeaponDrop == 1) {
                uiText.setString("Lanca-Chamas (" + std::to_string(player.dropAmmo) + ")");
            } else if (player.activeWeaponDrop == 2) {
                uiText.setString("Bazuca (" + std::to_string(player.dropAmmo) + ")");
            } else if (player.activeWeaponDrop == 3) {
                uiText.setString("Laser Sniper (" + std::to_string(player.dropAmmo) + ")");
            } else if (player.activeWeaponDrop == 4) {
                uiText.setString("Minas (" + std::to_string(player.dropAmmo) + ")");
            } else if (player.activeWeaponDrop == 5) {
                uiText.setString("Buraco Negro (" + std::to_string(player.dropAmmo) + ")");
            } else {
                uiText.setString("Arma Normal (Lv " + std::to_string(player.level) + ")");
            }
            
            if (player.activeWeaponDrop > 0) {
                uiText.setFillColor(sf::Color(255, 100, 0));
            } else {
                uiText.setFillColor(sf::Color::Yellow);
            }
            uiText.setPosition(sf::Vector2f({20.f, 120.f}));
            window.draw(uiText);
            
            if (player.freezeTimer > 0) {
                uiText.setString("CONGELADO!");
                uiText.setFillColor(sf::Color::Cyan);
                uiText.setPosition(sf::Vector2f({20.f, 145.f}));
                window.draw(uiText);
            }
            
            if (currentCombo > 1) {
                uiText.setString(std::to_string(currentCombo) + "x COMBO!");
                uiText.setCharacterSize(30 + (currentCombo > 10 ? 10 : 0));
                uiText.setFillColor(sf::Color(255, 150, 0));
                uiText.setPosition(sf::Vector2f({20.f, 175.f}));
                window.draw(uiText);
                uiText.setCharacterSize(22);
            }
            
            if (player.ultCharge >= 100) {
                uiText.setString("[Q] ESTRONDO PRONTO!");
                uiText.setFillColor(sf::Color::Cyan);
                uiText.setCharacterSize(35);
                uiText.setPosition(sf::Vector2f({450.f, 600.f}));
                window.draw(uiText);
                uiText.setCharacterSize(22);
            }
            
            bool bossAlive = false;
            float bossHpPct = 0.f;
            for (auto& d : dragons) {
                if (d.type == E_COLOSSAL) {
                    bossAlive = true;
                    bossHpPct = std::max(0.f, d.hp / d.maxHp);
                    break;
                }
            }
            
            if (bossAlive) {
                sf::RectangleShape bossBgA(sf::Vector2f({800.f, 25.f}));
                bossBgA.setFillColor(sf::Color(50, 0, 0, 200));
                bossBgA.setOutlineThickness(3.f);
                bossBgA.setOutlineColor(sf::Color::Black);
                bossBgA.setPosition(sf::Vector2f({240.f, 20.f}));
                window.draw(bossBgA);
                
                sf::RectangleShape bossHpA(sf::Vector2f({800.f * bossHpPct, 25.f}));
                bossHpA.setFillColor(sf::Color::Red);
                bossHpA.setPosition(sf::Vector2f({240.f, 20.f}));
                window.draw(bossHpA);
                
                uiText.setString("DRAGÃO COLOSSAL");
                uiText.setFillColor(sf::Color::White);
                uiText.setPosition(sf::Vector2f({550.f, 22.f}));
                window.draw(uiText);
            }

            if (waveBreakTimer > 0) {
                waveAlertText.setString("WAVE " + std::to_string(currentWave) + " COMPLETA!");
                window.draw(waveAlertText);
            } else if (enemiesSpawned == enemiesToSpawn - 1 && currentWave % 5 == 0) {
                waveAlertText.setString("! ALERTA DE DRAGÃO COLOSSAL !");
                waveAlertText.setFillColor(sf::Color::Magenta);
                window.draw(waveAlertText);
            }
            
            sf::RectangleShape radarBg(sf::Vector2f({180.f, 180.f}));
            radarBg.setFillColor(sf::Color(0, 0, 0, 200));
            radarBg.setOutlineThickness(3.f);
            radarBg.setOutlineColor(sf::Color(100, 100, 100));
            radarBg.setPosition(sf::Vector2f({1080.f, 20.f}));
            window.draw(radarBg);
            
            sf::CircleShape rD(3.f);
            rD.setOrigin(sf::Vector2f({3.f, 3.f}));
            rD.setFillColor(sf::Color::Green);
            rD.setPosition(sf::Vector2f({1170.f, 110.f}));
            window.draw(rD);
            
            for (auto& d : dragons) {
                float rX = 1170.f + ((d.sprite.getPosition().x - player.pos.x) * 0.04f);
                float rY = 110.f + ((d.sprite.getPosition().y - player.pos.y) * 0.04f);
                
                if (rX > 1080.f && rX < 1260.f && rY > 20.f && rY < 200.f) {
                    if (d.type == E_COLOSSAL) rD.setFillColor(sf::Color::Magenta);
                    else rD.setFillColor(sf::Color::Red);
                    rD.setPosition(sf::Vector2f({rX, rY}));
                    window.draw(rD);
                }
            }
            
            achManager.render(window, font);
            
        } else if (currentState == STATE_GAME_OVER) {
            window.clear(sf::Color(30, 0, 0));
            window.draw(gameOverText);
            uiText.setCharacterSize(40);
            uiText.setFillColor(sf::Color::White);
            uiText.setString("SCORE FINAL: " + std::to_string(currentScore) + "\nMAIOR SCORE: " + std::to_string(highScore));
            uiText.setPosition(sf::Vector2f({450.f, 400.f}));
            window.draw(uiText);
        }
        
        window.display();
    }
    
    return 0;
}