"""Story beats that need more than walking into a map.

progression.py derives a beat automatically for every on-entry cutscene in the
game -- 142 of them -- which covers the scripts that fire by themselves. What
it cannot derive is the beats a player has to *do*: take the starter, talk to
the person holding the plot, win a gym battle. Those go here.

A beat says where it happens, what the player must already have done for the
script to take its real branch, what to do on arrival, and what the game should
believe afterwards.

    map     the map directory name
    pos     where to warp in (defaults to the middle of the map)
    setup   harness lines -- setflag, setvar -- standing in for earlier beats
    act     harness lines -- walk, press, advance
    expect  (kind, name, value) where kind is "flag", "var" or "var-not"

`setup` is the honest weakness of this file: get a prerequisite wrong and the
beat tests the wrong branch of the script. Every line here is taken from the
map's own scripts.pory, and the positions from its events.inc, not guessed.
"""

BEATS = [
    dict(
        # events.inc puts Chikorita's ball at (8, 4) and the tile below it is
        # walkable; scripts.pory sets VAR_ELM_LAB_STATE to 1 once Elm has
        # finished telling the player to pick one.
        name="hand:starter-chikorita",
        map="NewBarkTown_ProfessorElmsLab", pos=(8, 5),
        setup=["setvar VAR_ELM_LAB_STATE 1"],
        act=["wait 30", "press UP", "wait 30", "press A", "advance"],
        expect=[("flag", "FLAG_SYS_POKEMON_GET", 1)],
    ),
]

# Beats the harness cannot judge, and why. A skipped beat is reported, not
# counted as a failure, so nothing is dropped quietly: each line here is a
# statement that a human still has to play this one.
UNRUNNABLE = {
    "DragonsDen_Shrine:0":
        "multichoice quiz -- mashing A answers question 5 wrong and the elder "
        "asks again for ever, which is the script doing its job",
    "Route36_NationalParkGatehouse:0":
        "the bug contest award ceremony reads the contest results, and forcing "
        "the state without running a contest feeds it a garbage species",
    "Route36_NationalParkGatehouse:1":
        "as above -- the ceremony needs a contest that actually happened",
    "DragonsDen:0":
        "the Clair scene only plays for a player leaving the shrine with the "
        "RISINGBADGE and no TM02; the map's own transition script switches it "
        "off for anyone else, including the harness",
    "MtMoon_1F:0":
        "the rival ambush needs the rival still undefeated, and the transition "
        "script sets the trigger past it for a save that has beaten him",
}

# Whole maps the harness does not judge. Battle Frontier is parked (Q3) and the
# Hoenn contest and mystery-event maps are cut; their entry scripts all wait on
# challenge state that only the facility itself sets.
UNRUNNABLE_MAPS = (
    "BattleFrontier_", "TrainerHill_", "ContestHall", "LilycoveCity_Contest",
    "SootopolisCity_MysteryEventsHouse", "_BattleTent",
)


def unrunnable(beat):
    for prefix in UNRUNNABLE_MAPS:
        if prefix in beat["map"]:
            return "parked or cut content: %s" % prefix.strip("_")
    return UNRUNNABLE.get(beat["name"])
