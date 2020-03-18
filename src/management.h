#ifndef __MANAGEMENT_AGENT__H
#define __MANAGEMENT_AGENT__H 

#include "enviro.h"
#include <chrono>
#include <iostream>
#include <string>
//#include "json/json.h"

using namespace enviro;
using namespace std;
using namespace std::chrono;

class ManagementController : public Process, public AgentInterface {

    public:
    ManagementController() : Process(), AgentInterface() {}
    // The Management handles the overarching events and spawn rates.
    // It also displays a noninteractive scoreboard in the center of the arena.

    void init() {
        n_swarm = 0;
        score = 0;
        swarm_mode = false;
        hero_HP = max_HP;

        watch("connection", [&](Event e) {
            if (!e.value()["client_id"].is_null()) {
                std::cout << "Connection from " << e.value() << "\n";
                double spawn_ang = (rand() % 10) * 0.31;
                double body_x = 200.0 * cos(spawn_ang);
                double body_y = 200.0 * sin(spawn_ang);
                double body_angle = -1 * spawn_ang;

// Spawn a new body, weapon, and shield.  Tie each to this client.
                Agent& newBody = add_agent("Body", body_x, body_y, body_angle, BODY_STYLE);
                newBody.set_client_id(e.value()["client_id"]);
                Agent& newWeapon = add_agent("Weapon", body_x - (20.0 * cos(body_angle)), body_y - (20.0 * sin(body_angle)), body_angle+0.2, WEAPON_STYLE);
                newWeapon.set_client_id(e.value()["client_id"]);
                Agent& newShield = add_agent("Shield", body_x + (20.0 * cos(body_angle)), body_y + (20.0 * sin(body_angle)), body_angle-0.2, SHIELD_STYLE);
                newShield.set_client_id(e.value()["client_id"]);
                // Emit a body update just to initialize things in the subsidiary functions.
                emit(Event("BodyUpdate", {
                    { "x", body_x },
                    { "y", body_y },
                    { "angle", body_angle},
                    { "ai", false},
                    { "weapon", 0.2},
                    { "shield", -0.2}
                }));
            }
        });

// Pressing the "m" key toggles Swarm Mode.  This also resets the timer on this mode, so that you always go back to 1 swarm member when the mode starts.
// Note that this does NOT kill any active Swarm members.  Kill those the old-fashioned way, please.
        watch("keyup", [&](Event& e) {
            if (e.value()["client_id"] == get_client_id()) {
                std::string key = e.value()["key"];
                if (key == "m") {
                    swarm_mode = not swarm_mode;
                    if (swarm_mode) {
                        // We're in Swarm mode.  Start at 0 points.  Gain points per swarmer you kill.
                        time_start = high_resolution_clock::now();
                        n_swarm = 0; // This'll increment to 1 in the first Update call.
                        score = 0;
                    }
                    else {
                        // We're not in Swarm mode.  Start at 10 points, lose 1 points per hit you take.  Obviously, you'll be at 0 if you die, and 2*HP if you kill the other player.
                        score = max_HP;
                    }
                    hero_HP = max_HP; // Switching modes resets your HP.  Yes, this can be abused.  Don't do it.
                }
            }
        });

        watch("SwarmDead", [this](Event e) {
// A member of the swarm died somewhere else.  Record the totals.
            auto junk_id = e.value()["id"]; // Not using this for anything at the moment besides a debug output
            int pts = e.value()["pts"];
            cout << "Swarm unit died: " << junk_id << "\n";
            n_swarm -= 1;
            score += pts;
        });

        watch("MinusHP", [this](Event e) {
// You've taken a point of damage.  Do the appropriate bookkeeping.
            int dam = e.value()["dam"];
            hero_HP -= dam;
            auto hero_id = e.value()["id"];
            cout << "Hero " << hero_id << " lost " << dam << " HP; now down to " << hero_HP <<" HP.\n";
            score -= 1; // In Swarm mode this offsets the "kill" point for swarmers who died hitting your body.
            // In non-Swarm mode there's no way to gain score, but you started at 10 points.  If you die you'll be at 0, but if you kill the other player then your final score will be however many HP you had left.

            if (hero_HP == 0) {
                // You ran out of lives.  Game over, man.
                remove_agent(hero_id);
                cout << "GAME OVER \n";
                cout << "Final Score: " << score << "\n";
                abort(); // Could replace this with a splash screen and allow the user to restart.
            }
        });
    }

    void start() {}
    void update() {
        // In the center of the screen, show the current HP and score.
        label("HP: " + to_string(hero_HP), -10, 10);

        if (swarm_mode) {
            // The swarm spawns new entities based on time since the simulation started, using the stopwatch logic from our earlier homework.
            // At the start it spawns 1, and if that one is killed a new one immediately replaces it.
            label("Score: " + to_string(score), -10, -10);

            high_resolution_clock::duration time_elapsed = (high_resolution_clock::now() - time_start);

            typedef std::chrono::duration<double, std::ratio<1, 1>> sec_type;
            dt = sec_type(time_elapsed).count();

            // Every five seconds, the maximum increases by 1, up to a maximum of 20 active swarm members at a time (after 95 seconds).
            int swarm_max = ((dt / 5.0) + 1) < 20;
            if (n_swarm < swarm_max) {
                // Swarm enemies can spawn anywhere on a circle 500 units from the center (i.e., just inside the Arena edge).
                // The new swarm entity will be at that location facing towards the center initially.
//                double spawn_ang = (rand() % 4) * 0.785;
                double spawn_ang = ((double)rand() / (double)RAND_MAX) * 2.0 * 3.14159265358979;
                // Pick one of the three Swarm types, randomly.
                int Swarmtype = (rand() % 3);
                if (Swarmtype == 0) {
                    Agent& newSwarm = add_agent("SwarmA", 500 * cos(spawn_ang), 500 * sin(spawn_ang), -1.0 * spawn_ang, SWARM_A_STYLE);
                } else if (Swarmtype == 1) {
                    Agent& newSwarm = add_agent("SwarmB", 500 * cos(spawn_ang), 500 * sin(spawn_ang), -1.0 * spawn_ang, SWARM_B_STYLE);
                } else {
                    Agent& newSwarm = add_agent("SwarmC", 500 * cos(spawn_ang), 500 * sin(spawn_ang), -1.0 * spawn_ang, SWARM_C_STYLE);
                }
//                newSwarm.set_client_id(e.value()["client_id"]); // Not used for anything at the moment.  If we do this, move the declaration of newSwarm outside the IF block.
                n_swarm += 1;
            }

        }
    }
    void stop() {}

    high_resolution_clock::time_point time_start; // time that START was pushed for the most recent interval.
    double dt;
    int n_swarm;
    bool swarm_mode;
    const int max_HP = 20;
    int hero_HP = max_HP;
    int score;
    const json  SWARM_A_STYLE = {
                {"fill", "blue"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
    const json  SWARM_B_STYLE = {
                {"fill", "green"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
    const json  SWARM_C_STYLE = {
                {"fill", "red"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
    const json  BODY_STYLE = {
                {"fill", "lightgreen"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
    const json  WEAPON_STYLE = {
                {"fill", "red"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
    const json  SHIELD_STYLE = {
                {"fill", "black"},
                {"stroke", "black"},
                {"strokeWidth", "10px"},
                {"strokeOpacity", "0.25"} };
};

class Management : public Agent {
    public:
    Management(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }
    private:
    ManagementController c;
};

DECLARE_INTERFACE(Management)

#endif