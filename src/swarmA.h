#ifndef __SWARM_A_AGENT__H
#define __SWARM_A_AGENT__H 

#include "enviro.h"
#include "math.h"

using namespace enviro;

class SwarmAController : public Process, public AgentInterface {

    public:
    SwarmAController() : Process(), AgentInterface() {}

    void init() {
        my_id = id();

        body_x = 0;
        body_y = 0;
        body_angle = 0;
        watch("BodyUpdate", [this](Event e) {
            body_x = e.value()["x"];
            body_y = e.value()["y"];
            body_id = e.value()["id"];
            body_angle = e.value()["ang"];
        });
    }
    void start() {}
    void update() {
        damp_movement();

        // Unlike the B and C swarmers, an A goes straight for the Body with no randomness.
        move_toward(body_x, body_y, 75, 20);

        // Check for collisions.  Collisions with a body are already being handled in body.h, so handle the other possibilities
        notice_collisions_with("Weapon", [&](Event& e) {
            remove_agent(my_id);
            emit(Event("SwarmDead", {
                { "id", my_id },
                { "pts", 1 }
            }));
        });
        notice_collisions_with("Shield", [&](Event& e) {
// If an A swarmer hits the Shield, it explodes.  The Swarmer is killed for no points, but it'll also deal 1 damage to the Hero linked to that Shield.
            remove_agent(my_id);
            emit(Event("SwarmDead", {
                { "id", my_id },
                { "pts", 0 }
            }));
            emit(Event("MinusHP", {
                { "id", body_id },
                { "dam", 1 }
            }));
        });
    }
    void stop() {}

    int my_id;
    int body_id;
    double body_x, body_y, body_angle;

};

class SwarmA : public Agent {
    public:
    SwarmA(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    SwarmAController c;
};

DECLARE_INTERFACE(SwarmA)

#endif