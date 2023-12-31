#include "PacketHandler.h"

bool PacketHandler::handleNull(HANDLE_ARGS)
{
	return true;
}

bool PacketHandler::handleKeyCheck(ENetPeer *peer, ENetPacket *packet)
{
	KeyCheck *keyCheck = (KeyCheck*)packet->data;
	uint64 userId = _blowfish->Decrypt(keyCheck->checkId);

	if(userId == keyCheck->userId)
	{
		PDEBUG_LOG_LINE(Log::getMainInstance()," User got the same key as i do, go on!\n");
		peerInfo(peer)->keyChecked = true;
		peerInfo(peer)->userId = userId;
	}
	else
	{
		Log::getMainInstance()->errorLine(" WRONG KEY, GTFO!!!\n");
		return false;
	}

	//Send response as this is correct (OFC DO SOME ID CHECKS HERE!!!)
	KeyCheck response;
	response.userId = keyCheck->userId;
	response.netId = 0;
	
	return sendPacket(peer, reinterpret_cast<uint8*>(&response), sizeof(KeyCheck), CHL_HANDSHAKE);
}

bool PacketHandler::handleGameNumber(ENetPeer *peer, ENetPacket *packet)
{
	WorldSendGameNumber world;
	world.gameId = 1;
	memcpy(world.data, peerInfo(peer)->name, peerInfo(peer)->nameLen);

	return sendPacket(peer, reinterpret_cast<uint8*>(&world), sizeof(WorldSendGameNumber), CHL_S2C);
}

bool PacketHandler::handleSynch(ENetPeer *peer, ENetPacket *packet)
{
	SynchVersion *version = reinterpret_cast<SynchVersion*>(packet->data);
	Log::getMainInstance()->writeLine("Client version: %s\n", version->version);

	SynchVersionAns answer;
	answer.mapId = 1;
	//exhaust = 0x08A8BAE4, Cleanse = 0x064D2094, flash = 0x06496EA8
	answer.players[0].userId = peerInfo(peer)->userId;
	answer.players[0].skill1 = SPL_Exhaust;
	answer.players[0].skill2 = SPL_Cleanse;

	sendPacket(peer, reinterpret_cast<uint8*>(&answer), sizeof(SynchVersionAns), 3);

	return true;
}

bool PacketHandler::handleMap(ENetPeer *peer, ENetPacket *packet)
{
	LoadScreenPlayer *playerName = LoadScreenPlayer::create(PKT_S2C_LoadName, peerInfo(peer)->name, peerInfo(peer)->nameLen);
	playerName->userId = peerInfo(peer)->userId;

	LoadScreenPlayer *playerHero = LoadScreenPlayer::create(PKT_S2C_LoadHero,  peerInfo(peer)->type, peerInfo(peer)->typeLen);
	playerHero->userId = peerInfo(peer)->userId;
	playerHero->skinId = 0;

	//Builds team info
	LoadScreenInfo screenInfo;
	screenInfo.bluePlayerNo = 1;
	screenInfo.redPlayerNo = 0;

	screenInfo.bluePlayerIds[0] = peerInfo(peer)->userId;

	bool pInfo = sendPacket(peer, reinterpret_cast<uint8*>(&screenInfo), sizeof(LoadScreenInfo), CHL_LOADING_SCREEN);

	//For all players send this info
	bool pName = sendPacket(peer, reinterpret_cast<uint8*>(playerName), playerName->getPacketLength(), CHL_LOADING_SCREEN);
	bool pHero = sendPacket(peer, reinterpret_cast<uint8*>(playerHero), playerHero->getPacketLength(), CHL_LOADING_SCREEN);

	//cleanup
	LoadScreenPlayer::destroy(playerName);
	LoadScreenPlayer::destroy(playerHero);

	return (pInfo && pName && pHero);

}

//building the map
bool PacketHandler::handleSpawn(ENetPeer *peer, ENetPacket *packet)
{
	uint8 bounds[] = {0x2c, 0x19, 0x00, 0x00, 0x40, 0x99, 0x14, 0x00, 0x00, 0x99, 0x14, 0x00, 0x00, 0x7f, 0x14, 0x00,\
		0x00, 0x7f, 0x14, 0x00, 0x00, 0x7f, 0x14, 0x00, 0x00, 0x7f, 0x14, 0x00, 0x00, 0x7f, 0x14, 0x00,\
		0x00, 0xe1, 0x14, 0x00, 0x00, 0xe1, 0x14, 0x00, 0x00, 0xe1, 0x14, 0x00, 0x00, 0x99, 0x14, 0x00,\
		0x00, 0x99, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00,\
		0x00, 0xa9, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00,\
		0x00, 0xc5, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00,\
		0x00, 0xa9, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00, 0x00, 0xa9, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00,\
		0x00, 0xa9, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00, 0x00, 0xc5, 0x14, 0x00, 0x00, 0x95, 0x86, 0x5e,\
		0x06, 0xa8, 0x6e, 0x49, 0x06, 0x64, 0x37, 0x00, 0x00, 0x01, 0x44, 0x36, 0x00, 0x00, 0x02, 0x41,\
		0x36, 0x00, 0x00, 0x01, 0x52, 0x37, 0x00, 0x00, 0x04, 0x71, 0x36, 0x00, 0x00, 0x01, 0x43, 0x36,\
		0x00, 0x00, 0x03, 0x42, 0x36, 0x00, 0x00, 0x03, 0x41, 0x37, 0x00, 0x00, 0x01, 0x64, 0x36, 0x00,\
		0x00, 0x01, 0x92, 0x36, 0x00, 0x00, 0x01, 0x52, 0x36, 0x00, 0x00, 0x04, 0x43, 0x37, 0x00, 0x00,\
		0x03, 0x61, 0x36, 0x00, 0x00, 0x01, 0x82, 0x36, 0x00, 0x00, 0x03, 0x62, 0x36, 0x00, 0x00, 0x01,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1e};

	uint8 turrets1[] = {0xff, 0x06, 0x4f, 0xa6, 0x01, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72,\
		0x72, 0x65, 0x74, 0x5f, 0x54, 0x31, 0x5f, 0x52, 0x5f, 0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5,\
		0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04,\
		0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00,\
		0x80, 0x01, 0xff, 0x01, 0x4a, 0x02, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74,\
		0x5f, 0x54, 0x31, 0x5f, 0x52, 0x5f, 0x30, 0x32, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76,\
		0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00,\
		0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff,\
		0x01, 0x4a, 0x03, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x31,\
		0x5f, 0x43, 0x5f, 0x30, 0x37, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59,\
		0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00,\
		0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x04,\
		0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x52, 0x5f,\
		0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,\
		0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9,\
		0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8,\
		0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x05, 0x00, 0x00, 0x40,\
		0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x52, 0x5f, 0x30, 0x32, 0x5f,\
		0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03,\
		0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2,\
		0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38,\
		0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x06, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75,\
		0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x52, 0x5f, 0x30, 0x31, 0x5f, 0x41, 0x00, 0x74,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00,\
		0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74,\
		0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00,\
		0x00, 0x80, 0x01, 0x12, 0x00, 0x00, 0x00, 0x00};

	uint8 turrets2[] = {0xff, 0x06, 0x4f, 0xa6, 0x07, 0x00, 0x00, 0x40, 0x07, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72,\
		0x72, 0x65, 0x74, 0x5f, 0x54, 0x31, 0x5f, 0x43, 0x5f, 0x30, 0x35, 0x5f, 0x41, 0x00, 0x74, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5,\
		0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04,\
		0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00,\
		0x80, 0x01, 0xff, 0x01, 0x4a, 0x08, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74,\
		0x5f, 0x54, 0x31, 0x5f, 0x43, 0x5f, 0x30, 0x34, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76,\
		0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00,\
		0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff,\
		0x01, 0x4a, 0x09, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x31,\
		0x5f, 0x43, 0x5f, 0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59,\
		0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00,\
		0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x0a,\
		0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x31, 0x5f, 0x43, 0x5f,\
		0x30, 0x31, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,\
		0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9,\
		0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8,\
		0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x0b, 0x00, 0x00, 0x40,\
		0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x31, 0x5f, 0x43, 0x5f, 0x30, 0x32, 0x5f,\
		0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03,\
		0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2,\
		0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38,\
		0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x0c, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75,\
		0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x43, 0x5f, 0x30, 0x35, 0x5f, 0x41, 0x00, 0x74,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00,\
		0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74,\
		0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00,\
		0x00, 0x80, 0x01};

	uint8 turrets3[] = {0xff, 0x06, 0x4f, 0xa6, 0x0d, 0x00, 0x00, 0x40, 0x0d, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72,\
		0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x43, 0x5f, 0x30, 0x34, 0x5f, 0x41, 0x00, 0x74, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5,\
		0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04,\
		0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00,\
		0x80, 0x01, 0xff, 0x01, 0x4a, 0x0e, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74,\
		0x5f, 0x54, 0x32, 0x5f, 0x43, 0x5f, 0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76,\
		0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00,\
		0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff,\
		0x01, 0x4a, 0x0f, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32,\
		0x5f, 0x43, 0x5f, 0x30, 0x31, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0xa0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59,\
		0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00,\
		0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x10,\
		0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x43, 0x5f,\
		0x30, 0x32, 0x5f, 0x41, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,\
		0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9,\
		0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8,\
		0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x11, 0x00, 0x00, 0x40,\
		0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x4f, 0x72, 0x64, 0x65, 0x72, 0x54, 0x75, 0x72,\
		0x72, 0x65, 0x74, 0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03,\
		0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2,\
		0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38,\
		0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x12, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75,\
		0x72, 0x72, 0x65, 0x74, 0x5f, 0x43, 0x68, 0x61, 0x6f, 0x73, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74,\
		0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00,\
		0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74,\
		0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00,\
		0x00, 0x80, 0x01};

uint8 turrets4[] = {0xff, 0x07, 0x4f, 0xa6, 0x13, 0x00, 0x00, 0x40, 0x13, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72,\
		0x72, 0x65, 0x74, 0x5f, 0x54, 0x31, 0x5f, 0x4c, 0x5f, 0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x53,\
		0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5,\
		0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04,\
		0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00,\
		0x80, 0x01, 0xff, 0x01, 0x4a, 0x14, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74,\
		0x5f, 0x54, 0x31, 0x5f, 0x4c, 0x5f, 0x30, 0x32, 0x5f, 0x41, 0x00, 0x74, 0x53, 0x68, 0x72, 0x69,\
		0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76,\
		0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00,\
		0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff,\
		0x01, 0x4a, 0x15, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x31,\
		0x5f, 0x43, 0x5f, 0x30, 0x36, 0x5f, 0x41, 0x00, 0x74, 0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f,\
		0x41, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59,\
		0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00,\
		0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x16,\
		0x00, 0x00, 0x40, 0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x4c, 0x5f,\
		0x30, 0x33, 0x5f, 0x41, 0x00, 0x74, 0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02,\
		0x00, 0x00, 0x03, 0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9,\
		0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8,\
		0x18, 0x00, 0x38, 0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x17, 0x00, 0x00, 0x40,\
		0x40, 0x54, 0x75, 0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x4c, 0x5f, 0x30, 0x32, 0x5f,\
		0x41, 0x00, 0x74, 0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03,\
		0x00, 0x1f, 0x00, 0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2,\
		0xb9, 0xa8, 0x74, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38,\
		0x14, 0x68, 0x00, 0x00, 0x80, 0x01, 0xff, 0x01, 0x4a, 0x18, 0x00, 0x00, 0x40, 0x40, 0x54, 0x75,\
		0x72, 0x72, 0x65, 0x74, 0x5f, 0x54, 0x32, 0x5f, 0x4c, 0x5f, 0x30, 0x31, 0x5f, 0x41, 0x00, 0x74,\
		0x53, 0x68, 0x72, 0x69, 0x6e, 0x65, 0x5f, 0x41, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x1f, 0x00,\
		0xc5, 0x16, 0x68, 0x76, 0xdd, 0x46, 0x59, 0x5d, 0xe2, 0xf9, 0x33, 0x77, 0xf2, 0xb9, 0xa8, 0x74,\
		0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xe8, 0xf8, 0x18, 0x00, 0x38, 0x14, 0x68, 0x00,\
		0x00, 0x80, 0x01, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00};

	HeroSpawnPacket *heroSpawn = HeroSpawnPacket::create(PKT_HeroSpawn,  peerInfo(peer)->name, peerInfo(peer)->nameLen, peerInfo(peer)->type, peerInfo(peer)->typeLen);

	sendPacket(peer, reinterpret_cast<uint8*>(heroSpawn), heroSpawn->getPacketLength(), 3);
	sendPacket(peer, reinterpret_cast<uint8*>(bounds), sizeof(bounds), 3);
	sendPacket(peer, reinterpret_cast<uint8*>(turrets1), sizeof(turrets1), 3);
	sendPacket(peer, reinterpret_cast<uint8*>(turrets2), sizeof(turrets2), 3);
	sendPacket(peer, reinterpret_cast<uint8*>(turrets3), sizeof(turrets3), 3);
	sendPacket(peer, reinterpret_cast<uint8*>(turrets4), sizeof(turrets4), 3);

	
	return true;
}

bool PacketHandler::handleInit(HANDLE_ARGS)
{
	uint8 resp[] = {0xff, 0x18, 0x06, 0x60, 0x00, 0x00, 0x00, 0x00, 0x24, 0x12, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x50, 0xc0, 0x01, 0x00, 0x00, 0x40, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00,\
		0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x01, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00,\
		0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x43, 0x02, 0x00, 0x00, 0x40,\
		0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0x70, 0x44, 0x03, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c,\
		0x00, 0x00, 0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x04, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01,\
		0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x43, 0x05, 0x00,\
		0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x70, 0x44, 0x06, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac,\
		0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x07, 0x00, 0x00, 0x40, 0x53, 0x01,\
		0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x43,\
		0x08, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x00, 0x70, 0x44, 0x09, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00,\
		0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x81, 0x0e, 0x45, 0x0a, 0x00, 0x00, 0x40,\
		0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x81,\
		0x0e, 0x45, 0x0b, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c,\
		0x00, 0x00, 0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x0c, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01,\
		0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x43, 0x0d, 0x00,\
		0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0x70, 0x44, 0x0e, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac,\
		0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x81, 0x0e, 0x45, 0x0f, 0x00, 0x00, 0x40, 0x53, 0x01,\
		0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x81, 0x0e, 0x45,\
		0x10, 0x00, 0x00, 0x40, 0x53, 0x02, 0x00, 0x01, 0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x50, 0xc3, 0x46, 0x12, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00,\
		0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x13, 0x00, 0x00, 0x40,\
		0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
		0xd2, 0x43, 0x14, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x44, 0x15, 0x00, 0x00, 0x40, 0x53, 0x01, 0x00, 0x01,\
		0x01, 0x00, 0x43, 0xac, 0xd4, 0x0c, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x0c, 0xf0, 0x43, 0x16, 0x00,\
		0x00, 0x40, 0x53, 0x01, 0x00, 0x01, 0x01, 0x00, 0x03, 0xa9, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00,\
		0x00, 0x00, 0xd2, 0x43, 0x17, 0x00, 0x00, 0x40};

	sendPacket(peer, reinterpret_cast<uint8*>(resp), sizeof(resp), 3);
	//sendPacket(peer, reinterpret_cast<uint8*>(fog), sizeof(fog), 4);
	
	return true;

}

bool PacketHandler::handleAttentionPing(ENetPeer *peer, ENetPacket *packet)
{
	AttentionPing *ping = reinterpret_cast<AttentionPing*>(packet->data);
	AttentionPingAns response(ping);

	Log::getMainInstance()->writeLine("Plong x: %f, y: %f, z: %f, type: %i\n", ping->x, ping->y, ping->z, ping->type);

	return broadcastPacket(reinterpret_cast<uint8*>(&response), sizeof(AttentionPing), 3);
}

bool PacketHandler::handleView(ENetPeer *peer, ENetPacket *packet)
{
	ViewReq *request = reinterpret_cast<ViewReq*>(packet->data);

	Log::getMainInstance()->writeLine("View (%i), x:%f, y:%f, zoom: %f\n", request->requestNo, request->x, request->y, request->zoom);

	ViewAns answer;
	answer.requestNo = request->requestNo;
	return sendPacket(peer, reinterpret_cast<uint8*>(&answer), sizeof(ViewAns), 3, UNRELIABLE);
}

bool PacketHandler::handleMove(ENetPeer *peer, ENetPacket *packet)
{
	MoveReq *request = reinterpret_cast<MoveReq*>(packet->data);

	Log::getMainInstance()->writeLine("Move to: x(left->right):%f, y(height):%f, z(bot->top): %f\n", request->x1, request->y1, request->z1);
	return true;
}

bool PacketHandler::handleLoadPing(ENetPeer *peer, ENetPacket *packet)
{
	PingLoadInfo *loadInfo = reinterpret_cast<PingLoadInfo*>(packet->data);

	PingLoadInfo response;
	memcpy(&response, packet->data, sizeof(PingLoadInfo));
	response.header.cmd = PKT_S2C_Ping_Load_Info;
	response.userId = peerInfo(peer)->userId;


	Log::getMainInstance()->writeLine("Loading: loaded: %f, ping: %f, %i, %f\n", loadInfo->loaded, loadInfo->ping, loadInfo->unk4, loadInfo->f3);
	return broadcastPacket(reinterpret_cast<uint8*>(&response), sizeof(PingLoadInfo), 4, UNRELIABLE);
}

bool PacketHandler::handleQueryStatus(HANDLE_ARGS)
{
	QueryStatus response;
	return sendPacket(peer, reinterpret_cast<uint8*>(&response), sizeof(QueryStatus), 3);
}

bool PacketHandler::handleChatBoxMessage(HANDLE_ARGS)
{
	ChatBoxMessage* message = reinterpret_cast<ChatBoxMessage*>(packet->data);

	switch(message->cmd)
	{
	case CMT_ALL:
	//!TODO make a player class and foreach player in game send the message
		return sendPacket(peer,packet->data,packet->dataLength,CHL_COMMUNICATION);
		break;
	case CMT_TEAM:
	//!TODO make a team class and foreach player in the team send the message
		return sendPacket(peer,packet->data,packet->dataLength,CHL_COMMUNICATION);
		break;
	default:
		Log::getMainInstance()->errorLine("Unknow ChatMessageType");
		break;
	}
	return false;
}

bool PacketHandler::handleSkillUp(HANDLE_ARGS) {

	SkillUpPacket* skillUpPacket = reinterpret_cast<SkillUpPacket*>(packet->data);
	//!TODO Check if can up skill? :)
	SkillUpResponse skillUpResponse;
	
	skillUpResponse.skill = skillUpPacket->skill;
	skillUpResponse.level = 0x0001;

	// TESTING
	/* Eleazan
	uint8 heroRealSpawn[] = {
		0xFF, 0x08, 0x09, 0xCA, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x24, 0xF3, 0x36, 0x12, 0xCB, 0x00, 0xE7,\
		0x15, 0xC2, 0x3C, 0x78, 0xC3, 0x19, 0x00, 0x00, 0x40, 0x01, 0x06, 0x01, 0x00, 0xD1, 0x07, 0x00,\
		0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00, 0x6B, 0x9C, 0x0F, 0x42, 0xA5, 0xC6, 0x88, 0x43, 0x7D,\
		0x1B, 0x44, 0x3F, 0xBB, 0x8D, 0x24, 0x3F, 0x22, 0xB6, 0x00, 0x00, 0x00, 0xED, 0x43, 0x00, 0x00,\
		0xED, 0x43, 0x0A, 0xC3, 0xFC, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B,\
		0xFE, 0x00, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(heroRealSpawn), sizeof(heroRealSpawn), 3);
	*/

	uint8 lightHeroAndTop[] = {
		0xFF, 0x08, 0x09, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x97, 0xC4, 0x08, 0x37, 0x12, 0xCB, 0x00, 0xBE,\
		0x12, 0xAA, 0x3C, 0x78, 0xC3, 0x19, 0x00, 0x00, 0x40, 0x01, 0x06, 0x01, 0x00, 0xD1, 0x07, 0x00,\
		0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00, 0x6B, 0x9C, 0x0F, 0x42, 0xA5, 0xC6, 0x88, 0x43, 0x7D,\
		0x1B, 0x44, 0x3F, 0xBB, 0x8D, 0x24, 0x3F, 0x22, 0xB6, 0x00, 0x00, 0x80, 0xF8, 0x43, 0x00, 0x80,\
		0xF8, 0x43, 0x0A, 0xC3, 0xFC, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B,\
		0xFE, 0x00, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightHeroAndTop), sizeof(lightHeroAndTop), 3);

	uint8 lightMid[] = {
		0xFF, 0x04, 0x07, 0xC3, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B, 0xFF,\
		0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightMid), sizeof(lightMid), 3);

	uint8 lightBot[] = {
		0xFF, 0x03, 0x07, 0xC3, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x0B, 0xFF, 0x00, 0x00, 0x0B, 0xFF,\
		0x00, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightBot), sizeof(lightBot), 3);

	uint8 lightNexus[] = {
		0xFF, 0x02, 0x06, 0xC3, 0x4C, 0x36, 0xEB, 0xFF, 0x00, 0x05, 0x71, 0x71, 0xB7, 0xFF, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightNexus), sizeof(lightNexus), 3);

	uint8 lightBase[] = {
		0xFF, 0x02, 0x06, 0xC3, 0x36, 0xB8, 0x53, 0xFF, 0x00, 0x05, 0xDB, 0xC6, 0x10, 0xFF, 0x00
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightBase), sizeof(lightBase), 3);

	uint8 lightSubNexus1[] = {
		0xFF, 0x12, 0x06, 0xC3, 0x3E, 0x3C, 0xD2, 0xFF, 0x00, 0x05, 0xF1, 0x20, 0x4A, 0xFF, 0x00, 0x70,\
		0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48,\
		0x44, 0x01, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x1A, 0x00, 0x00, 0x40, 0xBD, 0x92, 0x58,\
		0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x02,\
		0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x1B, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x03, 0x00, 0x00,\
		0x40, 0x00, 0x50, 0xC3, 0x46, 0x1C, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01,\
		0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x04, 0x00, 0x00, 0x40, 0x00,\
		0x50, 0xC3, 0x46, 0x1D, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00,\
		0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x05, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3,\
		0x46, 0x1E, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFE,\
		0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x06, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x1F,\
		0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF,\
		0xFF, 0x00, 0x00, 0x48, 0x44, 0x07, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x20, 0x00, 0x00,\
		0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00,\
		0x00, 0x48, 0x44, 0x08, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x21, 0x00, 0x00, 0x40, 0xAD,\
		0x50, 0x98, 0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48,\
		0x44, 0x09, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x22, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98,\
		0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x7A, 0x44, 0x0A,\
		0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x23, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73,\
		0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x7A, 0x44, 0x0B, 0x00, 0x00,\
		0x40, 0x00, 0x50, 0xC3, 0x46, 0x24, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01,\
		0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x0C, 0x00, 0x00, 0x40, 0x00,\
		0x50, 0xC3, 0x46, 0x25, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00,\
		0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x0D, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3,\
		0x46, 0x26, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFE,\
		0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x0E, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x27,\
		0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF,\
		0xFF, 0x00, 0x00, 0x7A, 0x44, 0x0F, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x28, 0x00, 0x00,\
		0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00,\
		0x00, 0x7A, 0x44, 0x10, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x29, 0x00, 0x00, 0x40, 0xAD,\
		0x50, 0x98, 0x05
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightSubNexus1), sizeof(lightSubNexus1), 3);

	uint8 lightSubNexus2[] = {
		0xFF, 0x09, 0x21, 0x26, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,\
		0x00, 0x00, 0xC8, 0x44, 0x12, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x2A, 0x00, 0x00, 0x40,\
		0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00,\
		0x48, 0x44, 0x13, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x2B, 0x00, 0x00, 0x40, 0xAD, 0x50,\
		0x98, 0x05, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44,\
		0x14, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x2C, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05,\
		0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x15, 0x00,\
		0x00, 0x40, 0x00, 0x50, 0xC3, 0x46, 0x2D, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00,\
		0x01, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x16, 0x00, 0x00, 0x40,\
		0x00, 0x50, 0xC3, 0x46, 0x2E, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00,\
		0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x17, 0x00, 0x00, 0x40, 0x00, 0x50,\
		0xC3, 0x46, 0x2F, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0x73, 0x00, 0x01, 0x00, 0x00, 0x00,\
		0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x48, 0x44, 0x18, 0x00, 0x00, 0x40, 0x00, 0x50, 0xC3, 0x46,\
		0x30, 0x00, 0x00, 0x40, 0xAD, 0x50, 0x98, 0x05, 0xD6, 0x84, 0x00, 0x03, 0x00, 0x19, 0x00, 0x00,\
		0x40, 0x02, 0x00, 0x00, 0x70, 0x41, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00,\
		0x00, 0x40, 0x78, 0x00, 0x00, 0x0C, 0x42, 0x00, 0x00, 0x52, 0x43, 0x00, 0x00, 0x00, 0x00, 0x19,\
		0x00, 0x00, 0x40, 0x79, 0x00, 0x00, 0x11, 0x43, 0x00, 0x00, 0x91, 0x42, 0x00, 0x00, 0x87, 0x43,\
		0x50, 0xC0, 0x19, 0x00, 0x00, 0x40, 0x02, 0x01, 0x01, 0x00, 0x05, 0x63, 0xF9, 0x06, 0x00, 0x00,\
		0x00, 0x00, 0x00, 0x24, 0x74, 0x48, 0x19, 0x00, 0x00, 0x40
	};
	sendPacket(peer, reinterpret_cast<uint8*>(lightSubNexus2), sizeof(lightSubNexus2), 3);	

	return sendPacket(peer, reinterpret_cast<uint8*>(&skillUpResponse),sizeof(skillUpResponse),CHL_GAMEPLAY);

}