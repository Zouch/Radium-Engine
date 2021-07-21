#pragma once

#include <Core/Containers/VectorArray.hpp>
#include <Core/Geometry/Spline.hpp>
#include <Core/RaCore.hpp>
#include <vector>

namespace Ra {
namespace Core {
namespace Geometry {
class Curve2D
{
  public:
    enum CurveType { LINE, CUBICBEZIER, SPLINE, SIZE };

    using Vector = Eigen::Matrix<Scalar, 2, 1>;

    virtual void addPoint( const Vector p ) = 0;
    virtual ~Curve2D()                      = default;

    virtual Vector f( Scalar u ) const                 = 0;
    virtual Vector df( Scalar u ) const                = 0;
    virtual Vector fdf( Scalar t, Vector& grad ) const = 0;

  protected:
    int size;
};

class QuadraSpline : public Curve2D
{
  public:
    QuadraSpline() { this->size = 0; }
    QuadraSpline( const Curve2D::Vector& p0,
                  const Curve2D::Vector& p1,
                  const Curve2D::Vector& p2 ) :
        m_points { p0, p1, p2 } {
        this->size = 3;
    }
    ~QuadraSpline() override = default;

    inline void addPoint( const Vector p ) override;

    inline Vector f( Scalar u ) const override;
    inline Vector df( Scalar u ) const override;
    inline Vector fdf( Scalar u, Vector& grad ) const override;

  private:
    Core::VectorArray<Vector> m_points;
};

class CubicBezier : public Curve2D
{
  public:
    CubicBezier() { this->size = 0; }
    CubicBezier( const Curve2D::Vector& p0,
                 const Curve2D::Vector& p1,
                 const Curve2D::Vector& p2,
                 const Curve2D::Vector& p3 ) :
        m_points { p0, p1, p2, p3 } {
        this->size = 4;
    }
    ~CubicBezier() override = default;

    inline void addPoint( const Vector p ) override;

    inline Vector f( Scalar u ) const override;
    inline Vector df( Scalar u ) const override;
    inline Vector fdf( Scalar t, Vector& grad ) const override;

  private:
    Vector m_points[4];
};

class Line : public Curve2D
{
  public:
    Line() { this->size = 0; }
    Line( const Vector& p0, const Vector& p1 ) : m_points { p0, p1 } { this->size = 2; }
    ~Line() override = default;

    inline void addPoint( const Vector p ) override;

    inline Vector f( Scalar u ) const override;
    inline Vector df( Scalar u ) const override;
    inline Vector fdf( Scalar t, Vector& grad ) const override;

  private:
    Vector m_points[2];
};

class SplineCurve : public Curve2D
{
  public:
    SplineCurve() { this->size = 0; }
    explicit SplineCurve( Core::VectorArray<Vector> points ) : m_points( points ) {
        this->size = points.size();
    }
    ~SplineCurve() override = default;

    inline void addPoint( const Vector p ) override;

    inline Vector f( Scalar u ) const override;
    inline Vector df( Scalar u ) const override;
    inline Vector fdf( Scalar t, Vector& grad ) const override;

  private:
    Core::VectorArray<Vector> m_points;
};

} // namespace Geometry
} // namespace Core
} // namespace Ra

#include <Core/Geometry/Curve2D.inl>
