#include "framedata.h"
#include "CharData.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"

PlayersInteractionState interaction;

const std::list<std::string> idleWords =
{ "_NEUTRAL", "CmnActStand", "CmnActStandTurn", "CmnActStand2Crouch",
"CmnActCrouch", "CmnActCrouchTurn", "CmnActCrouch2Stand",
"CmnActFWalk", "CmnActBWalk",
"CmnActJumpLanding",
// Proxi block is triggered when an attack is closing in without being actually blocked
// If the player.blockstun is = 0, then those animations are still considered idle
"CmnActMidGuardPre", "CmnActMidHeavyGuardPre",
"CmnActMidGuardEnd", "CmnActMidHeavyGuardEnd",
"CmnActCrouchGuardEnd", "CmnActCrouchHeavyGuardLoop",
"CmnActAirGuardEnd",
// Character specifics
"com3_kamae" // Mai 5xB stance
};

bool isDoingActionInList(const char currentAction[], const std::list<std::string>& listOfActions)
{
    const std::string currentActionString = currentAction;

    for (auto word : listOfActions)
    {
        if (currentActionString == word)
        {
            return true;
        }
    }
    return false;
}

bool isIdle(CharData& player)
{
    if (player.u_hitstunRelated == 0 && (isDoingActionInList(player.currentAction, idleWords) || player.hitstun == 0))
    {
        return true;
    }
    return false;
}

bool isBlocking(CharData& player)
{
    if (player.blockstun > 0 || player.moveSpecialBlockstun > 0)
        return true;
    return false;
}

bool isInHitstun(CharData& player)
{
    // hitstun alone does not cover air cases (reaches 0 even when the opponent is still in actual hitstun)
    // hitstunLeftover reflects hitstun in the air as well, except after an emergency tech.
    if (player.hitstun > 0 && !isDoingActionInList(player.currentAction, idleWords))
    {
        return true;
    }
    return false;
}

void getFrameAdvantage(CharData& player1, CharData& player2)
{
    bool isIdle1 = isIdle(player1);
    bool isIdle2 = isIdle(player2);
    bool isStunned = isBlocking(player2) || isInHitstun(player2);
    int frameAdvantage;

    if (!isIdle1 && !isIdle2 && !interaction.started)
    {
        interaction.started = true;
        interaction.timer = 0;
    }
    if (interaction.started)
    {
        if (isIdle1 && isIdle2)
        {
            interaction.started = false;
            frameAdvantage = interaction.timer;
        }
        if (!isIdle1)
        {
            interaction.timer -= interaction.increment;
        }
        if (!isIdle2)
        {
            interaction.timer += interaction.increment;
        }
    }
}


void checkActionTimeDifference(CharData& player1, CharData& player2)
{
    int diff1 = player1.actionTime - interaction.prevPlayer1ActionTime;
    int diff2 = player2.actionTime - interaction.prevPlayer2ActionTime;

    // If one or both players are frozen in time, then the frame advantage should not be impacted.
    (diff1 == 1 && diff2 == 1) ? interaction.increment = 1 : interaction.increment = 0;

    interaction.prevPlayer1ActionTime = player1.actionTime;
    interaction.prevPlayer2ActionTime = player2.actionTime;
}

void computeGaps(CharData& player, int& gapCounter, int& gapResult)
{
    interaction.inBlockstring = isBlocking(player) || isInHitstun(player);
 
    if (interaction.inBlockstring)
    {
        if (gapCounter > 0 && gapCounter <= 30)
        {
            gapResult = gapCounter;
        }
        gapCounter = 0; //resets everytime you are in block or hit stun
    }
    else if (!interaction.inBlockstring)
    {
        ++gapCounter;
        gapResult = -1;
    }
}

void computeFramedataInteractions()
{
    if (!isInMatch && !(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater))
        return;

    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr())
    {
        CharData& player1 = *g_interfaces.player1.GetData();
        CharData& player2 = *g_interfaces.player2.GetData();

        // Crash if framedata window is opened in VS screen and transitions into training mode
        
        computeGaps(player1, interaction.p1Gap, interaction.p1GapDisplay);
        computeGaps(player2, interaction.p2Gap, interaction.p2GapDisplay);
        checkActionTimeDifference(player1, player2);
        getFrameAdvantage(player1, player2);
    }
}