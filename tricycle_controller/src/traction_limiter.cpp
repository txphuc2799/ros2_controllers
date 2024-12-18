#include <algorithm>
#include <stdexcept>
#include <string>
#include <ros/ros.h>

#include <tricycle_controller/traction_limiter.h>

template <typename T>
T
clamp(T x, T min, T max)
{
    return std::min(std::max(min, x), max);
}

namespace tricycle_controller
{
TractionLimiter::TractionLimiter(double min_velocity,
                                 double max_velocity,
                                 double min_acceleration,
                                 double max_acceleration,
                                 double min_deceleration,
                                 double max_deceleration,
                                 double min_jerk,
                                 double max_jerk,
                                 double wheel_radius):
    min_velocity_(min_velocity),
    max_velocity_(max_velocity),
    min_acceleration_(min_acceleration),
    max_acceleration_(max_acceleration),
    min_deceleration_(min_deceleration),
    max_deceleration_(max_deceleration),
    min_jerk_(min_jerk),
    max_jerk_(max_jerk),
    wheel_radius_(wheel_radius)
{
    if (!std::isnan(min_velocity_) && std::isnan(max_velocity_)) max_velocity_ = 1000.0;  // Arbitrarily big number
    if (!std::isnan(max_velocity_) && std::isnan(min_velocity_)) min_velocity_ = 0.0;

    if (!std::isnan(min_acceleration_) && std::isnan(max_acceleration_)) max_acceleration_ = 1000.0;
    if (!std::isnan(max_acceleration_) && std::isnan(min_acceleration_)) min_acceleration_ = 0.0;

    if (!std::isnan(min_deceleration_) && std::isnan(max_deceleration_)) max_deceleration_ = 1000.0;
    if (!std::isnan(max_deceleration_) && std::isnan(min_acceleration_)) min_deceleration_ = 0.0;

    if (!std::isnan(min_jerk_) && std::isnan(max_jerk_)) max_jerk_ = 1000.0;
    if (!std::isnan(max_jerk_) && std::isnan(min_jerk_)) min_jerk_ = 0.0;

    const std::string error =
        "The positive limit will be applied to both directions. Setting different limits for positive "
        "and negative directions is not supported. Actuators are "
        "assumed to have the same constraints in both directions";
    if (min_velocity_ < 0 || max_velocity_ < 0)
    {
        throw std::invalid_argument("Velocity cannot be negative." + error);
    }

    if (min_acceleration_ < 0 || max_acceleration_ < 0)
    {
        throw std::invalid_argument("Acceleration cannot be negative." + error);
    }

    if (min_deceleration_ < 0 || max_deceleration_ < 0)
    {
        throw std::invalid_argument("Deceleration cannot be negative." + error);
    }

    if (min_jerk_ < 0 || max_jerk_ < 0)
    {
        throw std::invalid_argument("Jerk cannot be negative." + error);
    }
}

double TractionLimiter::limit(double & v, double v0, double v1, double dt)
{
    const double tmp = v;

    if (!std::isnan(min_jerk_) && !std::isnan(max_jerk_)) limit_jerk(v, v0, v1, dt);
    if (!std::isnan(min_acceleration_) && !std::isnan(max_acceleration_))
        limit_acceleration(v, v0, dt);
    if (!std::isnan(min_velocity_) && !std::isnan(max_velocity_)) limit_velocity(v);

    return tmp != 0.0 ? v / tmp : 1.0;
}

double TractionLimiter::limit_velocity(double & v)
{
    const double tmp = v;

    // Convert twist x to ackermann
    v = clamp(std::fabs(v), min_velocity_ / wheel_radius_, max_velocity_ / wheel_radius_);

    v *= tmp >= 0 ? 1 : -1;
    return tmp != 0.0 ? v / tmp : 1.0;
}

double TractionLimiter::limit_acceleration(double & v, double v0, double dt)
{
    const double tmp = v;

    double dv_min;
    double dv_max;
    if (abs(v) >= abs(v0))
    {
        dv_min = min_acceleration_ * dt;
        dv_max = max_acceleration_ * dt;
    }
    else
    {
        dv_min = min_deceleration_ * dt;
        dv_max = max_deceleration_ * dt;
    }
    double dv = clamp(std::fabs(v - v0), dv_min, dv_max);
    dv *= (v - v0 >= 0 ? 1 : -1);
    v = v0 + dv;

    return tmp != 0.0 ? v / tmp : 1.0;
}

double TractionLimiter::limit_jerk(double & v, double v0, double v1, double dt)
{
    const double tmp = v;

    const double dv = v - v0;
    const double dv0 = v0 - v1;

    const double dt2 = 2. * dt * dt;

    const double da_min = min_jerk_ * dt2;
    const double da_max = max_jerk_ * dt2;

    double da = clamp(std::fabs(dv - dv0), da_min, da_max);
    da *= (dv - dv0 >= 0 ? 1 : -1);
    v = v0 + dv0 + da;

    return tmp != 0.0 ? v / tmp : 1.0;
}
}   // namespace tricycle_controller
