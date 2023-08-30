

#include <QtGlobal>

// Automatically generated by extract_strings_qt.py
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif
static const char UNUSED *freicoin_strings[] = {
QT_TRANSLATE_NOOP("freicoin", "The %s developers"),
QT_TRANSLATE_NOOP("freicoin", ""
"%s corrupt. Try using the wallet tool freicoin-wallet to salvage or "
"restoring a backup."),
QT_TRANSLATE_NOOP("freicoin", ""
"%s failed to validate the -assumeutxo snapshot state. This indicates a "
"hardware problem, or a bug in the software, or a bad software modification "
"that allowed an invalid snapshot to be loaded. As a result of this, the node "
"will shut down and stop using any state that was built on the snapshot, "
"resetting the chain height from %d to %d. On the next restart, the node will "
"resume syncing from %d without using any snapshot data. Please report this "
"incident to %s, including how you obtained the snapshot. The invalid "
"snapshot chainstate will be left on disk in case it is helpful in diagnosing "
"the issue that caused this error."),
QT_TRANSLATE_NOOP("freicoin", ""
"%s is set very high! Fees this large could be paid on a single transaction."),
QT_TRANSLATE_NOOP("freicoin", ""
"%s request to listen on port %u. This port is considered \"bad\" and thus it "
"is unlikely that any peer will connect to it. See doc/p2p-bad-ports.md for "
"details and a full list."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot downgrade wallet from version %i to version %i. Wallet version "
"unchanged."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot obtain a lock on data directory %s. %s is probably already running."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot provide specific connections and have addrman find outgoing "
"connections at the same time."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot upgrade a non HD split wallet from version %i to version %i without "
"upgrading to support pre-split keypool. Please use version %i or no version "
"specified."),
QT_TRANSLATE_NOOP("freicoin", ""
"Disk space for %s may not accommodate the block files. Approximately %u GB "
"of data will be stored in this directory."),
QT_TRANSLATE_NOOP("freicoin", ""
"Distributed under the GNU Affero General Purpose License v3.0, see the "
"accompanying file %s or %s"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error loading %s: External signer wallet being loaded without external "
"signer support compiled"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error loading wallet. Wallet requires blocks to be downloaded, and software "
"does not currently support loading wallets while blocks are being downloaded "
"out of order when using assumeutxo snapshots. Wallet should be able to load "
"successfully after node sync reaches height %s"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error reading %s! All keys read correctly, but transaction data or address "
"metadata may be missing or incorrect."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error reading %s! Transaction data may be missing or incorrect. Rescanning "
"wallet."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Address book data in wallet cannot be identified to belong to "
"migrated wallets"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Dumpfile format record is incorrect. Got \"%s\", expected \"format\"."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Dumpfile identifier record is incorrect. Got \"%s\", expected \"%s\"."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Dumpfile version is not supported. This version of freicoin-wallet "
"only supports version 1 dumpfiles. Got dumpfile with version %s"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Duplicate descriptors created during migration. Your wallet may be "
"corrupted."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Legacy wallets only support the \"legacy\", \"p2sh-segwit\", and "
"\"bech32\" address types"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Transaction %s in wallet cannot be identified to belong to migrated "
"wallets"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Unable to produce descriptors for this legacy wallet. Make sure to "
"provide the wallet's passphrase if it is encrypted."),
QT_TRANSLATE_NOOP("freicoin", ""
"Failed to calculate bump fees, because unconfirmed UTXOs depend on enormous "
"cluster of unconfirmed transactions."),
QT_TRANSLATE_NOOP("freicoin", ""
"Failed to rename invalid peers.dat file. Please move or delete it and try "
"again."),
QT_TRANSLATE_NOOP("freicoin", ""
"Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable "
"%s."),
QT_TRANSLATE_NOOP("freicoin", ""
"File %s already exists. If you are sure this is what you want, move it out "
"of the way first."),
QT_TRANSLATE_NOOP("freicoin", ""
"Incompatible options: -dnsseed=1 was explicitly specified, but -onlynet "
"forbids connections to IPv4/IPv6"),
QT_TRANSLATE_NOOP("freicoin", ""
"Invalid amount for %s=<amount>: '%s' (must be at least the minrelay fee of "
"%s to prevent stuck transactions)"),
QT_TRANSLATE_NOOP("freicoin", ""
"Invalid or corrupt peers.dat (%s). If you believe this is a bug, please "
"report it to %s. As a workaround, you can move the file (%s) out of the way "
"(rename, move, or delete) to have a new one created on the next start."),
QT_TRANSLATE_NOOP("freicoin", ""
"More than one onion bind address is provided. Using %s for the automatically "
"created Tor onion service."),
QT_TRANSLATE_NOOP("freicoin", ""
"No dump file provided. To use createfromdump, -dumpfile=<filename> must be "
"provided."),
QT_TRANSLATE_NOOP("freicoin", ""
"No dump file provided. To use dump, -dumpfile=<filename> must be provided."),
QT_TRANSLATE_NOOP("freicoin", ""
"No wallet file format provided. To use createfromdump, -format=<format> must "
"be provided."),
QT_TRANSLATE_NOOP("freicoin", ""
"Outbound connections restricted to CJDNS (-onlynet=cjdns) but -"
"cjdnsreachable is not provided"),
QT_TRANSLATE_NOOP("freicoin", ""
"Outbound connections restricted to Tor (-onlynet=onion) but the proxy for "
"reaching the Tor network is explicitly forbidden: -onion=0"),
QT_TRANSLATE_NOOP("freicoin", ""
"Outbound connections restricted to Tor (-onlynet=onion) but the proxy for "
"reaching the Tor network is not provided: none of -proxy, -onion or -"
"listenonion is given"),
QT_TRANSLATE_NOOP("freicoin", ""
"Outbound connections restricted to i2p (-onlynet=i2p) but -i2psam is not "
"provided"),
QT_TRANSLATE_NOOP("freicoin", ""
"Please check that your computer's date and time are correct! If your clock "
"is wrong, %s will not work properly."),
QT_TRANSLATE_NOOP("freicoin", ""
"Please contribute if you find %s useful. Visit %s for further information "
"about the software."),
QT_TRANSLATE_NOOP("freicoin", ""
"Prune configured below the minimum of %d MiB.  Please use a higher number."),
QT_TRANSLATE_NOOP("freicoin", ""
"Prune mode is incompatible with -reindex-chainstate. Use full -reindex "
"instead."),
QT_TRANSLATE_NOOP("freicoin", ""
"Prune: last wallet synchronisation goes beyond pruned data. You need to -"
"reindex (download the whole blockchain again in case of pruned node)"),
QT_TRANSLATE_NOOP("freicoin", ""
"Rename of '%s' -> '%s' failed. You should resolve this by manually moving or "
"deleting the invalid snapshot directory %s, otherwise you will encounter the "
"same error again on the next startup."),
QT_TRANSLATE_NOOP("freicoin", ""
"SQLiteDatabase: Unknown sqlite wallet schema version %d. Only version %d is "
"supported"),
QT_TRANSLATE_NOOP("freicoin", ""
"The block database contains a block which appears to be from the future. "
"This may be due to your computer's date and time being set incorrectly. Only "
"rebuild the block database if you are sure that your computer's date and "
"time are correct"),
QT_TRANSLATE_NOOP("freicoin", ""
"The inputs size exceeds the maximum weight. Please try sending a smaller "
"amount or manually consolidating your wallet's UTXOs"),
QT_TRANSLATE_NOOP("freicoin", ""
"The preselected coins total amount does not cover the transaction target. "
"Please allow other inputs to be automatically selected or include more coins "
"manually"),
QT_TRANSLATE_NOOP("freicoin", ""
"The transaction amount is too small to send after the fee has been deducted"),
QT_TRANSLATE_NOOP("freicoin", ""
"This error could occur if this wallet was not shutdown cleanly and was last "
"loaded using a build with a newer version of Berkeley DB. If so, please use "
"the software that last loaded this wallet"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is a pre-release test build - use at your own risk - do not use for "
"mining or merchant applications"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is the maximum transaction fee you pay (in addition to the normal fee) "
"to prioritize partial spend avoidance over regular coin selection."),
QT_TRANSLATE_NOOP("freicoin", ""
"This is the transaction fee you may discard if change is smaller than dust "
"at this level"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is the transaction fee you may pay when fee estimates are not available."),
QT_TRANSLATE_NOOP("freicoin", ""
"Total length of network version string (%i) exceeds maximum length (%i). "
"Reduce the number or size of uacomments."),
QT_TRANSLATE_NOOP("freicoin", ""
"Transaction requires one destination of non-0 value, a non-0 feerate, or a "
"pre-selected input"),
QT_TRANSLATE_NOOP("freicoin", ""
"UTXO snapshot failed to validate. Restart to resume normal initial block "
"download, or try loading a different snapshot."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unable to replay blocks. You will need to rebuild the database using -"
"reindex-chainstate."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unconfirmed UTXOs are available, but spending them creates a chain of "
"transactions that will be rejected by the mempool"),
QT_TRANSLATE_NOOP("freicoin", ""
"Unexpected legacy entry in descriptor wallet found. Loading wallet %s\n"
"\n"
"The wallet might have been tampered with or created with malicious intent.\n"),
QT_TRANSLATE_NOOP("freicoin", ""
"Unknown wallet file format \"%s\" provided. Please provide one of \"bdb\" or "
"\"sqlite\"."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unrecognized descriptor found. Loading wallet %s\n"
"\n"
"The wallet might had been created on a newer version.\n"
"Please try running the latest software version.\n"),
QT_TRANSLATE_NOOP("freicoin", ""
"Unsupported category-specific logging level %1$s=%2$s. Expected "
"%1$s=<category>:<loglevel>. Valid categories: %3$s. Valid loglevels: %4$s."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unsupported chainstate database format found. Please restart with -reindex-"
"chainstate. This will rebuild the chainstate database."),
QT_TRANSLATE_NOOP("freicoin", ""
"Wallet created successfully. The legacy wallet type is being deprecated and "
"support for creating and opening legacy wallets will be removed in the "
"future."),
QT_TRANSLATE_NOOP("freicoin", ""
"Wallet loaded successfully. The legacy wallet type is being deprecated and "
"support for creating and opening legacy wallets will be removed in the "
"future. Legacy wallets can be migrated to a descriptor wallet with "
"migratewallet."),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: Dumpfile wallet format \"%s\" does not match command line specified "
"format \"%s\"."),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: Private keys detected in wallet {%s} with disabled private keys"),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: We do not appear to fully agree with our peers! You may need to "
"upgrade, or other nodes may need to upgrade."),
QT_TRANSLATE_NOOP("freicoin", ""
"Witness data for blocks after height %d requires validation. Please restart "
"with -reindex."),
QT_TRANSLATE_NOOP("freicoin", ""
"You need to rebuild the database using -reindex to go back to unpruned "
"mode.  This will redownload the entire blockchain"),
QT_TRANSLATE_NOOP("freicoin", ""
"\n"
"Unable to cleanup failed migration"),
QT_TRANSLATE_NOOP("freicoin", ""
"\n"
"Unable to restore backup of wallet."),
QT_TRANSLATE_NOOP("freicoin", "%s is set very high!"),
QT_TRANSLATE_NOOP("freicoin", "-maxmempool must be at least %d MB"),
QT_TRANSLATE_NOOP("freicoin", "A fatal internal error occurred, see debug.log for details"),
QT_TRANSLATE_NOOP("freicoin", "Block verification was interrupted"),
QT_TRANSLATE_NOOP("freicoin", "Cannot resolve -%s address: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Cannot set -forcednsseed to true when setting -dnsseed to false."),
QT_TRANSLATE_NOOP("freicoin", "Cannot set -peerblockfilters without -blockfilterindex."),
QT_TRANSLATE_NOOP("freicoin", "Cannot write to data directory '%s'; check permissions."),
QT_TRANSLATE_NOOP("freicoin", "Config setting for %s only applied on %s network when in [%s] section."),
QT_TRANSLATE_NOOP("freicoin", "Copyright (C) %i-%i"),
QT_TRANSLATE_NOOP("freicoin", "Corrupted block database detected"),
QT_TRANSLATE_NOOP("freicoin", "Could not find asmap file %s"),
QT_TRANSLATE_NOOP("freicoin", "Could not parse asmap file %s"),
QT_TRANSLATE_NOOP("freicoin", "Database corruption likely.  Try restarting with `-reindex=1`."),
QT_TRANSLATE_NOOP("freicoin", "Direct mining to an internal wallet is disabled."),
QT_TRANSLATE_NOOP("freicoin", "Disk space is too low!"),
QT_TRANSLATE_NOOP("freicoin", "Do you want to rebuild the block database now?"),
QT_TRANSLATE_NOOP("freicoin", "Done loading"),
QT_TRANSLATE_NOOP("freicoin", "Dump file %s does not exist."),
QT_TRANSLATE_NOOP("freicoin", "Error committing db txn for wallet transactions removal"),
QT_TRANSLATE_NOOP("freicoin", "Error creating %s"),
QT_TRANSLATE_NOOP("freicoin", "Error initializing block database"),
QT_TRANSLATE_NOOP("freicoin", "Error initializing wallet database environment %s!"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Private keys can only be disabled during creation"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Wallet corrupted"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Wallet requires newer version of %s"),
QT_TRANSLATE_NOOP("freicoin", "Error loading block database"),
QT_TRANSLATE_NOOP("freicoin", "Error opening block database"),
QT_TRANSLATE_NOOP("freicoin", "Error reading configuration file: %s"),
QT_TRANSLATE_NOOP("freicoin", "Error reading from database, shutting down."),
QT_TRANSLATE_NOOP("freicoin", "Error reading next record from wallet database"),
QT_TRANSLATE_NOOP("freicoin", "Error starting db txn for wallet transactions removal"),
QT_TRANSLATE_NOOP("freicoin", "Error: Cannot extract destination from the generated scriptpubkey"),
QT_TRANSLATE_NOOP("freicoin", "Error: Could not add watchonly tx %s to watchonly wallet"),
QT_TRANSLATE_NOOP("freicoin", "Error: Could not delete watchonly transactions. "),
QT_TRANSLATE_NOOP("freicoin", "Error: Couldn't create cursor into database"),
QT_TRANSLATE_NOOP("freicoin", "Error: Disk space is low for %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: Dumpfile checksum does not match. Computed %s, expected %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: Failed to create new watchonly wallet"),
QT_TRANSLATE_NOOP("freicoin", "Error: Got key that was not hex: %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: Got value that was not hex: %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: Keypool ran out, please call keypoolrefill first"),
QT_TRANSLATE_NOOP("freicoin", "Error: Missing checksum"),
QT_TRANSLATE_NOOP("freicoin", "Error: No %s addresses available."),
QT_TRANSLATE_NOOP("freicoin", "Error: This wallet already uses SQLite"),
QT_TRANSLATE_NOOP("freicoin", "Error: This wallet is already a descriptor wallet"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to begin reading all records in the database"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to make a backup of your wallet"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to parse version %u as a uint32_t"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to read all records in the database"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to read wallet's best block locator record"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to remove watchonly address book data"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to write record to new wallet"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to write solvable wallet best block locator record"),
QT_TRANSLATE_NOOP("freicoin", "Error: Unable to write watchonly wallet best block locator record"),
QT_TRANSLATE_NOOP("freicoin", "Error: address book copy failed for wallet %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: database transaction cannot be executed for wallet %s"),
QT_TRANSLATE_NOOP("freicoin", "Failed to listen on any port. Use -listen=0 if you want this."),
QT_TRANSLATE_NOOP("freicoin", "Failed to rescan the wallet during initialization"),
QT_TRANSLATE_NOOP("freicoin", "Failed to start indexes, shutting down.."),
QT_TRANSLATE_NOOP("freicoin", "Failed to verify database"),
QT_TRANSLATE_NOOP("freicoin", "Failure removing transaction: %s"),
QT_TRANSLATE_NOOP("freicoin", "Fee rate (%s) is lower than the minimum fee rate setting (%s)"),
QT_TRANSLATE_NOOP("freicoin", "Ignoring duplicate -wallet %s."),
QT_TRANSLATE_NOOP("freicoin", "Importing…"),
QT_TRANSLATE_NOOP("freicoin", "Incorrect or no genesis block found. Wrong datadir for network?"),
QT_TRANSLATE_NOOP("freicoin", "Initialization sanity check failed. %s is shutting down."),
QT_TRANSLATE_NOOP("freicoin", "Input not found or already spent"),
QT_TRANSLATE_NOOP("freicoin", "Insufficient dbcache for block verification"),
QT_TRANSLATE_NOOP("freicoin", "Insufficient funds"),
QT_TRANSLATE_NOOP("freicoin", "Invalid -i2psam address or hostname: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid -onion address or hostname: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid -proxy address or hostname: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid P2P permission: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for %s=<amount>: '%s' (must be at least %s)"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for %s=<amount>: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for -%s=<amount>: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid netmask specified in -whitelist: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid port specified in %s: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid pre-selected input %s"),
QT_TRANSLATE_NOOP("freicoin", "Listening for incoming connections failed (listen returned error %s)"),
QT_TRANSLATE_NOOP("freicoin", "Loading P2P addresses…"),
QT_TRANSLATE_NOOP("freicoin", "Loading banlist…"),
QT_TRANSLATE_NOOP("freicoin", "Loading block index…"),
QT_TRANSLATE_NOOP("freicoin", "Loading wallet…"),
QT_TRANSLATE_NOOP("freicoin", "Missing amount"),
QT_TRANSLATE_NOOP("freicoin", "Missing solving data for estimating transaction size"),
QT_TRANSLATE_NOOP("freicoin", "Need to specify a port with -whitebind: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "No addresses available"),
QT_TRANSLATE_NOOP("freicoin", "No wallets available for mining."),
QT_TRANSLATE_NOOP("freicoin", "Not enough file descriptors available."),
QT_TRANSLATE_NOOP("freicoin", "Not found pre-selected input %s"),
QT_TRANSLATE_NOOP("freicoin", "Not solvable pre-selected input %s"),
QT_TRANSLATE_NOOP("freicoin", "Prune cannot be configured with a negative value."),
QT_TRANSLATE_NOOP("freicoin", "Prune mode is incompatible with -txindex."),
QT_TRANSLATE_NOOP("freicoin", "Pruning blockstore…"),
QT_TRANSLATE_NOOP("freicoin", "Reducing -maxconnections from %d to %d, because of system limitations."),
QT_TRANSLATE_NOOP("freicoin", "Replaying blocks…"),
QT_TRANSLATE_NOOP("freicoin", "Rescanning…"),
QT_TRANSLATE_NOOP("freicoin", "SQLiteDatabase: Failed to execute statement to verify database: %s"),
QT_TRANSLATE_NOOP("freicoin", "SQLiteDatabase: Failed to prepare statement to verify database: %s"),
QT_TRANSLATE_NOOP("freicoin", "SQLiteDatabase: Failed to read database verification error: %s"),
QT_TRANSLATE_NOOP("freicoin", "SQLiteDatabase: Unexpected application id. Expected %u, got %u"),
QT_TRANSLATE_NOOP("freicoin", "Section [%s] is not recognized."),
QT_TRANSLATE_NOOP("freicoin", "Settings file could not be read"),
QT_TRANSLATE_NOOP("freicoin", "Settings file could not be written"),
QT_TRANSLATE_NOOP("freicoin", "Signing transaction failed"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" does not exist"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" is a relative path"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" is not a directory"),
QT_TRANSLATE_NOOP("freicoin", "Specified blocks directory \"%s\" does not exist."),
QT_TRANSLATE_NOOP("freicoin", "Specified data directory \"%s\" does not exist."),
QT_TRANSLATE_NOOP("freicoin", "Starting network threads…"),
QT_TRANSLATE_NOOP("freicoin", "The source code is available from %s."),
QT_TRANSLATE_NOOP("freicoin", "The specified config file %s does not exist"),
QT_TRANSLATE_NOOP("freicoin", "The transaction amount is too small to pay the fee"),
QT_TRANSLATE_NOOP("freicoin", "The wallet subsystem is not configured."),
QT_TRANSLATE_NOOP("freicoin", "The wallet subsystem is not enabled."),
QT_TRANSLATE_NOOP("freicoin", "The wallet will avoid paying less than the minimum relay fee."),
QT_TRANSLATE_NOOP("freicoin", "This is experimental software."),
QT_TRANSLATE_NOOP("freicoin", "This is the minimum transaction fee you pay on every transaction."),
QT_TRANSLATE_NOOP("freicoin", "This is the transaction fee you will pay if you send a transaction."),
QT_TRANSLATE_NOOP("freicoin", "Transaction %s does not belong to this wallet"),
QT_TRANSLATE_NOOP("freicoin", "Transaction amount too small"),
QT_TRANSLATE_NOOP("freicoin", "Transaction amounts must not be negative"),
QT_TRANSLATE_NOOP("freicoin", "Transaction change output index out of range"),
QT_TRANSLATE_NOOP("freicoin", "Transaction must have at least one recipient"),
QT_TRANSLATE_NOOP("freicoin", "Transaction needs a change address, but we can't generate it."),
QT_TRANSLATE_NOOP("freicoin", "Transaction too large"),
QT_TRANSLATE_NOOP("freicoin", "Unable to allocate memory for -maxsigcachesize: '%s' MiB"),
QT_TRANSLATE_NOOP("freicoin", "Unable to bind to %s on this computer (bind returned error %s)"),
QT_TRANSLATE_NOOP("freicoin", "Unable to bind to %s on this computer. %s is probably already running."),
QT_TRANSLATE_NOOP("freicoin", "Unable to create the PID file '%s': %s"),
QT_TRANSLATE_NOOP("freicoin", "Unable to find UTXO for external input"),
QT_TRANSLATE_NOOP("freicoin", "Unable to generate initial keys"),
QT_TRANSLATE_NOOP("freicoin", "Unable to generate keys"),
QT_TRANSLATE_NOOP("freicoin", "Unable to open %s for writing"),
QT_TRANSLATE_NOOP("freicoin", "Unable to parse -maxuploadtarget: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unable to start HTTP server. See debug log for details."),
QT_TRANSLATE_NOOP("freicoin", "Unable to unload the wallet before migrating"),
QT_TRANSLATE_NOOP("freicoin", "Unknown -blockfilterindex value %s."),
QT_TRANSLATE_NOOP("freicoin", "Unknown address type '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unknown change type '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unknown network specified in -onlynet: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unknown new rules activated (versionbit %i)"),
QT_TRANSLATE_NOOP("freicoin", "Unsupported global logging level %s=%s. Valid values: %s."),
QT_TRANSLATE_NOOP("freicoin", "Unsupported logging category %s=%s."),
QT_TRANSLATE_NOOP("freicoin", "User Agent comment (%s) contains unsafe characters."),
QT_TRANSLATE_NOOP("freicoin", "Verifying blocks…"),
QT_TRANSLATE_NOOP("freicoin", "Verifying wallet(s)…"),
QT_TRANSLATE_NOOP("freicoin", "Wallet file creation failed: %s"),
QT_TRANSLATE_NOOP("freicoin", "Wallet needed to be rewritten: restart %s to complete"),
QT_TRANSLATE_NOOP("freicoin", "acceptstalefeeestimates is not supported on %s chain."),
};
