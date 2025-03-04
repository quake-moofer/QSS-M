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
// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"

extern cvar_t cl_maxpitch; //johnfitz -- variable pitch clamping
extern cvar_t cl_minpitch; //johnfitz -- variable pitch clamping

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_jump, in_attack, in_button3, in_button4, in_button5, in_button6, in_button7, in_button8;
kbutton_t	in_up, in_down;

int			in_impulse;

// JPG 1.05 - translate +jump to +moveup under water | rook / woods 8/31/2021
//extern cvar_t	pq_moveup;    //                       rook / woods 8/31/2021

void KeyDown(kbutton_t* b)
{
	int		k;
	const char* c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

// begin rook / woods 8/31/2021


		// JPG 1.05 - if jump is pressed underwater, translate it to a moveup                 
	if (b == &in_jump /*&& pq_moveup.value*/ && cl.stats[STAT_HEALTH] > 0 && cl.inwater)
		b = &in_up;

	// end rook / woods 8/31/2021


	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_Printf("Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp(kbutton_t* b)
{
	int		k;
	const char* c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	// begin rook / woods 8/31/2021

	// JPG 1.05 - check to see if we need to translate -jump to -moveup
	if (b == &in_jump/* && pq_moveup.value*/)
	{
		if (k == in_up.down[0] || k == in_up.down[1])
			b = &in_up;
		else
		{
			// in case a -moveup got lost somewhere
			in_up.down[0] = in_up.down[1] = 0;
			in_up.state = 4;
		}
	}

	// end begin rook / woods 8/31/2021


	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)
	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown(void) { KeyDown(&in_klook); }
void IN_KLookUp(void) { KeyUp(&in_klook); }
void IN_MLookDown(void) { KeyDown(&in_mlook); }
void IN_MLookUp(void) {
	KeyUp(&in_mlook);
	if (!(in_mlook.state & 1) && lookspring.value)
		V_StartPitchDrift();
}
void IN_UpDown(void) { KeyDown(&in_up); }
void IN_UpUp(void) { KeyUp(&in_up); }
void IN_DownDown(void) { KeyDown(&in_down); }
void IN_DownUp(void) { KeyUp(&in_down); }
void IN_LeftDown(void) { KeyDown(&in_left); }
void IN_LeftUp(void) { KeyUp(&in_left); }
void IN_RightDown(void) { KeyDown(&in_right); }
void IN_RightUp(void) { KeyUp(&in_right); }
void IN_ForwardDown(void) { KeyDown(&in_forward); }
void IN_ForwardUp(void) { KeyUp(&in_forward); }
void IN_BackDown(void) { KeyDown(&in_back); }
void IN_BackUp(void) { KeyUp(&in_back); }
void IN_LookupDown(void) { KeyDown(&in_lookup); }
void IN_LookupUp(void) { KeyUp(&in_lookup); }
void IN_LookdownDown(void) { KeyDown(&in_lookdown); }
void IN_LookdownUp(void) { KeyUp(&in_lookdown); }
void IN_MoveleftDown(void) { KeyDown(&in_moveleft); }
void IN_MoveleftUp(void) { KeyUp(&in_moveleft); }
void IN_MoverightDown(void) { KeyDown(&in_moveright); }
void IN_MoverightUp(void) { KeyUp(&in_moveright); }

void IN_SpeedDown(void) { KeyDown(&in_speed); }
void IN_SpeedUp(void) { KeyUp(&in_speed); }
void IN_StrafeDown(void) { KeyDown(&in_strafe); }
void IN_StrafeUp(void) { KeyUp(&in_strafe); }

void IN_AttackDown(void) { KeyDown(&in_attack); }
void IN_AttackUp(void) { KeyUp(&in_attack); }

void IN_UseDown(void) { KeyDown(&in_button3); }
void IN_UseUp(void) { KeyUp(&in_button3); }
void IN_JumpDown(void) { KeyDown(&in_jump); }
void IN_JumpUp(void) { KeyUp(&in_jump); }

void IN_Button3Down(void) { KeyDown(&in_button3); }
void IN_Button3Up(void) { KeyUp(&in_button3); }
void IN_Button4Down(void) { KeyDown(&in_button4); }
void IN_Button4Up(void) { KeyUp(&in_button4); }
void IN_Button5Down(void) { KeyDown(&in_button5); }
void IN_Button5Up(void) { KeyUp(&in_button5); }
void IN_Button6Down(void) { KeyDown(&in_button6); }
void IN_Button6Up(void) { KeyUp(&in_button6); }
void IN_Button7Down(void) { KeyDown(&in_button7); }
void IN_Button7Up(void) { KeyUp(&in_button7); }
void IN_Button8Down(void) { KeyDown(&in_button8); }
void IN_Button8Up(void) { KeyUp(&in_button8); }


// begin rook / woods


// void IN_Impulse (void) {in_impulse=Q_atoi(Cmd_Argv(1));}   | rook / woods old code


// Tonik ...
void IN_Impulse(void)
{
	int best, i, imp;

	in_impulse = Q_atoi(Cmd_Argv(1));

	if (Cmd_Argc() <= 2)
	{
		return;
	}

	best = 0;

	for (i = Cmd_Argc() - 1; i > 0; i--)
	{
		imp = Q_atoi(Cmd_Argv(i));
		if (imp < 1 || imp > 8)
			continue;
		switch (imp)
		{
		case 1:
			if (cl.items & IT_AXE)
				best = 1;
			break;
		case 2:
			if (cl.items & IT_SHOTGUN && cl.stats[STAT_SHELLS] >= 1)
				best = 2;
			break;
		case 3:
			if (cl.items & IT_SUPER_SHOTGUN && cl.stats[STAT_SHELLS] >= 2)
				best = 3;
			break;
		case 4:
			if (cl.items & IT_NAILGUN && cl.stats[STAT_NAILS] >= 1)
				best = 4;
			break;
		case 5:
			if (cl.items & IT_SUPER_NAILGUN && cl.stats[STAT_NAILS] >= 2)
				best = 5;
			break;
		case 6:
			if (cl.items & IT_GRENADE_LAUNCHER && cl.stats[STAT_ROCKETS] >= 1)
				best = 6;
			break;
		case 7:
			if (cl.items & IT_ROCKET_LAUNCHER && cl.stats[STAT_ROCKETS] >= 1)
				best = 7;
			break;
		case 8:
			if (cl.items & IT_LIGHTNING && cl.stats[STAT_CELLS] > 0)
				best = 8;
		}
	}

	if (best)
	{
		in_impulse = best;
	}
}
//... Tonik


// end rook / woods




/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState(kbutton_t* key)
{
	float		val;
	qboolean	impulsedown, impulseup, down;

	impulsedown = key->state & 2;
	impulseup = key->state & 4;
	down = key->state & 1;
	val = 0;

	if (impulsedown && !impulseup)
	{
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}
	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}
	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}
	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state &= 1;		// clear impulses

	return val;
}


//==========================================================================

cvar_t	cl_upspeed = { "cl_upspeed","200",CVAR_NONE };
cvar_t	cl_forwardspeed = { "cl_forwardspeed","200", CVAR_ARCHIVE };
cvar_t	cl_backspeed = { "cl_backspeed","200", CVAR_ARCHIVE };
cvar_t	cl_sidespeed = { "cl_sidespeed","350",CVAR_NONE };

cvar_t	cl_movespeedkey = { "cl_movespeedkey","2.0",CVAR_NONE };

cvar_t	cl_yawspeed = { "cl_yawspeed","140",CVAR_NONE };
cvar_t	cl_pitchspeed = { "cl_pitchspeed","150",CVAR_NONE };

cvar_t	cl_anglespeedkey = { "cl_anglespeedkey","1.5",CVAR_NONE };

cvar_t	cl_alwaysrun = { "cl_alwaysrun","0",CVAR_ARCHIVE }; // QuakeSpasm -- new always run

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles(void)
{
	float	speed;
	float	up, down;

	if ((in_speed.state & 1) ^ (cl_alwaysrun.value != 0.0))
		speed = host_frametime * cl_anglespeedkey.value;
	else
		speed = host_frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed * cl_yawspeed.value * CL_KeyState(&in_right);
		cl.viewangles[YAW] += speed * cl_yawspeed.value * CL_KeyState(&in_left);
		cl.viewangles[YAW] = anglemod(cl.viewangles[YAW]);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift();
		cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * CL_KeyState(&in_forward);
		cl.viewangles[PITCH] += speed * cl_pitchspeed.value * CL_KeyState(&in_back);
	}

	up = CL_KeyState(&in_lookup);
	down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * up;
	cl.viewangles[PITCH] += speed * cl_pitchspeed.value * down;

	if (up || down)
		V_StopPitchDrift();

	//johnfitz -- variable pitch clamping
	if (cl.viewangles[PITCH] > cl_maxpitch.value)
		cl.viewangles[PITCH] = cl_maxpitch.value;
	if (cl.viewangles[PITCH] < cl_minpitch.value)
		cl.viewangles[PITCH] = cl_minpitch.value;
	//johnfitz

	if (cl.viewangles[ROLL] > 50)
		cl.viewangles[ROLL] = 50;
	if (cl.viewangles[ROLL] < -50)
		cl.viewangles[ROLL] = -50;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove(usercmd_t* cmd)
{
	Q_memset(cmd, 0, sizeof(*cmd));

	VectorCopy(cl.viewangles, cmd->viewangles);

	if (cls.signon != SIGNONS)
		return;

	if (in_strafe.state & 1)
	{
		cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in_right);
		cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in_left);
	}

	cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in_moveright);
	cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in_moveleft);

	cmd->upmove += cl_upspeed.value * CL_KeyState(&in_up);
	cmd->upmove -= cl_upspeed.value * CL_KeyState(&in_down);

	if (!(in_klook.state & 1))
	{
		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState(&in_forward);
		cmd->forwardmove -= cl_backspeed.value * CL_KeyState(&in_back);
	}

	//
	// adjust for speed key
	//
	if ((in_speed.state & 1) ^ (cl_alwaysrun.value != 0.0))
	{
		cmd->forwardmove *= cl_movespeedkey.value;
		cmd->sidemove *= cl_movespeedkey.value;
		cmd->upmove *= cl_movespeedkey.value;
	}
}

void CL_FinishMove(usercmd_t* cmd)
{
	unsigned int bits;
	//
	// send button bits
	//
	bits = 0;

	if (in_attack.state & 3)
		bits |= 1;
	in_attack.state &= ~2;

	if (in_jump.state & 3)
		bits |= 2;
	in_jump.state &= ~2;

	if (in_button3.state & 3)
		bits |= 4;
	in_button3.state &= ~2;

	if (in_button4.state & 3)
		bits |= 8;
	in_button4.state &= ~2;

	if (in_button5.state & 3)
		bits |= 16;
	in_button5.state &= ~2;

	if (in_button6.state & 3)
		bits |= 32;
	in_button6.state &= ~2;

	if (in_button7.state & 3)
		bits |= 64;
	in_button7.state &= ~2;

	if (in_button8.state & 3)
		bits |= 128;
	in_button8.state &= ~2;

	cmd->buttons = bits;
	cmd->impulse = in_impulse;

	in_impulse = 0;
}

/*
==============
CL_SendMove
==============
*/
void CL_SendMove(const usercmd_t* cmd)
{
	unsigned int	i;
	sizebuf_t		buf;
	byte			data[1024];

	buf.maxsize = sizeof(data);
	buf.cursize = 0;
	buf.data = data;

	for (i = 0; i < cl.ackframes_count; i++)
	{
		MSG_WriteByte(&buf, clcdp_ackframe);
		MSG_WriteLong(&buf, cl.ackframes[i]);
	}
	cl.ackframes_count = 0;

	if (cmd)
	{
		int dump = buf.cursize;
		unsigned int bits = cmd->buttons;

		//
		// send the movement message
		//
		MSG_WriteByte(&buf, clc_move);

		if (cl.protocol == PROTOCOL_VERSION_DP7)
		{
			if (0)
			{
				MSG_WriteLong(&buf, 0);
				MSG_WriteFloat(&buf, cl.mtime[0]);	// so server can get ping times
			}
			else
			{
				MSG_WriteLong(&buf, cl.movemessages);
				MSG_WriteFloat(&buf, cmd->servertime);	// for input timing
			}
		}
		else if (cl.protocol_pext2 & PEXT2_PREDINFO)
		{
			MSG_WriteShort(&buf, cl.movemessages & 0xffff);	//server will ack this once it has been applied to the player's entity state
			MSG_WriteFloat(&buf, cmd->servertime);	// so server can get cmd timing (pings will be calculated by entframe acks).
		}
		else
			MSG_WriteFloat(&buf, cl.mtime[0]);	// so server can get ping times

		for (i = 0; i < 3; i++)
			//johnfitz -- 16-bit angles for PROTOCOL_FITZQUAKE
			//spike -- nq+bjp3 use 8bit angles. all other supported protocols use 16bit ones.
			//spike -- proquake servers bump client->server angles up to at least 16bit. this is safe because it only happens when both client+server advertise it, and because it never actually gets recorded into demos anyway.
			//spike -- predinfo also always means 16bit angles, even if for some reason the server doesn't advertise proquake (like dp).
			if ((cl.protocol == PROTOCOL_NETQUAKE || cl.protocol == PROTOCOL_VERSION_BJP3) && !NET_QSocketGetProQuakeAngleHack(cls.netcon) && !(cl.protocol_pext2 & PEXT2_PREDINFO))
				MSG_WriteAngle(&buf, cl.viewangles[i], cl.protocolflags);
			else
				MSG_WriteAngle16(&buf, cl.viewangles[i], cl.protocolflags);
		//johnfitz

		MSG_WriteShort(&buf, cmd->forwardmove);
		MSG_WriteShort(&buf, cmd->sidemove);
		MSG_WriteShort(&buf, cmd->upmove);

		if (cl.protocol_pext2 & PEXT2_PRYDONCURSOR)
		{
			if (cmd->weapon)
				bits |= (1u << 30);
			if (cmd->cursor_screen[0] || cmd->cursor_screen[1] ||
				cmd->cursor_start[0] || cmd->cursor_start[1] || cmd->cursor_start[2] ||
				cmd->cursor_impact[0] || cmd->cursor_impact[1] || cmd->cursor_impact[2] ||
				cmd->cursor_entitynumber)
				bits |= (1u << 31);
			MSG_WriteLong(&buf, bits);
		}
		else if (cl.protocol == PROTOCOL_VERSION_DP7)
		{
			MSG_WriteLong(&buf, bits);
			bits |= (1u << 31);
		}
		else
			MSG_WriteByte(&buf, bits);
		MSG_WriteByte(&buf, cmd->impulse);
		if (bits & (1u << 30))
			MSG_WriteLong(&buf, cmd->weapon);
		if (bits & (1u << 31))
		{
			MSG_WriteShort(&buf, cmd->cursor_screen[0] * 32767);
			MSG_WriteShort(&buf, cmd->cursor_screen[1] * 32767);
			MSG_WriteFloat(&buf, cmd->cursor_start[0]);	//start (view pos)
			MSG_WriteFloat(&buf, cmd->cursor_start[1]);
			MSG_WriteFloat(&buf, cmd->cursor_start[2]);
			MSG_WriteFloat(&buf, cmd->cursor_impact[0]);	//impact
			MSG_WriteFloat(&buf, cmd->cursor_impact[1]);
			MSG_WriteFloat(&buf, cmd->cursor_impact[2]);
			MSG_WriteEntity(&buf, cmd->cursor_entitynumber, cl.protocol_pext2);
		}
		in_impulse = 0;

		cl.movecmds[cl.movemessages & MOVECMDS_MASK] = *cmd;

		//
		// allways dump the first two message, because it may contain leftover inputs
		// from the last level
		//
		if (++cl.movemessages <= 2)
			buf.cursize = dump;
		else
			S_Voip_Transmit(clcfte_voicechat, &buf);/*Spike: Add voice data*/
	}
	else
		S_Voip_Transmit(clcfte_voicechat, NULL);/*Spike: Add voice data (with cl_voip_test anyway)*/

	//fixme: nops if we're still connecting, or something.

	//
	// deliver the message
	//
	if (cls.demoplayback || !buf.cursize)
		return;

	if (NET_SendUnreliableMessage(cls.netcon, &buf) == -1)
	{
		Con_Printf("CL_SendMove: lost server connection\n");
		CL_Disconnect();
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	Cmd_AddCommand("+moveup", IN_UpDown);
	Cmd_AddCommand("-moveup", IN_UpUp);
	Cmd_AddCommand("+movedown", IN_DownDown);
	Cmd_AddCommand("-movedown", IN_DownUp);
	Cmd_AddCommand("+left", IN_LeftDown);
	Cmd_AddCommand("-left", IN_LeftUp);
	Cmd_AddCommand("+right", IN_RightDown);
	Cmd_AddCommand("-right", IN_RightUp);
	Cmd_AddCommand("+forward", IN_ForwardDown);
	Cmd_AddCommand("-forward", IN_ForwardUp);
	Cmd_AddCommand("+back", IN_BackDown);
	Cmd_AddCommand("-back", IN_BackUp);
	Cmd_AddCommand("+lookup", IN_LookupDown);
	Cmd_AddCommand("-lookup", IN_LookupUp);
	Cmd_AddCommand("+lookdown", IN_LookdownDown);
	Cmd_AddCommand("-lookdown", IN_LookdownUp);
	Cmd_AddCommand("+strafe", IN_StrafeDown);
	Cmd_AddCommand("-strafe", IN_StrafeUp);
	Cmd_AddCommand("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand("+moveright", IN_MoverightDown);
	Cmd_AddCommand("-moveright", IN_MoverightUp);
	Cmd_AddCommand("+speed", IN_SpeedDown);
	Cmd_AddCommand("-speed", IN_SpeedUp);
	Cmd_AddCommand("+attack", IN_AttackDown);
	Cmd_AddCommand("-attack", IN_AttackUp);
	Cmd_AddCommand("+use", IN_UseDown);
	Cmd_AddCommand("-use", IN_UseUp);
	Cmd_AddCommand("+button3", IN_Button3Down);
	Cmd_AddCommand("-button3", IN_Button3Up);
	Cmd_AddCommand("+button4", IN_Button4Down);
	Cmd_AddCommand("-button4", IN_Button4Up);
	Cmd_AddCommand("+button5", IN_Button5Down);
	Cmd_AddCommand("-button5", IN_Button5Up);
	Cmd_AddCommand("+button6", IN_Button6Down);
	Cmd_AddCommand("-button6", IN_Button6Up);
	Cmd_AddCommand("+button7", IN_Button7Down);
	Cmd_AddCommand("-button7", IN_Button7Up);
	Cmd_AddCommand("+button8", IN_Button8Down);
	Cmd_AddCommand("-button8", IN_Button8Up);
	Cmd_AddCommand("+jump", IN_JumpDown);
	Cmd_AddCommand("-jump", IN_JumpUp);
	Cmd_AddCommand("impulse", IN_Impulse);
	Cmd_AddCommand("+klook", IN_KLookDown);
	Cmd_AddCommand("-klook", IN_KLookUp);
	Cmd_AddCommand("+mlook", IN_MLookDown);
	Cmd_AddCommand("-mlook", IN_MLookUp);

}

