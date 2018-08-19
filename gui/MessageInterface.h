#ifndef MESSAGEINTERFACE_H
#define MESSAGEINTERFACE_H

struct control_message_t {
	enum opcode_e { op_set_mute, op_set_volume, op_set_eq, op_set_delay } opcode;
	int outputInstance;
	union {
		struct {
			bool mute;
		} set_mute;
		struct {
			double volume;
		} set_volume;
		struct {
			int index;
			int type;
			double f0;
			double Q;
			double gain;
		} set_eq;
		struct {
			double delay;
		} set_delay;
	} data;
};

struct notification_message_t {
	enum opcode_e { op_state, op_level, op_control } opcode;
	int outputInstance;
	union {
		struct {
			int numOutputInstances;
			int numChannels;
			int numEq;
		} state;
		struct {
			double level[16];
		} level;
		struct control_message_t control;
	} data;
};

#endif  // MESSAGEINTERFACE_H
