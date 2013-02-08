/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include "BlabbleAudioManager.h"

BlabbleAudioManager::BlabbleAudioManager(const std::string& wavPath) :
	wav_path_(wavPath), wav_player_(-1)
{
	pj_str_t name = pj_str(const_cast<char*>("inring"));
	pjmedia_tone_desc tone[3];
	pj_status_t status;

	pool_ = pjsua_pool_create("blabble", 4096, 4096);
	if (pool_ == NULL)
		throw std::runtime_error("Ran out of memory creating pool!");

	tone[0].freq1 = 440;
	tone[0].freq2 = 480;
	tone[0].on_msec = 2000;
	tone[0].off_msec = 1000;
	tone[1].freq1 = 440;
	tone[1].freq2 = 480;
	tone[1].on_msec = 2000;
	tone[1].off_msec = 4000;
	tone[2].freq1 = 440;
	tone[2].freq2 = 480;
	tone[2].on_msec = 2000;
	tone[2].off_msec = 3000;

	try {
		in_ring_slot_ = -1;
		in_ring_player_ = -1;

		std::string path = 
//#if WIN32
//				wavPath + "\\ringtone.wav";
//#else
				wavPath + "/ringtone.wav";
//#endif
		pj_str_t ring_file = pj_str(const_cast<char*>(path.c_str()));

		if (pjsua_player_create(&ring_file, 0, &in_ring_player_) == PJ_SUCCESS)
		{
			in_ring_slot_ = pjsua_player_get_conf_port(in_ring_player_);
		}

		if (in_ring_slot_ < 0)
		{
			//We don't have the wav ringtone, use a modified tone
			status = pjmedia_tonegen_create2(pool_, &name, 8000, 1, 160, 16, PJMEDIA_TONEGEN_LOOP, &in_ring_port_);
			if (status != PJ_SUCCESS)
				throw std::runtime_error("Failed inring pjmedia_tonegen_create2");

			status = pjmedia_tonegen_play(in_ring_port_, 1, tone, PJMEDIA_TONEGEN_LOOP);
			if (status != PJ_SUCCESS)
				throw std::runtime_error("Failed inring pjmedia_tonegen_play");

			status = pjsua_conf_add_port(pool_, in_ring_port_, &in_ring_slot_);
			if (status != PJ_SUCCESS)
				throw std::runtime_error("Failed inring pjsua_conf_add_port");

		}

		tone[0].off_msec = 4000;
		name = pj_str(const_cast<char*>("ring"));

		status = pjmedia_tonegen_create2(pool_, &name, 8000, 1, 160, 16, PJMEDIA_TONEGEN_LOOP, &ring_port_);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed ring pjmedia_tonegen_create2");

		status = pjmedia_tonegen_play(ring_port_, 3, tone, PJMEDIA_TONEGEN_LOOP);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed ring pjmedia_tonegen_play");

		status = pjsua_conf_add_port(pool_, ring_port_, &ring_slot_);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed ring pjsua_conf_add_port");

		tone[0].freq1 = 440;
		tone[0].freq2 = 0;
		tone[0].on_msec = 500;
		tone[0].off_msec = 2000;
		tone[1].freq1 = 440;
		tone[1].freq2 = 0;
		tone[1].on_msec = 500;
		tone[1].off_msec = 4000;
		name = pj_str(const_cast<char*>("call_wait"));

		status = pjmedia_tonegen_create2(pool_, &name, 8000, 1, 160, 16, 
			PJMEDIA_TONEGEN_LOOP, &call_wait_ring_port_);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed call_wait pjmedia_tonegen_create2");

		status = pjmedia_tonegen_play(call_wait_ring_port_, 2, tone, PJMEDIA_TONEGEN_LOOP);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed call_wait pjmedia_tonegen_play");

		status = pjsua_conf_add_port(pool_, call_wait_ring_port_, &call_wait_slot_);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Failed call_wait pjsua_conf_add_port");
	}
	catch (std::runtime_error& e) 
	{
		pj_pool_release(pool_);
		pool_ = NULL;
		throw e;
	}
}

void BlabbleAudioManager::StopRings()
{
	pjsua_conf_disconnect(ring_slot_, 0);
	pjmedia_tonegen_rewind(ring_port_);

	if (in_ring_slot_ > -1)
	{
		pjsua_conf_disconnect(in_ring_slot_, 0);
	}
	if (in_ring_player_ > -1)
	{
		pjsua_player_set_pos(in_ring_player_, 0);
	}

	pjsua_conf_disconnect(call_wait_slot_, 0);
	pjmedia_tonegen_rewind(call_wait_ring_port_);
}

void BlabbleAudioManager::StartRing()
{
	pjsua_conf_connect(ring_slot_, 0);
}

void BlabbleAudioManager::StartInRing()
{
	if (pjsua_call_get_count() > 1)
	{
		pjsua_conf_connect(call_wait_slot_, 0);
	}
	else
	{
		pjsua_conf_connect(in_ring_slot_, 0);
	}
}

BlabbleAudioManager::~BlabbleAudioManager()
{
}

bool BlabbleAudioManager::StartWav(const std::string& fileName)
{
	if (wav_player_ > 0)
		return false;

	std::string path = 
#if WIN32
				wav_path_ + "\\" + fileName;
#else
				wav_path_ + "/" + fileName;
#endif
	pj_str_t wav_file = pj_str(const_cast<char*>(path.c_str()));
	if (pjsua_player_create(&wav_file, 0, &wav_player_) == PJ_SUCCESS)
	{
		wav_slot_ = pjsua_player_get_conf_port(wav_player_);
		pjsua_conf_connect(wav_slot_, 0);
		return true;
	}

	wav_player_ = -1;
	return false;
}

void BlabbleAudioManager::StopWav()
{
	if (wav_player_ > 0) 
	{
		pjsua_conf_disconnect(wav_slot_, 0);
		pjsua_player_destroy(wav_player_);
		wav_player_ = -1;
	}
}
