#ifndef GAME_ENUMS_HPP
#define GAME_ENUMS_HPP

#include <SFML/Graphics.hpp>
#include <cmath>

const float PI = 3.14159265f;

enum GameState { STATE_MENU, STATE_CHAR_SELECT, STATE_PLAYING, STATE_GAME_OVER, STATE_CONTROLS, STATE_LEVEL_UP };
enum EnemyType { E_NORMAL, E_FAST, E_ARMORED, E_COLOSSAL, E_TOXIC, E_GHOST, E_ICE, E_SUMMONER, E_NECROMANCER, E_WORM };

float getDistance(sf::Vector2f p1, sf::Vector2f p2);
float distToSegment(sf::Vector2f p, sf::Vector2f v, sf::Vector2f w);
sf::Vector2f getScreenShake(float& sTmr);

#endif