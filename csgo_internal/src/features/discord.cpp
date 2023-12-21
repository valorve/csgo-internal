#include "discord.hpp"
#include "../base_includes.hpp"

#ifndef _DEBUG
#include <discord_register.h>
#include <discord_rpc.h>

#pragma comment(lib, "discord-rpc")

void discord::init() {
	DiscordEventHandlers h;
	memset(&h, 0, sizeof(h));
	Discord_Initialize(("1103631900477771796"), &h, 1, NULL);
}

void discord::update() {
	DiscordRichPresence p;
	memset(&p, 0, sizeof(p));
	p.state = ("Hanging out with weave");
	p.startTimestamp = time(0);
	p.largeImageKey = ("weave_logo");
	//p.largeImageText = STRSC("weyvy beta");
	Discord_UpdatePresence(&p);
}

#endif