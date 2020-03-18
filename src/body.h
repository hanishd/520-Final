#ifndef __BODY_AGENT__H
#define __BODY_AGENT__H 

#include "enviro.h"
#include "math.h"
#include <vector>
#include <string>
#include <iostream>

using namespace enviro;
using namespace std;

class BodyController : public Process, public AgentInterface {
    // The Body of the hero is the only part whose movement is fully controlled by the player/AI.

public:
    BodyController() : Process(), AgentInterface() {}

    void init() {
        body_v = 0.0;
        body_w = 0.0;
        hero_id = id();
        hero_HP = 20;
        hero_AI = true;
        weapon_angle = -0.2;
        shield_angle = 0.2;

        watch("keyup", [&](Event& e) {
            if (e.value()["client_id"] == get_client_id()) {
                std::string key = e.value()["key"];
                if (key == "q") {
                    hero_AI = not hero_AI;
                }
            }
        });
        watch("keydown", [&](Event& e) {
            if (e.value()["client_id"] == get_client_id()) {
                std::string key = e.value()["key"];
                if (key == "w" && body_v < vmax) {
                    body_v += delv;
                }
                else if (key == "s" && body_v > -0.5 * vmax) {
                    body_v -= delv;
                }
                else if (key == "a" && body_w > -1 * wmax) {
                    body_w -= delw;
                }
                else if (key == "d" && body_w < wmax) {
                    body_w += delw;
                }
            }
        });

        notice_collisions_with("SwarmA", [&](Event& e) {
            // a Swarm agent ran into you.  Take 1 damage and despawn that Swarm member.
            remove_agent(e.value()["id"]);
            emit(Event("SwarmDead", {
                { "id", e.value()["id"] },
                { "pts", 1 }
                }));
            emit(Event("MinusHP", {
                { "id", id() },
                { "dam", 2 }
                }));
            hero_HP -= 2;
        });
        notice_collisions_with("SwarmB", [&](Event& e) {
            // a Swarm agent ran into you.  Take 1 damage and despawn that Swarm member.
            remove_agent(e.value()["id"]);
            emit(Event("SwarmDead", {
                { "id", e.value()["id"] },
                { "pts", 1 }
                }));
            emit(Event("MinusHP", {
                { "id", id() },
                { "dam", 1 }
                }));
            hero_HP -= 1;
        });
        notice_collisions_with("SwarmC", [&](Event& e) {
            // a Swarm agent ran into you.  Take 1 damage and despawn that Swarm member.
            remove_agent(e.value()["id"]);
            emit(Event("SwarmDead", {
                { "id", e.value()["id"] },
                { "pts", 1 } // Yes, C is worth 2 normally.  This is the point for Body collision, which gets offset by a -1 in the MinusHP handling.
                }));
            emit(Event("MinusHP", {
                { "id", id() },
                { "dam", 1 }
                }));
            hero_HP -= 1;
        });
        notice_collisions_with("Weapon", [&](Event& e) {
            // You ran into a weapon.  Hopefully it wasn't your own.  You'll take 1 damage per tick that the weapon remains in contact, so you can die very quickly if you're not careful.
            emit(Event("MinusHP", {
                { "id", id() },
                { "dam", 1 }
                }));
            hero_HP -= 1;
        });
    }
    void start() {
    }
    void update() {
        damp_movement();
        center(x(), y()); // Center the camera on the player.

        body_x = position().x;
        body_y = position().y;
        body_angle = angle();

// At the center of the body, place a circle (the head) and display your current HP there.
        decorate("<circle x='-10' y='10' r='10' style='fill: blue'></circle>");
        label(to_string(hero_HP), 0, 5); // This is all we use hero_HP for.  The max HP set in the management script is what determines everything that matters.

        // Only use sensors if we're in AI mode.  Otherwise, listen for the keyboard.
        if (hero_AI) {
// Sensor IDs: 0-16 go from left to right, with #8 being due front, and #17 is back
// Before considering sensors, our desired path is to walk forward at a bit more than half speed.
// We then add a bit of randomness to both speed and direction to give it a bit of a random walk.
            float desired_v = (vmax / 2.0) + (((double)rand() / (double)RAND_MAX) * 6.0)*delv;
            float desired_w = (((double)rand() / (double)RAND_MAX) * 6.0 - 3.0) * delw;

// Initial choice for arm angles should have a bit of randomness.  They'll average to zero, but then get pulled a bit to the sides by the later logic.
            weapon_angle = ((double)rand() / (double)RAND_MAX) * 0.4 - 0.2;
            shield_angle = ((double)rand() / (double)RAND_MAX) * 0.4 - 0.2;

// Next we modify our path based on what the sensors pick up.
            int n_body = 0; // Number of sensors that spot non-AI enemies.  If you get close then multiple sensors can spot the same enemy.
            for (int ii = 0; ii < 17; ii++) {
                if (sensor_value(ii) < 200) {
// Set up a range scaling factor, "dr".  This is 0.25 at maximum range, 1.0 at range 50 or less.
// That way we can keep our sensor range long without letting extreme-range detections screw things up too badly.  Given that the arena is 1080 units from end to end, a circular radius of 200 will cover a sizeable fraction of the arena for pathfinding purposes.
                    float dr = (50.0 / sensor_value(ii)) < 1.0;
                    string stype = sensor_reflection_type(ii);
//                    cout << "Sensor " << ii << "  range: " << sensor_value(ii) << "  type: " << sensor_reflection_type(ii) << "\n";
                    if (stype == "StaticObject") {
// We spotted a wall.  Try to steer away from it.  The closer we are to the wall, the more pronounced this is.
// While on paper this effect isn't large (desired_w ranging from (0->2)*dr, there are 17 sensors on the half-circle and any barrier is likely to set off at least half of them.
// This makes the range dependence even more pronounced; if you get close to a wall, not only will dr max out at 1.0, but you'll have a half-dozen sensors telling you to turn.
                        desired_w += delw * (ii - 8)/4 * dr;
                        desired_v -= delv * (8 - abs(ii-8))/8 * dr;
                    }
                    if (stype == "SwarmA" || stype == "SwarmB" || stype == "SwarmC") {
// We spotted a Swarm enemy.  Try to steer towards it so that both weapon and shield can engage it, and slow down a bit.  The effect is four times as large as for static barriers, but each enemy will only set off at most one sensor.
// Note that if we spot multiple Swarm enemies (unlikely but possible), we'll try to steer harder, but we'll also want to slow down a bit more.
// If we spot enemies to both left and right, we'll try to stay centered between them and just modify our arm angles instead.
                        desired_w -= delw * (ii - 8);
                        desired_v -= delv * (8 - abs(ii - 8)) / 2;
                        // For each spotted enemy on our left side, move the weapon a bit further left.
                        // The closer the enemy is, the larger the adjustment.
                        if (ii < 11) {
                            weapon_angle += 0.2 * dr;
                        }
                        // For each spotted enemy on our right side, move the shield a bit further right.
                        if (ii > 5) {
                            shield_angle -= 0.2 * dr;
                        }
                    }
                    if (stype == "Body" || (sensor_value(ii) > 50.0 && (stype == "Weapon" || stype == "Shield"))) {
// We spotted another player (I hope).  Turn towards them as best you can, and slow down as you get close.
// To be clear, that if check says that if we spot a Body it MUST be an enemy, while if we spot a Weapon or Shield then it's only assumed to be an enemy if the range is greater than 50.
// That way, we can't accidentally set off a sensor with our own weapon or shield.  Besides, once you get close, at least one sensor WILL spot the enemy Body.
                        desired_w = wmax * (ii - 8) / 8;
                        desired_v = vmax / (4 * dr); // max speed at long range, 1/4th speed at close range.
// Aim your weapon and shield directly towards that enemy.
// Note that these angles are requests, not demands; the arms can only swing at a certain rate, so they'll try to aim towards these angles as best they can.
                        n_body += 1;
                        weapon_angle += 1.6 - 0.2 * ii;
                        shield_angle += 1.6 - 0.2 * ii;
// If multiple sensors spot this kind of enemy, it'll average them.
                    }
//                    cout << "  moo 4 " << desired_v << " " << desired_w << "\n";
                }
            }
            if (n_body > 0) {
                // Multiple sensors spotted a non-AI enemy.  Average the angles to aim for their centerpoint.
                weapon_angle = weapon_angle / n_body;
                shield_angle = shield_angle / n_body;
            }

            // Check the rear camera for walls
            if (sensor_value(17) < 100 && sensor_reflection_type(17) == "StaticObject") {
// If there's a wall behind us (unlikely unless the AI was backing up), the AI will be less likely to want to back up further.
                float dr = (50.0 / sensor_value(17)) < 1.0;
                desired_v += (vmax * dr)/2.0;
// At range 200 this adds vmax/8, giving a slight predisposition to forward movement.
// At range <50 this adds vmax/2, which should drown out any changes from sensors.
            }

            // Now that we know the speeds we want to travel at, cap the values based on the current speed.
/*            cout << "  moo 5 v=" << body_v << " w=" << body_w << " --- " << desired_v << " " << desired_w << "\n";
            do
            {
                cout << '\n' << "Press a key to continue...";
            } while (cin.get() != '\n');
            */
            if (desired_v < body_v && body_v > (-vmax/2.0)) { body_v -= delv; }
            if (desired_v > body_v && body_v < vmax) { body_v += delv; }
            if (desired_w < body_w && body_w > -1*wmax) { body_w -= delw; }
            if (desired_w > body_w && body_w < wmax) { body_w += delw; }

//            cout << "AI movement: v=" << body_v << " w=" << body_w << " weapon=" << weapon_angle << " shield=" << shield_angle << "\n";
        }

// Move the Body with the given velocity and angular speed.  The arms will be teleported along with it.
        track_velocity(body_v, body_w, 10, 10);

// To let the limbs know where the body is, emit an update event.
// This gives the X, Y, and angle of the body, whether the AI is driving (needed to disable the arm controls), and what angles the AI wants to aim each arm at.
        emit(Event("BodyUpdate", {
            { "x", body_x },
            { "y", body_y },
            { "angle", body_angle},
            { "ai", hero_AI},
            { "weapon", weapon_angle},
            { "shield", shield_angle}
        }));

    }
    void stop() {}

    int hero_id, hero_HP;
    bool hero_AI;

    const double vmax = 20.0;
    const double wmax = 1.0;
    const double delv = 1.0;
    const double delw = 0.05;
    double body_v, body_w;
    double body_x, body_y, body_angle;
    // Weapon and shield angle will only be set if the AI is turned on.
    double weapon_angle, shield_angle;
};

class Body : public Agent {
    public:
    Body(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    BodyController c;
};

DECLARE_INTERFACE(Body)

#endif