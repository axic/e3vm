# E3VM

aka. "Embedded EVM-C-compatible Ethereum VM"

The goal of this project is to provide a compact and portable EVM implementation, which can also be compiled using Emscripten to Javascript or to WebAssembly.

Depends on:
- [EVM-C](https://github.com/ethereum/evmjit)
- [trezor-crypto](https://github.com/trezor/trezor-crypto) for bignumbers and keccak256

Currently it can be used with [cpp-ethereum](https://github.com/ethereum/cpp-ethereum).

## License

MIT License

Copyright (C) 2017 Alex Beregszaszi
