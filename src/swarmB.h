#ifndef __SWARM_B_AGENT__H
#define __SWARM_B_AGENT__H 

#include "enviro.h"
#include "math.h"

using namespace enviro;

class SwarmBController : public Process, public AgentInterface {

    public:
    SwarmBController() : Process(), AgentInterface() {}

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
        double targ_x = body_x + ((double)rand() / (double)RAND_MAX) * 100.0 - 50.0;
        double targ_y = body_y + ((double)rand() / (double)RAND_MAX) * 100.0 - 50.0;
        move_toward(targ_x, targ_y, 125, 40);

        // Check for collisions.  Collisions with a body are already being handled in body.h, so handle the other possibilities
        notice_collisions_with("Weapon", [&](Event& e) {
            remove_agent(my_id);
            emit(Event("SwarmDead", {
                { "id", my_id },
                { "pts", 1 }
            }));
        });
        notice_collisions_with("Shield", [&](Event& e) {
// If a B hits the shield, it bounces off.  The linear force will range from -50 to -200 (meaning it'll reverse its current movement), and the angular from -100 to +100
            double f1 = ((double)rand() / (double)RAND_MAX) * 150.0 - 200.0;
            double f2 = ((double)rand() / (double)RAND_MAX) * 200.0 - 100.0;
            apply_force(f1,f2);
        });
    }
    void stop() {}

    int my_id;
    double body_x, body_y, body_angle;

};

class SwarmB : public Agent {
    public:
    SwarmB(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    SwarmBController c;
};

DECLARE_INTERFACE(SwarmB)

#endif