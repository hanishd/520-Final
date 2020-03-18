#ifndef __SHIELD_AGENT__H
#define __SHIELD_AGENT__H 

#include "enviro.h"

using namespace enviro;

class ShieldController : public Process, public AgentInterface {
    // The defensive bit.  The Shield is rigidly tied to the right arm of the Body, and can be pivoted.

    public:
    ShieldController() : Process(), AgentInterface() {}

    void init() {
        shield_x = 0;
        shield_y = 0;
        shield_angle = 0.2;  // Start off ~20 degrees right of forward

        body_x = 0;
        body_y = 0;
        body_angle = 0;
        watch("BodyUpdate", [this](Event e) {
// Unlike with the keyboard events, a Body update will be sent from non-active windows, so we can't use the client ID as before.
            if (e.value()["client_id"] == get_client_id()) {
                body_x = e.value()["x"];
                body_y = e.value()["y"];
                body_angle = e.value()["angle"];
                body_AI = e.value()["ai"];
                body_shield = e.value()["shield"];

                if (body_AI) {
                    // If we're in AI mode, instead of just setting shield_angle equal to body_shield, we want to rotate towards it at the same speed a user would get by holding down the key.
                    if (body_shield > shield_angle) {
                        shield_angle = (shield_angle + dtheta) < (body_shield < 2.0);
                    }
                    else if (body_shield < shield_angle) {
                        shield_angle = (shield_angle - dtheta) > (body_shield > -0.5);
                    }
                }
            }
        });

        // The shield is mounted on the right arm.  It can be rotated between -0.5 radians (~29 degrees left of forward) and 2.0 radians (~116 degrees right).
        watch("keydown", [&](Event& e) {
            if (e.value()["client_id"] == get_client_id()) {
                std::string key = e.value()["key"];
                if (key == "i" && shield_angle < 2.0) {
                    shield_angle += dtheta;
                }
                else if (key == "o" && shield_angle > -0.5) {
                    shield_angle -= dtheta;
                }
            }
        });
    }
    void start() {}
    void update() {
//        damp_movement();

        // The shield pivots on a point 20 units right of the body center, if the body were facing upward.
        shield_x = body_x + (20.0 * cos(body_angle));
        shield_y = body_y + (20.0 * sin(body_angle));

        teleport(shield_x, shield_y, (shield_angle + body_angle));
        //        move_toward(body_x, body_y, 400, 200);
    }
    void stop() {}

    double dtheta = 0.1;
    double shield_x, shield_y, shield_angle;
    double body_x, body_y, body_angle, body_shield;
    bool body_AI;

};

class Shield : public Agent {
    public:
    Shield(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    ShieldController c;
};

DECLARE_INTERFACE(Shield)

#endif