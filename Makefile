CFLAGS += -DUSE_BN_PRINT=1 -DUSE_KECCAK=1 -Itrezor-crypto -Ievmjit/include

all:
#	$(MAKE) -C trezor-crypto
	$(CC) $(CFLAGS) -o e3vm.so -c e3vm.c $(LDFLAGS)

capi:
	$(CC) $(CFLAGS) -o capi e3vm.c capi.c trezor-crypto/bignum.c trezor-crypto/sha3.c