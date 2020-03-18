#ifndef __WEAPON_AGENT__H
#define __WEAPON_AGENT__H 

#include "enviro.h"
#include "math.h"

using namespace enviro;

class WeaponController : public Process, public AgentInterface {
    // The stabby bit.  The Weapon is rigidly tied to the left arm of the Body, but can be pivoted.

    public:
    WeaponController() : Process(), AgentInterface() {}

    void init() {
        weapon_x = 0;
        weapon_y = 0;
        weapon_angle = 0.2;  // Start off ~20 degrees left of forward

        body_x = 0;
        body_y = 0;
        body_angle = 0;
        watch("BodyUpdate", [this](Event e) {
            if (e.value()["client_id"] == get_client_id()) {
                body_x = e.value()["x"];
                body_y = e.value()["y"];
                body_angle = e.value()["angle"];
                body_AI = e.value()["ai"];
                body_weapon = e.value()["weapon"];

                if (body_AI) {
// If we're in AI mode, instead of just setting weapon_angle equal to body_weapon, we want to rotate towards it at the same speed a user would get by holding down the appropriate key.
                    if (body_weapon > weapon_angle) {
                        weapon_angle = (weapon_angle + dtheta) < (body_weapon < 0.5);
                    } else if (body_weapon < weapon_angle) {
                        weapon_angle = (weapon_angle - dtheta) > (body_weapon > -2.0);
                    }
                }
            }
        });

        // The weapon is mounted on the left arm.  It can be rotated between +0.5 radians (~29 degrees right of forward) and -2.0 radians (~115 degrees left).
        watch("keydown", [&](Event& e) {
            if (e.value()["client_id"] == get_client_id()) {
                std::string key = e.value()["key"];
                if (key == "k" && weapon_angle < 0.5) {
                    weapon_angle += dtheta;
                }
                else if (key == "l" && weapon_angle > -2.0) {
                    weapon_angle -= dtheta;
                }
            }
        });
    }
    void start() {}
    void update() {
//        damp_movement();

        // The weapon pivots on a point 20 units left of the body center, if the body were facing upward.
        weapon_x = body_x - (20.0 * cos(body_angle));
        weapon_y = body_y - (20.0 * sin(body_angle));

        teleport(weapon_x, weapon_y, (weapon_angle+body_angle));
        //        move_toward(body_x, body_y, 400, 200);
    }
    void stop() {}

    double dtheta = 0.1;
    double weapon_x, weapon_y, weapon_angle;
    double body_x, body_y, body_angle, body_weapon;
    bool body_AI;

};

class Weapon : public Agent {
    public:
    Weapon(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    WeaponController c;
};

DECLARE_INTERFACE(Weapon)

#endif