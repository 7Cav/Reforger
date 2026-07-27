//------------------------------------------------------------------------------------------------
//! Disables the ACE "Metal Clanging" radial command for 7Cav servers.
//! CanBePerformed() gates the menu entry client-side.
//! Execute() is the server-side backstop so the sound never plays even if
//! something else routes to this command.
modded class ACE_MetalClangingCommand
{
    //------------------------------------------------------------------------------------------------
    override bool CanBePerformed(notnull SCR_ChimeraCharacter user)
    {
        return false;
    }

    //------------------------------------------------------------------------------------------------
    override bool Execute(IEntity cursorTarget, IEntity target, vector targetPosition, int playerID, bool isClient)
    {
        return false;
    }
}
