Shared Libraries
================

## freicoinconsensus

The purpose of this library is to make the verification functionality that is critical to Freicoin's consensus available to other applications, e.g. to language bindings.

### API

The interface is defined in the C header `freicoinconsensus.h` located in  `src/script/freicoinconsensus.h`.

#### Version

`freicoinconsensus_version` returns an `unsigned int` with the API version *(currently at an experimental `0`)*.

#### Script Validation

`freicoinconsensus_verify_script` returns an `int` with the status of the verification. It will be `1` if the input script correctly spends the previous output `scriptPubKey`.

##### Parameters
- `const unsigned char *scriptPubKey` - The previous output script that encumbers spending.
- `unsigned int scriptPubKeyLen` - The number of bytes for the `scriptPubKey`.
- `const unsigned char *txTo` - The transaction with the input that is spending the previous output.
- `unsigned int txToLen` - The number of bytes for the `txTo`.
- `unsigned int nIn` - The index of the input in `txTo` that spends the `scriptPubKey`.
- `unsigned int flags` - The script validation flags *(see below)*.
- `freicoinconsensus_error* err` - Will have the error/success code for the operation *(see below)*.

##### Script Flags
- `freicoinconsensus_SCRIPT_FLAGS_VERIFY_NONE`
- `freicoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH` - Evaluate P2SH ([BIP16](https://github.com/freicoin/bips/blob/master/bip-0016.mediawiki)) subscripts
- `freicoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG` - Enforce strict DER ([BIP66](https://github.com/freicoin/bips/blob/master/bip-0066.mediawiki)) compliance
- `freicoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS` - Enable WITNESS ([BIP141](https://github.com/freicoin/bips/blob/master/bip-0141.mediawiki))
- `freicoinconsensus_SCRIPT_FLAGS_VERIFY_PROTOCOL_CLEANUP` - Relax certain consensus rules

##### Errors
- `freicoinconsensus_ERR_OK` - No errors with input parameters *(see the return value of `freicoinconsensus_verify_script` for the verification status)*
- `freicoinconsensus_ERR_TX_INDEX` - An invalid index for `txTo`
- `freicoinconsensus_ERR_TX_SIZE_MISMATCH` - `txToLen` did not match with the size of `txTo`
- `freicoinconsensus_ERR_DESERIALIZE` - An error deserializing `txTo`
- `freicoinconsensus_ERR_AMOUNT_REQUIRED` - Input amount is required if WITNESS is used

### Example Implementations
- [NFreicoin](https://github.com/NicolasDorier/NFreicoin/blob/master/NFreicoin/Script.cs#L814) (.NET Bindings)
- [node-libfreicoinconsensus](https://github.com/bitpay/node-libfreicoinconsensus) (Node.js Bindings)
- [java-libfreicoinconsensus](https://github.com/dexX7/java-libfreicoinconsensus) (Java Bindings)
- [freicoinconsensus-php](https://github.com/Bit-Wasp/freicoinconsensus-php) (PHP Bindings)
