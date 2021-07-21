#pragma once

#include "SpotLight.hpp"

namespace Ra {
namespace Engine {
namespace Scene {
inline void SpotLight::setPosition( const Eigen::Matrix<Scalar, 3, 1>& position ) {
    m_position = position;
}

inline const Eigen::Matrix<Scalar, 3, 1>& Scene::SpotLight::getPosition() const {
    return m_position;
}

inline void SpotLight::setDirection( const Eigen::Matrix<Scalar, 3, 1>& direction ) {
    m_direction = direction.normalized();
}

inline const Eigen::Matrix<Scalar, 3, 1>& SpotLight::getDirection() const {
    return m_direction;
}

inline void SpotLight::setInnerAngleInRadians( Scalar angle ) {
    m_innerAngle = angle;
}

inline void SpotLight::setOuterAngleInRadians( Scalar angle ) {
    m_outerAngle = angle;
}

inline void SpotLight::setInnerAngleInDegrees( Scalar angle ) {
    m_innerAngle = angle * Core::Math::toRad;
}

inline void SpotLight::setOuterAngleInDegrees( Scalar angle ) {
    m_outerAngle = Core::Math::toRad * angle;
}

inline Scalar SpotLight::getInnerAngle() const {
    return m_innerAngle;
}

inline Scalar SpotLight::getOuterAngle() const {
    return m_outerAngle;
}

inline void SpotLight::setAttenuation( const Attenuation& attenuation ) {
    m_attenuation = attenuation;
}

inline void SpotLight::setAttenuation( Scalar constant, Scalar linear, Scalar quadratic ) {
    m_attenuation = { constant, linear, quadratic };
}

inline const SpotLight::Attenuation& SpotLight::getAttenuation() const {
    return m_attenuation;
}

} // namespace Scene
} // namespace Engine
} // namespace Ra
