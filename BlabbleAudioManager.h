/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#ifndef H_BlabbleAudioManagerPLUGIN
#define H_BlabbleAudioManagerPLUGIN

#include <string>
#include <map>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h> 

/*! @class A simple class to manage ringing.
 *  This class controls ringing (either via generated tone or
 *  a wave file used as a ringtone. It also handles playing the
 *  call waiting beep when another call comes in while on a call.
 */
class BlabbleAudioManager
{
public:
	BlabbleAudioManager(const std::string& wavPath);
	virtual ~BlabbleAudioManager();
	
	/*! @Brief Stop all media playing that was started by us.
	 */
	void StopRings();
	
	/*! @Brief Start playing the tone for an outgoing call.
	 */
	void StartRing();
	
	/*! @Brief Start playing the tone or ringtone for an outgoing call.
	 */
	void StartInRing();
	
	/*! @Brief Start playing the wave file fileName located in wavPath that was passed to the constructor.
	 */
	bool StartWav(const std::string& fileName);
	
	/*! @Brief Stop playing a wave played with StartWav.
	 *  @sa StartWav
	 */
	void StopWav();
private:
	std::string wav_path_;
	pj_pool_t* pool_;
	pjmedia_port *ring_port_, *in_ring_port_, *call_wait_ring_port_;
	pjsua_player_id in_ring_player_, wav_player_;
	pjsua_conf_port_id ring_slot_, in_ring_slot_, call_wait_slot_, 
		wav_slot_;
};

#endif
