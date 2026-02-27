#include "GameEnums.hpp"
#include <cstdlib>

float getDistance(sf::Vector2f p1, sf::Vector2f p2) {
    return std::sqrt((p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y));
}

float distToSegment(sf::Vector2f p, sf::Vector2f v, sf::Vector2f w) {
    float l2 = (v.x-w.x)*(v.x-w.x) + (v.y-w.y)*(v.y-w.y);
    if (l2 == 0.0f) return getDistance(p, v);
    float t = std::max(0.0f, std::min(1.0f, ((p.x-v.x)*(w.x-v.x) + (p.y-v.y)*(w.y-v.y)) / l2));
    return getDistance(p, sf::Vector2f({v.x + t*(w.x-v.x), v.y + t*(w.y-v.y)}));
}

sf::Vector2f getScreenShake(float& sTmr) {
    if (sTmr > 0) {
        sTmr--;
        return sf::Vector2f({(std::rand()%10-5)*(sTmr/10.f), (std::rand()%10-5)*(sTmr/10.f)});
    }
    return sf::Vector2f({0.f, 0.f});
}