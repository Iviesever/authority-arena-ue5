#include "Ability/AuthorityArenaGameplayTags.h"

namespace AuthorityArenaTags
{
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dash, "Ability.Dash", "Predicted dash ability");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "Server-authoritative projectile attack");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Shield, "Ability.Shield", "Energy-backed shield ability");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Actor is dead and cannot activate abilities");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Actor is stunned and cannot activate abilities");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Shield_Active, "State.Shield.Active", "Shield damage reduction is active");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash cooldown");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Attack, "Cooldown.Attack", "Attack cooldown");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Shield, "Cooldown.Shield", "Shield cooldown");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Failure_Dead, "Failure.Dead", "Activation rejected because actor is dead");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Failure_Stunned, "Failure.Stunned", "Activation rejected because actor is stunned");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Failure_Resource, "Failure.Resource", "Activation rejected for insufficient resources");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Failure_Cooldown, "Failure.Cooldown", "Activation rejected because cooldown is active");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Failure_Target, "Failure.Target", "Activation rejected because the server target is invalid");
}
