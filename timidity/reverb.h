/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * REVERB EFFECT FOR TIMIDITY++-1.X (Version 0.06e  1999/1/28)
 *
 * Copyright (C) 1997,1998,1999  Masaki Kiryu <mkiryu@usa.net>
 *                           (http://w3mb.kcom.ne.jp/~mkiryu/)
 *
 * reverb.h
 *
 */
#ifndef ___REVERB_H_
#define ___REVERB_H_

#define DEFAULT_REVERB_SEND_LEVEL 40

extern int opt_reverb_control;
extern int opt_effect_quality;

extern void set_dry_signal(register int32 *, int32);
extern void mix_dry_signal(register int32 *, int32);
extern void free_effect_buffers(void);

/*                    */
/*  Effect Utitities  */
/*                    */
/*! simple delay */
typedef struct {
	int32 *buf, size, index;
} delay;

#ifndef SINE_CYCLE_LENGTH
#define SINE_CYCLE_LENGTH 1024
#endif

/*! LFO */
typedef struct {
	int32 buf[SINE_CYCLE_LENGTH];
	int32 count, cycle;	/* in samples */
	int32 icycle;	/* proportional to (SINE_CYCLE_LENGTH / cycle) */
	int type;	/* current content of its buffer */
	double freq;	/* in Hz */
} lfo;

enum {
	LFO_NONE = 0,
	LFO_SINE,
	LFO_COSINE,
	LFO_TRIANGULAR,
	LFO_WHITENOISE,
};

/*! modulated delay with allpass interpolation */
typedef struct {
	int32 *buf, size, rindex, windex, hist;
	int32 ndelay, depth;	/* in samples */
} mod_delay;

/*! modulated allpass filter with allpass interpolation */
typedef struct {
	int32 *buf, size, rindex, windex, hist;
	int32 ndelay, depth;	/* in samples */
	double feedback;
	int32 feedbacki;
} mod_allpass;

/*! Moog VCF (resonant IIR state variable filter) */
typedef struct {
	int16 freq, last_freq;	/* in Hz */
	double res_dB, last_res_dB; /* in dB */
	int32 f, q, p;	/* coefficients in fixed-point */
	int32 b0, b1, b2, b3, b4;
} filter_moog;

/*! LPF18 (resonant IIR lowpass filter with waveshaping) */
typedef struct {
	int16 freq, last_freq;	/* in Hz */
	double dist, res, last_dist, last_res; /* in linear */
	double ay1, ay2, aout, lastin, kres, value, kp, kp1h;
} filter_lpf18;

/*! 1st order lowpass filter */
typedef struct {
	double a;
	int32 ai, iai;	/* coefficients in fixed-point */
	int32 x1l, x1r;
} filter_lowpass1;

extern void init_filter_lowpass1(filter_lowpass1 *);

/*! shelving filter */
typedef struct {
	double freq, gain, q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 a1, a2, b0, b1, b2;
} filter_shelving;

extern void calc_filter_shelving_high(filter_shelving *);
extern void calc_filter_shelving_low(filter_shelving *);

/*! peaking filter */
typedef struct {
	double freq, gain, q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 ba1, a2, b0, b2;
} filter_peaking;

extern void calc_filter_peaking(filter_peaking *);

/*! allpass filter */
typedef struct _allpass {
	int32 *buf, size, index;
	double feedback;
	int32 feedbacki;
} allpass;

/*! comb filter */
typedef struct _comb {
	int32 *buf, filterstore, size, index;
	double feedback, damp1, damp2;
	int32 feedbacki, damp1i, damp2i;
} comb;

/*                                  */
/*  Insertion and Variation Effect  */
/*                                  */
extern void do_insertion_effect_gs(int32*, int32);
extern void do_insertion_effect_xg(int32*, int32);
extern void do_variation_effect_xg(int32*, int32);

enum {
	EFFECT_NONE,
	EFFECT_EQ2,
	EFFECT_OVERDRIVE1,
	EFFECT_DISTORTION1,
	EFFECT_HEXA_CHORUS,
	EFFECT_OD1OD2,
};

#define MAGIC_INIT_EFFECT_INFO -1
#define MAGIC_FREE_EFFECT_INFO -2

typedef struct _EffectList {
	int8 type;
	void *info;	/* private effect information struct */
	void (*do_effect)(int32 *, int32, struct _EffectList *);
	struct _EffectList *next_ef;
} EffectList;

extern void convert_effect(EffectList *);
extern EffectList *push_effect(EffectList *, int8, void *);
extern void do_effect_list(int32 *, int32, EffectList *);
extern void free_effect_list(EffectList *);

/*! 2-Band EQ */
typedef struct {
    int16 low_freq, high_freq;		/* in Hz */
	int16 low_gain, high_gain;		/* in dB */
	filter_shelving hsf, lsf;
} InfoEQ2;

/*! Overdrive 1 / Distortion 1 */
typedef struct {
	double level;
	int32 leveli, leveldi;	/* in fixed-point */
	int8 drive, pan;
	filter_moog svf;
	filter_lpf18 lpf18;
} InfoOverdrive1;

/*! OD1 / OD2 */
typedef struct {
	double level, levell, levelr;
	int32 levelli, levelri, leveldli, leveldri;	/* in fixed-point */
	int8 drivel, driver, panl, panr;
	filter_moog svfl, svfr;
	filter_lpf18 lpf18l, lpf18r;
	int32 typel, typer;
} InfoOD1OD2;

/*! HEXA-CHORUS */
typedef struct {
	delay buf0;
	lfo lfo0;
	double dry, wet, level;
	int32 pdelay, depth;	/* in samples */
	int8 pdelay_dev, depth_dev, pan_dev;
	int32 dryi, weti;	/* in fixed-point */
	int32 pan0, pan1, pan2, pan3, pan4, pan5;
	int32 depth0, depth1, depth2, depth3, depth4, depth5,
		pdelay0, pdelay1, pdelay2, pdelay3, pdelay4, pdelay5;
	int32 spt0, spt1, spt2, spt3, spt4, spt5,
		hist0, hist1, hist2, hist3, hist4, hist5;
} InfoHexaChorus;

/*! Plate Reverb */
typedef struct {
	delay pd, od1l, od2l, od3l, od4l, od5l, od6l, od7l,
		od1r, od2r, od3r, od4r, od5r, od6r, od7r,
		td1, td2, td1d, td2d;
	lfo lfo1, lfo1d;
	allpass ap1, ap2, ap3, ap4, ap6, ap6d;
	mod_allpass ap5, ap5d;
	filter_lowpass1 lpf1, lpf2;
	int32 t1, t1d;
	double decay, ddif1, ddif2, idif1, idif2, dry, wet;
	int32 decayi, ddif1i, ddif2i, idif1i, idif2i, dryi, weti;
} InfoPlateReverb;

/*! Freeverb */
#define numcombs 8
#define numallpasses 4

typedef struct {
	double roomsize, roomsize1, damp, damp1, wet, wet1, wet2, width;
	comb combL[numcombs], combR[numcombs];
	allpass allpassL[numallpasses], allpassR[numallpasses];
	int32 wet1i, wet2i;
	int8 alloc_flag;
} InfoFreeverb;

/*                             */
/*        System Effect        */
/*                             */
/* Reverb Effect */
extern void do_reverb(int32 *, int32);
extern void do_ch_reverb(int32 *, int32);
extern void set_ch_reverb(register int32 *, int32, int32);
extern void do_mono_reverb(int32 *, int32);
extern void init_reverb(int32);
extern void reverb_rc_event(int, int32);
extern void recompute_reverb_value(int32);

/* Chorus Effect */
extern void do_ch_chorus(int32 *, int32);
extern void set_ch_chorus(register int32 *, int32, int32);
extern void init_chorus_lfo(void);
extern void init_ch_chorus(void);

/* Delay (Celeste) Effect */
extern void do_ch_delay(int32 *, int32);
extern void set_ch_delay(register int32 *, int32, int32);
extern void init_ch_delay(void);

/* EQ */
extern void init_eq_gs(void);
extern void set_ch_eq_gs(register int32 *, int32);
extern void do_ch_eq_gs(int32 *, int32);
extern void do_multi_eq_xg(int32 *, int32);

/* GS parameters of reverb effect */
struct reverb_status_t
{
	/* GS parameters */
	uint8 character, pre_lpf, level, time, delay_feedback, pre_delay_time;

	/* for pre-calculation */
	double level_ratio, time_ratio;

	InfoPlateReverb info_plate_reverb;
	InfoFreeverb info_freeverb;
	filter_lowpass1 lpf;
} reverb_status;

/* GS parameters of chorus effect */
struct chorus_param_t
{
	/* GS parameters */
	uint8 chorus_macro, chorus_pre_lpf, chorus_level, chorus_feedback,
		chorus_delay, chorus_rate, chorus_depth, chorus_send_level_to_reverb,
		chorus_send_level_to_delay;

	/* for pre-calculation */
	double level_ratio, feedback_ratio, send_reverb_ratio, send_delay_ratio;
	int32 cycle_in_sample, depth_in_sample, delay_in_sample;

	filter_lowpass1 lpf;
} chorus_param;

/* dummy. see also readmidi.c */
struct chorus_status_t
{
    int status;
    uint8 voice_reserve[18], macro[3], pre_lpf[3], level[3], feed_back[3],
		delay[3], rate[3], depth[3], send_level[3];
} chorus_status;

/* GS parameters of delay effect */
struct delay_status_t
{
	/* GS parameters */
	uint8 type, level, level_center, level_left, level_right,
		feedback, pre_lpf, send_reverb;
    double time_center;			/* in ms */
    double time_ratio_left, time_ratio_right;		/* in pct */

	/* for pre-calculation */
	int32 sample_c, sample_l, sample_r;
	double level_ratio_c, level_ratio_l, level_ratio_r,
		feedback_ratio, send_reverb_ratio;

	filter_lowpass1 lpf;
} delay_status;

/* GS parameters of channel EQ */
struct eq_status_gs_t
{
	/* GS parameters */
    uint8 low_freq, high_freq, low_gain, high_gain;

	filter_shelving hsf, lsf;
} eq_status_gs;

/* XG parameters of Multi EQ */
struct multi_eq_xg_t
{
	/* XG parameters */
	uint8 type, gain1, gain2, gain3, gain4, gain5,
		freq1, freq2, freq3, freq4, freq5,
		q1, q2, q3, q4, q5, shape1, shape5;

	int8 valid, valid1, valid2, valid3, valid4, valid5;
	filter_shelving eq1s, eq5s;
	filter_peaking eq1p, eq2p, eq3p, eq4p, eq5p;
} multi_eq_xg;

struct insertion_effect_gs {
	int32 type;
	int8 type_lsb, type_msb, parameter[20], send_reverb,
		send_chorus, send_delay, control_source1, control_depth1,
		control_source2, control_depth2, send_eq_switch;
	struct _EffectList *ef;
} ie_gs;

#endif /* ___REVERB_H_ */
