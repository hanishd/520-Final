THE BILESTOAD 2

Based (loosely) on the computer game "The Bilestoad", (c)1982 by Datamost.

The goal of this project is to create a semi-interactive video game based on the old Apple ][ PC game of the same name.  The "Hero" consists of three linked objects: the Body, the Shield, and the Weapon.  He uses these elements to fight enemies.
In the original game, two players shared a single keyboard to fight.  Combat was very slow and difficult to manage, with the keyboard controls used to slowly adjust rotations.  For instance, each rotation (body, weapon, shield) had three keys: rotate left, rotate right, and stop rotation.  I have replaced this with a simpler system that adds or subtracts rotation rate as long as you hold down one of two keys.
The game included dismemberment (with obviously crude graphics), teleporting, flying discs, and more.  Those elements won't be included.

FILE CONTENTS
The provided files require the ENVIRO package (https://github.com/klavinslab/enviro), which itself requires ELMA (https://github.com/klavinslab/elma), as well as the JSON library (https://github.com/nlohmann/json).  A stamdard Docker interface can access all of these; instructions are on the ENVIRO page.
The files included in this package are as follows:
config.json: Defines the initial positions of the Arena and four permanent entities
defs/*.json: Defines the shapes of the game entities, both permanent and temporary
src/*.cc: C++ files.  These contain only a basic wrapper; all of the appropriate logic and declarations are made in the corresponding .H files.
src/*.h: Define the logic for the various game components.  A more detailed description of each will be included in the appropriate section below.

INSTALLATION AND EXECUTION
On Windows 10 Home or earlier, this software must be installed in a subdirectory of C:/Users/(yourname), a requirement of Docker Toolbox.  More advanced versions of Win10 use Docker and do not suffer this restriction.  Both Docker and Docker Toolbox are available for free online.
cd to the installed location
"docker run -p80:80 -p8765:8765 -v $PWD:/source -it klavins/enviro:v1.61 bash"
"esm start"
If this is your first time using this package, you must execute a "make clean" followed by "make".
"enviro" (This starts the required web server; CTRL-C aborts this.  Note that this server suspends input in the console window.)

CONTENTS
This project consists of four key phases.  This is entirely internal; each phase is used to gate the more complex content for testing purposes.

PHASE 1: The Basics
The Hero is implemented, controlled by keyboard input.

No enemies are active in this phase.  The purpose of phase 1 is to test keyboard-controlled movement.  While this document lists keys using their uppercase counterparts, the program actually parses for lowercase characters.

BODY: The part that matters most.  (src/body.h)
Any enemy hits on the Body costs Hit Points (HPs).  When the Hero reaches zero HP (beginning at 20HP), the game ends.  The Body is the part whose movement is directly controlled, either by the player or the AI.  The Body starts with 10 HP.
The Body is moved with the W, A, S, and D keys.  These adjust velocities, both linear and rotational, within certain caps.  Hold W down longer and you'll move forward more quickly, for instance, up to a certain maximum velocity.
The Body's name is "Marc Goodman" and it is left-handed.  (Marc created the original game, and the characters in the game were all left-handed.)

SHIELD: A high-mass entity that blocks incoming attacks, either mitigating damage or buying time for other actions.  (src/shield.h)
The Shield is moved back and forth with the I and O keys, pivoting on the right side of the Body.  I moves the Shield left, O moves it right, and releasing both keys freezes the shield at the current angle relative to the body.
Its range of movement is from +30 degrees (somewhat left of forward) to -120 degrees (right).
In an ideal world, the shield's actual movement is not rigidly tied to the Body; it'd try to move to the correct location under its own power, which'd allow it to be "pushed" by collisions.  But the ENVIRO move_towards does snot allow for an angle argument.
So, in this program the shield arm is rigidly constrained, pivoting on the "shoulder" point near the right edge of the Body.

WEAPON: Enemies that collide with the weapon are damaged or killed.  (src/weapon.h)
Swarm enemies die instantly, and Hero enemies lose HP if the Weapon strikes their Body.  The Weapon has low mass and inertia.
The Weapon is moved back and forth with the K and L keys, pivoting on the left side of the Body.  K moves the Weapon left, L moves it right, and releasing both keys freezes the shield at the current angle relative to the body.
Its range of movement is from -30 degrees (somewhat right of forward) to +120 degrees (left).
As with the Shield, the Weapon is rigidly tied to one end of the body.

MANAGER: A noninteractive arena entity that tracks your score.  (src/management.h)
The "Manager" is a small octagon in the center of the arena and displays your current score and HP.  Internally, it also handles much of the hidden logic that ties everything else together, keeping track of the various broadcast events used to pass information between components.

The graphics set uses somewhat detailed models.  As the game tracks collisions, the shapes used actually matter to gameplay.  Originally I'd used simple geometric shapes for testing purposes (rectangle for Body and Shield, triangle for Weapon) but they now more closely match the items they represent.
The Arena is a large octagon, so the edges are not likely to have much effect on gameplay.  These walls are indestructible and immobile.
The keys used are not the same as in the original game, because the original game could only be played by two players on a single keyboard.  This made the controls notoriously difficult to manage, although it added the metagame of trying to interfere with your opponent's typing.
The mouse is not used.

PHASE 2: AI Mode
A rudimentary AI is given to the Hero

As an alternative to the keyboard control, the Hero can be set to AI control.  The "Q" key toggles this mode.  The Body uses 18 sensors to identify targets and walls (17 sensors covering the forward half-circle, and one aft sensor).
On each tick the body will semi-randomly pick its movements, subject to the same acceleration limits as the keyboard control.  That is, if holding down the "A" or "D" key changes your rotation rate by 0.05 radians per second per tick, then the AI annot change your rotation speed by more than that amount.

BODY: The body randomly wanders the arena, generally moving towards nearby enemies but if seriously outnumbered might back away to better defend its unprotected rear arc.  If it picks a single target to attack (in non-Swarm mode) then it'll try to charge toward that target, but slow down as it approaches.

SHIELD: The Shield attempts to place itself between the Body and nearby enemies on the right side or front.

WEAPON: The Weapon attempts to place itself between the Body and nearby enemies on the left side or front.

Again, no enemies are active in this phase.  Without any enemies there's no real way to see most of these behaviors; I manually created temporary "enemy" targets to test with.


PHASE 3: The Swarm
AI-controlled Swarm enemies are added

(src/swarmA.h, src/swarmB.h, src/swarmC.h)

Pressing M toggles Swarm Mode.  Activating this mode will begin spawning Swarm enemies from the edges of the arena.  They spawn at the four cardinal direction edges and move toward the Body, in a somewhat random pattern.
There is no sensor involved; a swarmer always knows the location of the Body.  The randomness comes from varying the point they're moving towards, which has enough uncertainty that the faster swarmers can miss entirely.
A Swarm enemy that impacts the Shield will vary based on its type.  More on this below.
A Swarm enemy that impacts the Weapon will be destroyed, and the player's score increases.
A Swarm enemy that impacts the Body will deal damage and be destroyed.  The Body starts with 20HP; when it runs out, the game ends.
As time progresses the number of Swarm enemies increases; eventually, the sheer number will probably overwhelm the player.  (The maximum spawn rate occurs after ~95 seconds, when you'll have 20 simultaneous enemies.)  The objective is to have the highest score when this happens.

The Swarm consists of three distinct types of enemy.  When a Swarm enemy is spawned by the Management script, its type will be randomly chosen.

Type A: the "Boomer". Large, deals two damage if it hits the Body, worth one point
Goes straight towards the Hero at a slow speed.  If it hits the Shield it explodes, dying, but also dealing 1 damage to the linked Body.  This means that the only safe way to deal with an A is to interpose your Weapon; given the size, slow speed, and predictable trajectory, this isn't too hard.

Type B: the "Bouncer". Medium-sized, deals one damage, worth one point
Movement is somewhat imprecise.  If it hits the Shield, a type B will bounce off in a random direction, and come back again and again until it deals damage or is destroyed.

Type C: the "Speeder".  Tiny, deals one damage, worth two points
Fast, but very imprecise movement such that it's likely to miss a moving target.  Dies if it hits the Shield, giving the same two points as for the Weapon; in other words, for this enemy type the Shield acts as a second Weapon in every way.


PHASE 4: PvP
Multiple Hero units can spawn

Each browser window spawns and controls a unique Hero unit.  Keyboard controls only apply to the Hero controlled by the active browser window.
Instead of the Swarm, the Hero units fight each other to the death.  As with Swarm Mode, each player starts with 20HP.  Each "tick" where your Weapon touches another player's Body will deal 1 damage to them, so it's important to back away or use the Shield to limit contact.
Score, while maintained, is basically irrelevant as it matches your current HP.

The Management entity is unique to your Hero.  It tracks only your score/HP, and Swarm entities it spawn (if you turn on Swarm mode) will only home in on your player, although they will damage any other Hero entities they collide with along the way.
The AI logic from Phase 2, if used, can handle the presence of Swarm enemies AND other Heroes; in general, enemy Hero units receive a much higher priority for action.  In theory, then, a human player could choose to face Swarm enemies AND AI-controlled Hero units for additional challenge.