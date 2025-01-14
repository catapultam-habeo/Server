#include "../client.h"

void command_shownpcgloballoot(Client *c, const Seperator *sep)
{
	if (!c->GetTarget() || !c->GetTarget()->IsNPC()) {
		c->Message(Chat::White, "You must target an NPC to use this command.");
		return;
	}

	const auto t = c->GetTarget()->CastToNPC();

	zone->ShowNPCGlobalLoot(c, t);
}

