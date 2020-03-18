#ifndef __SWARM_C_AGENT__H
#define __SWARM_C_AGENT__H 

#include "enviro.h"
#include "math.h"

using namespace enviro;

class SwarmCController : public Process, public AgentInterface {

    public:
    SwarmCController() : Process(), AgentInterface() {}

    void init() {
        my_id = id();

        body_x = 0;
        body_y = 0;
        body_angle = 0;
        watch("BodyUpdate", [this](Event e) {
            body_x = e.value()["x"];
            body_y = e.value()["y"];
            body_angle = e.value()["ang"];
        });
    }
    void start() {}
    void update() {
        damp_movement();

        // Add some randomness to the spot the swarm element is aiming for.
        double targ_x = body_x + ((double)rand() / (double)RAND_MAX) * 200.0 - 100.0;
        double targ_y = body_y + ((double)rand() / (double)RAND_MAX) * 200.0 - 100.0;
        move_toward(targ_x, targ_y, 200, 100);

        // Check for collisions.  Collisions with a body are already being handled in body.h, so handle the other possibilities
        notice_collisions_with("Weapon", [&](Event& e) {
            remove_agent(my_id);
            emit(Event("SwarmDead", {
                { "id", my_id },
                { "pts", 2 }
            }));
        });
        notice_collisions_with("Shield", [&](Event& e) {
// For a C swarmer, the Shield acts as a second Weapon, killing it and giving points.
            remove_agent(my_id);
            emit(Event("SwarmDead", {
                { "id", my_id },
                { "pts", 2 }
            }));
        });
    }
    void stop() {}

    int my_id;
    double body_x, body_y, body_angle;

};

class SwarmC : public Agent {
    public:
    SwarmC(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    SwarmCController c;
};

DECLARE_INTERFACE(SwarmC)

#endif