# PST Howto for Freicoin

Since Freicoin 0.17, an RPC interface exists for Partially Signed Freicoin
Transactions (PSTs, as specified in
[BIP 174](https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki)).

This document describes the overall workflow for producing signed transactions
through the use of PST, and the specific RPC commands used in typical
scenarios.

## PST in general

PST is an interchange format for Freicoin transactions that are not fully signed
yet, together with relevant metadata to help entities work towards signing it.
It is intended to simplify workflows where multiple parties need to cooperate to
produce a transaction. Examples include hardware wallets, multisig setups, and
[CoinJoin](https://bitcointalk.org/?topic=279249) transactions.

### Overall workflow

Overall, the construction of a fully signed Freicoin transaction goes through the
following steps:

- A **Creator** proposes a particular transaction to be created. They construct
  a PST that contains certain inputs and outputs, but no additional metadata.
- For each input, an **Updater** adds information about the UTXOs being spent by
  the transaction to the PST. They also add information about the scripts and
  public keys involved in each of the inputs (and possibly outputs) of the PST.
- **Signers** inspect the transaction and its metadata to decide whether they
  agree with the transaction. They can use amount information from the UTXOs
  to assess the values and fees involved. If they agree, they produce a
  partial signature for the inputs for which they have relevant key(s).
- A **Finalizer** is run for each input to convert the partial signatures and
  possibly script information into a final `scriptSig` and/or `scriptWitness`.
- An **Extractor** produces a valid Freicoin transaction (in network format)
  from a PST for which all inputs are finalized.

Generally, each of the above (excluding Creator and Extractor) will simply
add more and more data to a particular PST, until all inputs are fully signed.
In a naive workflow, they all have to operate sequentially, passing the PST
from one to the next, until the Extractor can convert it to a real transaction.
In order to permit parallel operation, **Combiners** can be employed which merge
metadata from different PSTs for the same unsigned transaction.

The names above in bold are the names of the roles defined in BIP174. They're
useful in understanding the underlying steps, but in practice, software and
hardware implementations will typically implement multiple roles simultaneously.

## PST in Freicoin

### RPCs

- **`converttopst` (Creator)** is a utility RPC that converts an
  unsigned raw transaction to PST format. It ignores existing signatures.
- **`createpst` (Creator)** is a utility RPC that takes a list of inputs and
  outputs and converts them to a PST with no additional information. It is
  equivalent to calling `createrawtransaction` followed by `converttopst`.
- **`walletcreatefundedpst` (Creator, Updater)** is a wallet RPC that creates a
  PST with the specified inputs and outputs, adds additional inputs and change
  to it to balance it out, and adds relevant metadata. In particular, for inputs
  that the wallet knows about (counting towards its normal or watch-only
  balance), UTXO information will be added. For outputs and inputs with UTXO
  information present, key and script information will be added which the wallet
  knows about. It is equivalent to running `createrawtransaction`, followed by
  `fundrawtransaction`, and `converttopst`.
- **`walletprocesspst` (Updater, Signer, Finalizer)** is a wallet RPC that takes as
  input a PST, adds UTXO, key, and script data to inputs and outputs that miss
  it, and optionally signs inputs. Where possible it also finalizes the partial
  signatures.
- **`utxoupdatepst` (Updater)** is a node RPC that takes a PST and updates it
  to include information available from the UTXO set (works only for SegWit
  inputs).
- **`finalizepst` (Finalizer, Extractor)** is a utility RPC that finalizes any
  partial signatures, and if all inputs are finalized, converts the result to a
  fully signed transaction which can be broadcast with `sendrawtransaction`.
- **`combinepst` (Combiner)** is a utility RPC that implements a Combiner. It
  can be used at any point in the workflow to merge information added to
  different versions of the same PST. In particular it is useful to combine the
  output of multiple Updaters or Signers.
- **`joinpsts`** (Creator) is a utility RPC that joins multiple PSTs together,
  concatenating the inputs and outputs. This can be used to construct CoinJoin
  transactions.
- **`decodepst`** is a diagnostic utility RPC which will show all information in
  a PST in human-readable form, as well as compute its eventual fee if known.
- **`analyzepst`** is a utility RPC that examines a PST and reports the
  current status of its inputs, the next step in the workflow if known, and if
  possible, computes the fee of the resulting transaction and estimates the
  final weight and feerate.


### Workflows

#### Multisig with multiple Freicoin instances

For a quick start see [Basic M-of-N multisig example using descriptor wallets and PSTs](./descriptors.md#basic-multisig-example).
If you are using legacy wallets feel free to continue with the example provided here.

Alice, Bob, and Carol want to create a 2-of-3 multisig address. They're all using
Freicoin. We assume their wallets only contain the multisig funds. In case
they also have a personal wallet, this can be accomplished through the
multiwallet feature - possibly resulting in a need to add `-rpcwallet=name` to
the command line in case `freicoin-cli` is used.

Setup:
- All three call `getnewaddress` to create a new address; call these addresses
  *Aalice*, *Abob*, and *Acarol*.
- All three call `getaddressinfo "X"`, with *X* their respective address, and
  remember the corresponding public keys. Call these public keys *Kalice*,
  *Kbob*, and *Kcarol*.
- All three now run `addmultisigaddress 2 ["Kalice","Kbob","Kcarol"]` to teach
  their wallet about the multisig script. Call the address produced by this
  command *Amulti*. They may be required to explicitly specify the same
  addresstype option each, to avoid constructing different versions due to
  differences in configuration.
- They also run `importaddress "Amulti" "" false` to make their wallets treat
  payments to *Amulti* as contributing to the watch-only balance.
- Others can verify the produced address by running
  `createmultisig 2 ["Kalice","Kbob","Kcarol"]`, and expecting *Amulti* as
  output. Again, it may be necessary to explicitly specify the addresstype
  in order to get a result that matches. This command won't enable them to
  initiate transactions later, however.
- They can now give out *Amulti* as address others can pay to.

Later, when *V* FRC has been received on *Amulti*, and Bob and Carol want to
move the coins in their entirety to address *Asend*, with no change. Alice
does not need to be involved.
- One of them - let's assume Carol here - initiates the creation. She runs
  `walletcreatefundedpst [] {"Asend":V} 0 {"subtractFeeFromOutputs":[0], "includeWatching":true}`.
  We call the resulting PST *P*. *P* does not contain any signatures.
- Carol needs to sign the transaction herself. In order to do so, she runs
  `walletprocesspst "P"`, and gives the resulting PST *P2* to Bob.
- Bob inspects the PST using `decodepst "P2"` to determine if the transaction
  has indeed just the expected input, and an output to *Asend*, and the fee is
  reasonable. If he agrees, he calls `walletprocesspst "P2"` to sign. The
  resulting PST *P3* contains both Carol's and Bob's signature.
- Now anyone can call `finalizepst "P3"` to extract a fully signed transaction
  *T*.
- Finally anyone can broadcast the transaction using `sendrawtransaction "T"`.

In case there are more signers, it may be advantageous to let them all sign in
parallel, rather than passing the PST from one signer to the next one. In the
above example this would translate to Carol handing a copy of *P* to each signer
separately. They can then all invoke `walletprocesspst "P"`, and end up with
their individually-signed PST structures. They then all send those back to
Carol (or anyone) who can combine them using `combinepst`. The last two steps
(`finalizepst` and `sendrawtransaction`) remain unchanged.
