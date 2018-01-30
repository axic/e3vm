enum instruction {
	STOP = 0x00,

	ADD = 0x01,
	MUL = 0x02,
	SUB = 0x03,

	KECCAK256 = 0x20,

	BALANCE = 0x31,

	MLOAD = 0x51,
	MSTORE = 0x52,

	PUSH1 = 0x60,
	//
	PUSH32 = 0x60 + 31,

	DUP1 = 0x80,
	//
	DUP32 = 0x80 + 31,
};

static inline unsigned instruction_is_push(enum instruction inst) {
	return inst >= PUSH1 && inst <= PUSH32;
}

static inline unsigned instruction_push_size(enum instruction inst) {
	return inst - PUSH1 + 1;
}
