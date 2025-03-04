/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _CLIENT_H_
#define _CLIENT_H_

// client.h

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
	char	average; //johnfitz
	char	peak; //johnfitz
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	int		ping;
	byte	translations[VID_GRADES * 256];

	char	userinfo[8192];
} scoreboard_t;

// begin rook / woods 8/31/2021

// JPG - added this for teamscore status bar and proquake messages
typedef struct
{
	int colors;
	int frags;
} teamscore_t;



// end rook / woods 8/31/2021




typedef struct
{
	int		destcolor[3];
	float	percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_DLIGHTS		64 //johnfitz -- was 32
typedef struct
{
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
	vec3_t	color;				//johnfitz -- lit support via lordhavoc
} dlight_t;


#define	MAX_BEAMS	32 //johnfitz -- was 24
typedef struct
{
	int		entity;
	struct qmodel_s* model;
	float	endtime;
	vec3_t	start, end;
	const char* trailname;
	struct trailstate_s* trailstate;
} beam_t;

#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected		// valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	cactive_t	state;

	// personalization data sent to server
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

// demo loop control
	int		demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];	// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;

	// did the user pause demo playback? (separate from cl.paused because we don't
	// want a svc_setpause inside the demo to actually pause demo playback).
	qboolean	demopaused;

	qboolean	timedemo;
	int		forcetrack;		// -1 = use normal cd track
	FILE* demofile;
	int		td_lastframe;		// to meter out one message a frame
	int		td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

// connection information
	int		signon;			// 0 to SIGNONS
	struct qsocket_s* netcon;
	sizebuf_t	message;		// writing buffer to send to server

//downloads don't restart/fail when the server sends random serverinfo packets
	struct
	{
		qboolean active;
		unsigned int size;
		FILE* file;
		char	current[MAX_QPATH];	//also prevents us from repeatedly trying to download the same file
		char	temp[MAX_OSPATH];		//the temp filename for the download, will be renamed to current
		float	starttime;
	} download;

	char userinfo[8192];
	//Spike -- menuqc stuff.
	qcvm_t menu_qcvm;
} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the
								// first frame
	int			ackedmovemessages;	// echo of movemessages from the server.
	usercmd_t	movecmds[64];	// ringbuffer of previous movement commands (journal for prediction)
#define MOVECMDS_MASK (countof(cl.movecmds)-1)
	usercmd_t	pendingcmd;		// accumulated state from mice+joysticks.

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	float		statsf[MAX_CL_STATS];
	char* statss[MAX_CL_STATS];
	int			items;			// inventory bit flags
	float	item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

// pitch drifting vars
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;

	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start

	double		mtime[2];		// the timestamp of last two messages
	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups


	float		last_received_message;	// (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
	struct qmodel_s* model_precache[MAX_MODELS];
	struct qmodel_s* model_precache_csqc[MAX_MODELS];
	struct sfx_s* sound_precache[MAX_SOUNDS];

	char		mapname[128];
	char		levelname[128];	// for display on solo scoreboard //johnfitz -- was 40.
	int			viewentity;		// cl_entitites[cl.viewentity] = player
	int			maxclients;
	int			gametype;

	// refresh related state
	struct qmodel_s* worldmodel;	// cl_entitites[0].model
	struct efrag_s* free_efrags;
	int			num_efrags;
	//	int			num_entities;	// held in cl_entities array
	//	int			num_statics;	// held in cl_staticentities array
	entity_t	viewent;			// the gun model

	entity_t* entities;	//spike -- moved into here
	int			max_edicts;
	int			num_entities;

	entity_t** static_entities; //spike -- was static
	int			max_static_entities;
	int			num_statics;

	int			cdtrack, looptrack;	// cd audio

// frag scoreboard
	scoreboard_t* scores;		// [cl.maxclients]

	unsigned	protocol; //johnfitz
	unsigned	protocolflags;
	unsigned	protocol_pext1;	//spike -- flag of fte protocol extensions
	unsigned	protocol_pext2;	//spike -- flag of fte protocol extensions
	qboolean	protocol_dpdownload;

#ifdef PSET_SCRIPT
	qboolean	protocol_particles;
	struct
	{
		const char* name;
		int index;
	} particle_precache[MAX_PARTICLETYPES];
	struct
	{
		const char* name;
		int index;
	} local_particle_precache[MAX_PARTICLETYPES];
#endif
	int ackframes[8];	//big enough to cover burst
	unsigned int ackframes_count;
	qboolean requestresend;

	char stuffcmdbuf[1024];	//comment-extensions are a thing with certain servers, make sure we can handle them properly without further hacks/breakages. there's also some server->client only console commands that we might as well try to handle a bit better, like reconnect
	char printbuffer[1024];
	enum
	{
		PRINT_NONE,
		PRINT_PINGS,
		//		PRINT_STATUSINFO,
		//		PRINT_STATUSPLAYER,
		//		PRINT_STATUSIP,
	} printtype;
	int printplayer;
	float expectingpingtimes;
	float printversionresponse;

	//spike -- moved this stuff here to deal with downloading content named by the server
	qboolean sendprespawn;	//download+load content, send the prespawn command once done
	int		model_count;
	int		model_download;
	char	model_name[MAX_MODELS][MAX_QPATH];
	char	model_name_csqc[MAX_MODELS][MAX_QPATH];	//negative indexes in the csqc are csqc ones.
	int		sound_count;
	int		sound_download;
	char	sound_name[MAX_SOUNDS][MAX_QPATH];
	//spike -- end downloads

	qcvm_t	qcvm;	//for csqc.
	float	csqc_sensitivity;	//scaler for sensitivity
	size_t	ssqc_to_csqc_max;
	edict_t** ssqc_to_csqc;		//to find the csqc ent for an ssqc index.

	qboolean	listener_defined;
	vec3_t		listener_origin;
	vec3_t		listener_axis[3];

	char serverinfo[8192];	// \key\value infostring data.


	// begin rook / woods 8/31/2021


	teamscore_t* teamscores;		// [13] - JPG for teamscores in status bar                   
	qboolean		teamgame;			// JPG = true for match, false for individual
	int				minutes;			// JPG - for match time in status bar
	int				seconds;			// JPG - for match time in status bar
	double			last_match_time;	// JPG - last time match time was obtained
	double			last_ping_time;		// JPG - last time pings were obtained
	double			version_time;		// 
	qboolean		console_ping;		// JPG 1.05 - true if the ping came from the console
	double			last_status_time;	// JPG 1.05 - last time status was obtained
	qboolean		console_status;		// JPG 1.05 - true if the status came from the console
	double			match_pause_time;	// JPG - time that match was paused (or 0)
	vec3_t			lerpangles;			// JPG - angles now used by view.c so that smooth chasecam doesn't fuck up demos
	vec3_t			death_location;		// JPG 3.20 - used for %d formatting


	// end rook / woods 8/31/2021



} client_state_t;


//
// cvars
//
extern	cvar_t	cl_name;
extern	cvar_t	cl_topcolor, cl_bottomcolor;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_alwaysrun; // QuakeSpasm

extern	cvar_t	cl_autofire;

extern	cvar_t	cl_recordingdemo;
extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern	cvar_t	cfg_unbindall;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;


#define	MAX_TEMP_ENTITIES			256		//johnfitz -- was 64

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];
extern	entity_t		cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t			cl_beams[MAX_BEAMS];
extern	entity_t** cl_visedicts;
extern	int				cl_numvisedicts;
extern	int				cl_maxvisedicts;	//extended if we exceeded it the previous frame

//=============================================================================

//
// cl_main
//
dlight_t* CL_AllocDlight(int key);
void	CL_DecayLights(void);

void CL_Init(void);

void CL_EstablishConnection(const char* host);
void CL_Signon1(void);
void CL_Signon2(void);
void CL_Signon3(void);
void CL_Signon4(void);

void CL_Disconnect(void);
void CL_Disconnect_f(void);
void CL_NextDemo(void);

void SV_UpdateInfo(int edict, const char* keyname, const char* value);

//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput(void);
void CL_AccumulateCmd(void);
void CL_SendCmd(void);
void CL_SendMove(const usercmd_t* cmd);
int  CL_ReadFromServer(void);
void CL_AdjustAngles(void);
void CL_BaseMove(usercmd_t* cmd);
void CL_FinishMove(usercmd_t* cmd);

void CL_Download_Data(void);
qboolean CL_CheckDownloads(void);

void CL_ParseEffect(qboolean big);
void CL_UpdateBeam(struct qmodel_s* m, const char* trailname, const char* impactname, int ent, float* start, float* end);
void CL_ParseTEnt(void);
void CL_UpdateTEnts(void);

void CL_FreeState(void);
void CL_ClearState(void);
void CL_ClearTrailStates(void);

//
// cl_demo.c
//
void CL_StopPlayback(void);
int CL_GetMessage(void);

void CL_Stop_f(void);
void CL_Record_f(void);
void CL_PlayDemo_f(void);
void CL_TimeDemo_f(void);

//
// cl_parse.c
//
void CL_ParseServerMessage(void);
void CL_RegisterParticles(void);
//void CL_NewTranslation (int slot);

//
// view
//
void V_StartPitchDrift(void);
void V_StopPitchDrift(void);

void V_RenderView(void);
//void V_UpdatePalette (void); //johnfitz
void V_Register(void);
void V_ParseDamage(void);
void V_SetContentsColor(int contents);

//
// cl_tent
//
void CL_InitTEnts(void);
void CL_SignonReply(void);
float CL_TraceLine(vec3_t start, vec3_t end, vec3_t impact, vec3_t normal, int* ent);

//
// chase
//
extern	cvar_t	chase_active;

void Chase_Init(void);
void TraceLine(vec3_t start, vec3_t end, vec3_t impact);
void Chase_UpdateForClient(void);	//johnfitz
void Chase_UpdateForDrawing(void);	//johnfitz

#endif	/* _CLIENT_H_ */

