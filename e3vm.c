#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "evm.h"

#include "bignum.h"
#include "sha3.h"

#include "instructions.h"

// A runtime instance. Can handle multiple calls.
struct e3vm_instance
{
	struct evm_instance instance;
	const struct evm_host* host;
};

// A single call instance.
struct e3vm_call
{
	
};

static void evm_release_result(struct evm_result const* result)
{
	(void)result;
}

static void free_result_output_data(struct evm_result const* result)
{
	free((uint8_t*)result->output_data);
}

static inline int bn_read_flexible(const uint8_t* in, size_t inlen, bignum256* out)
{
	if (inlen > 32)
		return -1;

	uint8_t tmp[32] = { 0 };
	memcpy(tmp + 32 - inlen, in, inlen);
	bn_read_be(tmp, out);
	return 0;
}

static inline void evm160_to_bn(const struct evm_uint160be* in, bignum256* out)
{
	uint8_t tmp[32] = { 0 };
	memcpy(tmp + 12, in->bytes, 20);
	bn_read_be(in->bytes, out);
}

static inline void bn_to_evm160(const bignum256* in, struct evm_uint160be* out)
{
	uint8_t tmp[32] = { 0 };
	bn_write_be(in, tmp);
	memcpy(out->bytes, tmp + 12, 20);
}

static inline void evm256_to_bn(struct evm_uint256be* in, bignum256* out)
{
	bn_read_be(in->bytes, out);
}

static char* dump_evm160(const struct evm_uint160be* in)
{
	bignum256 tmp;
	evm160_to_bn(in, &tmp);
	char* out = calloc(1, 128);
	bn_format(&tmp, NULL, NULL, 0, 0, 0, out, 128);
	return out;
}

static struct evm_result abort_failure(enum evm_result_code code)
{
	struct evm_result ret = {
		.code = code,
		.gas_left = 0,
		.output_data = NULL,
		.output_size = 0,
		.release = NULL,
	};
	return ret;
}

static void execute(struct e3vm_call* instance)
{
}

static struct evm_result evm_execute(
	struct evm_instance* instance,
	struct evm_context* context,
	enum evm_revision rev,
	const struct evm_message* msg,
	const uint8_t* code,
	size_t code_size)
{
	printf("{executing instance=%p context=%p rev=%d msg=%p code=%p,%lu}\n",
		instance, context, rev, msg, code, (unsigned long)code_size);
	printf("{message address=%s sender=%s}\n", dump_evm160(&msg->address), dump_evm160(&msg->sender));

	struct e3vm_instance* vm = (struct e3vm_instance*)instance;
	struct evm_result ret = { 0, };

	// EVM memory
	size_t memory_words = 0;
	uint8_t* memory = calloc(1, 2*1024*1024);
	if(!memory) abort();

	// EVM stack
	bignum256 stack[1025] = { 0 };
	size_t sp = 0;

	// EVM code
	size_t pc = 0;

	// EVM execution status
	int64_t gas = msg->gas;
	unsigned stopped = 0;

	while (!stopped && pc < code_size) {
		enum instruction opcode = code[pc++];

		printf("[%lu] opcode=0x%x\n", (unsigned long)pc, opcode);
		printf("stack[%lu]:{\n", (unsigned long)sp);
		for (int i = 0; i < sp; i++)
		{
			printf("  ");
			bn_print(&stack[i]);
			printf("\n");
		}
		printf("}\n");

		switch (opcode) {
		case STOP:
			stopped = 1;
			break;

		case ADD: {
			if (sp < 2)
				return abort_failure(EVM_STACK_UNDERFLOW);
			if (sp > 1024)
				return abort_failure(EVM_STACK_OVERFLOW);

			bn_add(&stack[sp - 2], &stack[sp - 1]);
			sp--;
			break;
		}

		case KECCAK256: {
			if (sp < 2)
				return abort_failure(EVM_STACK_UNDERFLOW);
			if (sp > 1024)
				return abort_failure(EVM_STACK_OVERFLOW);

			uint64_t pos = bn_write_uint64(&stack[sp - 2]);
			uint64_t len = bn_write_uint64(&stack[sp - 1]);
			printf("keccak256 {pos=%llu len=%llu}\n", pos, len);

			uint8_t tmp[32] = { 0 };
			keccak_256(memory + pos, len, tmp);
			bn_read_be(tmp, &stack[sp - 2]);

			sp--;
			break;
		}

		case BALANCE: {
			if (sp < 1)
				return abort_failure(EVM_STACK_UNDERFLOW);

			struct evm_uint160be tmp;
			bn_to_evm160(&stack[sp - 1], &tmp);

			struct evm_uint256be ret;
			vm->host->get_balance(&ret, context, &tmp);
			evm256_to_bn(&ret, &stack[sp - 1]);
			break;
		}

		case MSTORE: {
			if (sp < 2)
				return abort_failure(EVM_STACK_UNDERFLOW);

			uint64_t pos = bn_write_uint64(&stack[sp - 2]);
			printf("mstore {pos=%llu}\n", pos);
			bn_write_be(&stack[sp - 1], memory + pos);
			sp -= 2;
			break;
		}

		case PUSH1 ... PUSH32: {
			unsigned push_size = opcode - PUSH1 + 1;
//			unsigned push_size = instruction_push_size(opcode);

			if (pc + push_size > code_size)
				return abort_failure(EVM_BAD_INSTRUCTION);

			if (bn_read_flexible(code + pc, push_size, &stack[sp]) < 0)
				return abort_failure(EVM_INTERNAL_ERROR);

			bn_print(&stack[sp]);
			printf("\n");
			sp++;
			pc += push_size;
			break;
		}

		case DUP1 ... DUP32: {
			unsigned pos = opcode - DUP1 + 1;
			if (sp < pos)
				return abort_failure(EVM_STACK_UNDERFLOW);
			bn_copy(&stack[sp - pos], &stack[sp]);
			sp++;
			break;
		}

		default:
			return abort_failure(EVM_BAD_INSTRUCTION);
		}
	}

	if (pc == code_size)
		stopped = 1;

	printf("returning %p\n", &ret);
	if (stopped) {
		ret.code = EVM_SUCCESS;
		return ret;
	}

	ret.code = EVM_FAILURE;
	return ret;
}

static void evm_destroy(struct evm_instance* evm)
{
	free(evm);
}

static struct evm_instance* evm_create(const struct evm_host* host)
{
	struct e3vm_instance* vm = calloc(1, sizeof(struct e3vm_instance));
	if (!vm)
		return NULL;
	struct evm_instance* interface = &vm->instance;
	interface->destroy = evm_destroy;
	interface->execute = evm_execute;
	vm->host = host;
	return interface;
}

struct evm_factory examplevm_get_factory()
{
	struct evm_factory factory = { EVM_ABI_VERSION, evm_create };
	return factory;
}
