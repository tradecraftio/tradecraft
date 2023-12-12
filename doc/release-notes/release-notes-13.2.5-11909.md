v13.2.5-11909 Release Notes
===========================

Freicoin version v13.2.5-11909 is now available from:

  https://github.com/tradecraftio/tradecraft/releases/tag/v13.2.5-11909

This is a new patch release, containing a changes to the auxiliary proof-of-work (AKA "merge mining") support in the built-in stratum server for the purpose of supporting merge-mining in conjunction with an appropriately configured bitcoind instance.  Upgrading is not required.

Please report bugs using the issue tracker at GitHub:

  https://github.com/tradecraftio/tradecraft/issues

To receive security and update notifications, please subscribe to:

  https://tradecraft.groups.io/g/announce/

Notable changes
---------------

The recently released v13.2-11780 features activation parameters for the segregated witness (segwit) soft-fork, and dropped support for Microsoft Windows XP and macOS 10.7.  Please see the comprehensive notes accompanying v13.2-11780 for more information.

The recently released v13.2.4-11864 features activation parameters for the auxiliary proof-of-work (AKA "merge mining") soft-fork.  Please see the release notes accompanying v13.2.4-11864 for more information.

### Merge-mining stratum server support

Release v13.2.4-11864 introduced the auxiliary proof-of-work soft-fork, which has since activated on both testnet and mainnet.  The auxiliary proof-of-work changes allow for merge-mining, where a single proof-of-work computation for a Bitcoin block is reused for Tradecraft, or other chains.  However previously released versions were not configured to support actual merge-mining.  This release adds that capability, by introducing 4 new stratum server methods, modifying one existing method, and adding one new notification method.

First, the 3 new server methods having to do with initialization:

- "mining.aux.subscribe"
  Returns: aux_pow_path MAX_COMMIT_BRANCH_LENGTH MAX_BRANCH_LENGTH

  This method takes no parameters, and merely indicates to the server that the client supports the merge-mining extensions.  Once received, the server will stop sending "mining.notify" messages for the auxiliary proof-of-work stage.  It will only send a traditional "mining.notify" for the second-stage native proof-of-work.

  The return value is the auxiliary proof-of-work commitment path (a 256-bit value fixed for each chain), and the two consensus-critical chain parameters which limit auxiliary proof sizes: the maximum number of hashes in a commit branch proof, and the maximum number of hashes in the auxiliary (Bitcoin) transaction branch proof.

- "minign.aux.authorize" username [password]
  Returns: string

  This method takes a username which is a Tradecraft address (with a possible suffixed minimum difficulty constraint), and adds that address to the list of current miners.  Newly generated work will be customized to that address in future "mining.aux.notify" messages (see below).  A password option is present but not currently used.

  When auxiliary work notifications are sent, the work for this user will be identified by the canonical representation of the Tradecraft address, as achieved by round-trip conversions of the username field to CFreicoinAddress and back.  For convenience, the resulting string representation is returned to the callee.

- "mining.aux.deauthorize" username
  Returns: bool

  This method removes the specified username (Tradecraft address) from the list of miners.  It returns true if the address was previously registered, or false if it was not recognized.

Now the methods for notifying the client of auxiliary work, and for submitting solved auxiliary shares to the server:

- "mining.aux.notify" job_id commits nBits bias
  Returns: ignored

  The auxiliary work notification, which sends merge-mining commitments to the client, one for each authorized miner.

  The first parameter is a string which identifies the work template, and must be kept verbatim for it is used in the submission of work.

  The second parameter is a JSON object mapping addresses (corresponding to the authorized miners) to 256-bit commitment hashes.

  The third and fourth parameters are the pseudo-nBits and a bias value (between 0 and 7) representing the work target.

- "mining.aux.submit" username job_id commit_branch midstate_hash midstate_buffer midstate_length lock_time aux_branch num_txns nVersion hashPrevBlock nTime nBits nNonce
  Returns: bool

  Submits an auxiliary proof-of-work share for consideration.  Returns true if the share was received without error.

  The username is a Tradecraft address identifying the authorized miner, and the job_id is the value specified in the work notification message.  The remaining parameters are the auxiliary proof-of-work data structure.

- "mining.submit" ... nVersion extranonce1
  Returns: bool

  The regular "mining.submit" method is modified to take one extra parameter, which specifies the extranonce1 string.  Normally the extranonce1 string is determined by the connection metadata, but in a merge-mining setup the second stage proof-of-work is potentially relayed to another miner by the upstream merge-mining stratum server.  We don't know what extranonce1 value will be in use by that miner, so we allow it to be specified during work submission.

  For similar reasons, the nVersion parameter is unconstrained when submitting second-stage shares.  We don't know what version-bits configuration is negotiated by the client, so we accept any value without masking.

Support for these merge-mining extensions is optional, and should not disrupt existing use of the stratum server for dedicated / non-merged mining.

### Delay protocol-cleanup rule change

The set of protocol-cleanup rule changes is currently scheduled to activate January 16th, 2021.  It is, however, designed to be safely pushed back so long as a majority of miners agree to do so.  Out of concern that further integration tests ought to be done before activation, this release pushes back activation on mainnet until April 16th, 2021.

At the time of release, this change of the activation date has already been adopted by a majority of the hashrate.  This official release serves to protect other users as well.

v13.2.5-11909 Change log
------------------------

The following pull requests were merged into the Tradecraft code repository for this release.

- tradecraftio/tradecraft#90 Add auxiliary subscription capability, for bitcoin merge-mining

  This pull request accomplishes a couple of tasks:

  - Relicense the stratum subsystem under the terms of the Mozilla Public License 2.0, which allows it to be extracted and used in Bitcoin Core without engaging the GPL's viral clauses.  This is, in principle, something that is not to be done lightly, since those GPL clauses protect the rights of users.  However in this instance Mark Friedenbach, the sole author of the stratum subsystem, deemed that the presumed greater adoption of merged mining benefits all to a larger degree.

  - Add auxiliary work registration to the stratum mining subsystem.  This allows a single stratum connection to be used for connecting the auxiliary chain (bitcoind) to the freicoin miner, and for multiplexing multiple mining users along that connection. Auxiliary commitments are compactly served to the upstream miner, while still reporting second-stage work units as needed.

  - Allow the specification of `extranonce1` and an unconstrained `nVersion` on submission, since the merge-mined chain has little control over what extranonce and version bits are set by the upstream miner.

  - A large number of small fixes to the stratum mining subsystem to improve performance, stability, and logging.

  This PR adds sufficient capability to serve work to upstream merge-mining servers implemented in Bitcoin Core.  There will be a separate PR to add this capability to the `bitcoin-merge-mining-13` and later branches of the tradecraft repository.

Credits
-------

Thanks to everyone who directly contributed to this release:

- Mark Friedenbach

As well as everyone that helped translating on [Transifex](https://www.transifex.com/tradecraft/freicoin-1/).
