v12.1.3-10161 Release Notes
===========================

Freicoin version v12.1.3-10161 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v12.1.3-10161

This is a new point release, enabling soft-fork deployment of the block-final transaction protocol rule update, a prerequisite for segregated witness and forward blocks.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/Freicoin-Qt (on Mac) or freicoind/freicoin-qt (on Linux).

Downgrade warning
-----------------

### Downgrade to a version < v12

Because release v12 and later will obfuscate the chainstate on every fresh sync or reindex, the chainstate is not backwards-compatible with pre-v12 versions of Freicoin or other software.

If you want to downgrade after you have done a reindex with v12 or later, you will need to reindex when you first start Freicoin version v11 or earlier.

This does not affect wallet forward or backward compatibility.

Notable changes
---------------

### Soft-fork deployment code for block-final transactions

This release includes a soft fork deployment, using the [BIP9]() deployment mechanism, to enforce a scheme for making the final transaction of a block accessible to miners, for use by future soft-forks.

The deployment sets the 2nd bit (1 << 1) of the block version number between midnight 2 July 2019 and midnight 16 April 2020 to signal readiness for deployment.  Note that as a residual effect of the coinbase-MTP soft-fork, the high-order nyble of a block's version field must 0b0011 until 2 October 2019 (0b001 to indicate version bits, and the highest version bit set).  The "locktime" soft-fork is also being signaled for during this time, using the first (1 << 0) version bit.  Please see the v12.1.2-10135 release notes for more information regarding that soft-fork deployment.

The status of the block-final soft-fork deployment can be queried as the "blockfinal" soft-fork reported by the `getblockchaininfo` RPC call.

Should this soft-fork activate, there are additional steps that miners will be required to undertake in order for their blocks to validate.  If you maintain mining or pool software, please see the next section for instructions.

For more information about the soft-fork change, please see https://github.com/tradecraftio/tradecraft/pull/46

[BIP9]: https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki

### Block-final transactions for miner commitments

Commitments made by miners in bitcoin, such as to the segregated witness data structure, are placed in the coinbase transaction.  This is the easiest choice, as it is the one place in the block which miners have free control over the contents, and which supports non-trivial amounts of arbitrary data.  However it is not an optimal location: proofs of miner commitments require a Merkle tree proof of the coinbase transaction, which is log(N) sized (where N is the number of transactions in the block).

It has been known for some time that should the commitment be placed in the LAST transaction of a block, the size of the Merkle proof can be considerably reduced, even to the point of being a constant 32B if the number of transactions in a block is of the form (2^Z) + 1 for some integer Z.  Furthermore if the commitment is placed at the end of this block-final transaction then midstate compression can be used to reduce the amount of additional information required by a proof to around 100B total.  This minimized proof size is of considerable importance when we consider SPV proofs of witness data to mobile clients, or or block header commitments providing log-sized "proofs of proof of work" for sidechain pegs and other protocols.

Having the last transaction of a block available for miners to use is also vitally important, it turns out, for various schemes that involve protocol-managed pots of funds, such as rebateable fees or the cross-shard transfers that are a part of forward blocks.  In these proposals "anyone-can-spend" user outputs of a certain form are collected (spent) by the final transaction of the block in which they appear, where they are then carried forward from block-to-block in an unending chain until they are redeemed at which point they pass to the coinbase transaction as "fee" and are paid out in an output subject to coinbase maturity rules.

Although elegant, the trouble with using the final transaction of a block for these purposes is that every transaction in a block after the coinbase is required to have inputs, and therefore in order to create such block-final transactions the miner must have access to at least one mature output it controls in the UTXO set.  The solution is to chain the transactions together so that the outputs of the final transaction of one block are used as the inputs to the next block's final transaction. To get the process started, the coinbase of the block where activation occurs contains at least one anyone-can-spend output of this special form, and the new rules activate when that coinbase's outputs mature 100 blocks later.

A block-final transaction is required to spend ALL the outputs of the prior block's final transaction, and the outputs it creates must be spendable with an empty scriptSig.  Because transactions are required to have at least one output, this ensures that there is always an input available for the next block's final transaction.  Additionally a block-final transaction is also allowed to gather inputs from the current block, or the just-matured coinbase from 100 blocks back, but no other input sources are allowed.

### Instructions for miners to create block-final transactions

Upon activation, blocks are REQUIRED to have block-final transactions as described above, and miners MUST update their software or else they will find their blocks orphaned by the network.

On the block which the [BIP9]() status of the "blockfinal" soft-fork switches from "locked_in" to "active", the coinbase MUST contain an output that can be successfully spent with an empty scriptSig, for which `scriptPubKey = CScript([OP_TRUE])` suffices.

It is not recommended that mining software re-implement the [BIP9]() version bits soft-fork activation logic, due to the inherent risk of reimplementing consensus code.  Instead it is recommended that mining software include a zero-valued OP_TRUE output in their coinbase if all three of the following conditions are met:

  1. The block height is a multiple of 2016 (`nHeight % 2016 == 0`);

  2. The [BIP9]() status of the "blockfinal" soft-fork is reported as "active" in the output of the `getblockchaininfo` RPC; and

  3. There is no "blockfinal" section in the output of the `getblocktemplate` RPC.

This will produce the initial output that is required to get the block-final transaction chain going.

Once the initial output matures, mining software must also produce block-final transactions correctly, and the necessary information for doing so is provided in the output of the `getblocktemplate` RPC:

    { ...
      "blockfinal": {
        "prevout": [
          {
            "txid":   ...(string)
            "vout":   ...(numeric)
            "amount": ...(numeric)
          },
          ...
        ],
      },
      "locktime": ...(numeric)
      "height":   ...(numeric)
    }

The block-final transaction needs to include as inputs ALL of the UTXOs specified in the "prevout" array.  It also needs to include at least one trivially spendable output (e.g. OP_TRUE).  Finally, the block-final `nLockTime` and `lock_height` fields should be set to the same values specified in the "locktime" and "height" fields from the output of `getblocktemplate`.

Note that construction of the block-final transaction WILL require additional steps in future soft-forks, which is why it must be constructed by the miner. Pay close attention to release notes for future releases.

v12.1.3-10161 Change log
------------------------

  * `12cf4e24` [Checkpoint]
    Add recent checkpoint, block #256000.

  * `25c92385` [DNSseed]
    Add new DNS seeds run by Tradecraft developers.

  * PR #46 [FinalTx]
    Add block-final transactions, which are a sort of "2nd coinbase" generated by the miner _after_ all other transactions in a block, and a [BIP9]() soft-fork deployment mechanism to coodinate activation of this feature.

Credits
--------

Thanks to who contributed to this release, including:

- Kalle Alm
- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
