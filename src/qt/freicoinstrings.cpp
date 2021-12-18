

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
"-maxtxfee is set very high! Fees this large could be paid on a single "
"transaction."),
QT_TRANSLATE_NOOP("freicoin", ""
"Can't generate a change-address key. No keys in the internal keypool and "
"can't generate any keys."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot obtain a lock on data directory %s. %s is probably already running."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot provide specific connections and have addrman find outgoing "
"connections at the same."),
QT_TRANSLATE_NOOP("freicoin", ""
"Cannot upgrade a non HD split wallet without upgrading to support pre split "
"keypool. Please use -upgradewallet=169900 or -upgradewallet with no version "
"specified."),
QT_TRANSLATE_NOOP("freicoin", ""
"Distributed under the GNU Affero General Purpose License v3.0, see the "
"accompanying file %s or %s"),
QT_TRANSLATE_NOOP("freicoin", ""
"Error reading %s! All keys read correctly, but transaction data or address "
"book entries might be missing or incorrect."),
QT_TRANSLATE_NOOP("freicoin", ""
"Error: Listening for incoming connections failed (listen returned error %s)"),
QT_TRANSLATE_NOOP("freicoin", ""
"Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -"
"fallbackfee."),
QT_TRANSLATE_NOOP("freicoin", ""
"Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay "
"fee of %s to prevent stuck transactions)"),
QT_TRANSLATE_NOOP("freicoin", ""
"Please check that your computer's date and time are correct! If your clock "
"is wrong, %s will not work properly."),
QT_TRANSLATE_NOOP("freicoin", ""
"Please contribute if you find %s useful. Visit %s for further information "
"about the software."),
QT_TRANSLATE_NOOP("freicoin", ""
"Prune configured below the minimum of %d MiB.  Please use a higher number."),
QT_TRANSLATE_NOOP("freicoin", ""
"Prune: last wallet synchronisation goes beyond pruned data. You need to -"
"reindex (download the whole blockchain again in case of pruned node)"),
QT_TRANSLATE_NOOP("freicoin", ""
"The block database contains a block which appears to be from the future. "
"This may be due to your computer's date and time being set incorrectly. Only "
"rebuild the block database if you are sure that your computer's date and "
"time are correct"),
QT_TRANSLATE_NOOP("freicoin", ""
"The transaction amount is too small to send after the fee has been deducted"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is a pre-release test build - use at your own risk - do not use for "
"mining or merchant applications"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is the transaction fee you may discard if change is smaller than dust "
"at this level"),
QT_TRANSLATE_NOOP("freicoin", ""
"This is the transaction fee you may pay when fee estimates are not available."),
QT_TRANSLATE_NOOP("freicoin", ""
"Total length of network version string (%i) exceeds maximum length (%i). "
"Reduce the number or size of uacomments."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unable to replay blocks. You will need to rebuild the database using -"
"reindex-chainstate."),
QT_TRANSLATE_NOOP("freicoin", ""
"Unable to rewind the database to a pre-fork state. You will need to "
"redownload the blockchain"),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: Private keys detected in wallet {%s} with disabled private keys"),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: The network does not appear to fully agree! Some miners appear to "
"be experiencing issues."),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: Wallet file corrupt, data salvaged! Original %s saved as %s in %s; "
"if your balance or transactions are incorrect you should restore from a "
"backup."),
QT_TRANSLATE_NOOP("freicoin", ""
"Warning: We do not appear to fully agree with our peers! You may need to "
"upgrade, or other nodes may need to upgrade."),
QT_TRANSLATE_NOOP("freicoin", ""
"You need to rebuild the coin database and block-undo records using -reindex "
"when upgrading from a pre-v15 chainstate database.  If pruning is enabled, "
"this will redownload the entire blockchain."),
QT_TRANSLATE_NOOP("freicoin", ""
"You need to rebuild the database using -reindex to go back to unpruned "
"mode.  This will redownload the entire blockchain"),
QT_TRANSLATE_NOOP("freicoin", "%d of last 100 blocks have unexpected version"),
QT_TRANSLATE_NOOP("freicoin", "%s corrupt, salvage failed"),
QT_TRANSLATE_NOOP("freicoin", "%s is set very high!"),
QT_TRANSLATE_NOOP("freicoin", "-maxmempool must be at least %d MB"),
QT_TRANSLATE_NOOP("freicoin", "Cannot downgrade wallet"),
QT_TRANSLATE_NOOP("freicoin", "Cannot resolve -%s address: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Cannot write to data directory '%s'; check permissions."),
QT_TRANSLATE_NOOP("freicoin", "Change index out of range"),
QT_TRANSLATE_NOOP("freicoin", "Config setting for %s only applied on %s network when in [%s] section."),
QT_TRANSLATE_NOOP("freicoin", "Copyright (C) %i-%i"),
QT_TRANSLATE_NOOP("freicoin", "Corrupted block database detected"),
QT_TRANSLATE_NOOP("freicoin", "Could not find asmap file %s"),
QT_TRANSLATE_NOOP("freicoin", "Could not parse asmap file %s"),
QT_TRANSLATE_NOOP("freicoin", "Database corruption likely.  Try restarting with `-reindex=1`."),
QT_TRANSLATE_NOOP("freicoin", "Do you want to rebuild the block database now?"),
QT_TRANSLATE_NOOP("freicoin", "Done loading"),
QT_TRANSLATE_NOOP("freicoin", "Error initializing block database"),
QT_TRANSLATE_NOOP("freicoin", "Error initializing wallet database environment %s!"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Private keys can only be disabled during creation"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Wallet corrupted"),
QT_TRANSLATE_NOOP("freicoin", "Error loading %s: Wallet requires newer version of %s"),
QT_TRANSLATE_NOOP("freicoin", "Error loading block database"),
QT_TRANSLATE_NOOP("freicoin", "Error loading wallet %s. Duplicate -wallet filename specified."),
QT_TRANSLATE_NOOP("freicoin", "Error opening block database"),
QT_TRANSLATE_NOOP("freicoin", "Error reading from database, shutting down."),
QT_TRANSLATE_NOOP("freicoin", "Error upgrading chainstate database"),
QT_TRANSLATE_NOOP("freicoin", "Error: A fatal internal error occurred, see debug.log for details"),
QT_TRANSLATE_NOOP("freicoin", "Error: Disk space is low for %s"),
QT_TRANSLATE_NOOP("freicoin", "Error: Disk space is too low!"),
QT_TRANSLATE_NOOP("freicoin", "Failed to listen on any port. Use -listen=0 if you want this."),
QT_TRANSLATE_NOOP("freicoin", "Failed to rescan the wallet during initialization"),
QT_TRANSLATE_NOOP("freicoin", "Importing..."),
QT_TRANSLATE_NOOP("freicoin", "Incorrect or no genesis block found. Wrong datadir for network?"),
QT_TRANSLATE_NOOP("freicoin", "Initialization sanity check failed. %s is shutting down."),
QT_TRANSLATE_NOOP("freicoin", "Insufficient funds"),
QT_TRANSLATE_NOOP("freicoin", "Invalid -onion address or hostname: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid -proxy address or hostname: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid P2P permission: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for -%s=<amount>: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for -discardfee=<amount>: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for -fallbackfee=<amount>: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
QT_TRANSLATE_NOOP("freicoin", "Invalid netmask specified in -whitelist: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Loading P2P addresses..."),
QT_TRANSLATE_NOOP("freicoin", "Loading banlist..."),
QT_TRANSLATE_NOOP("freicoin", "Loading block index..."),
QT_TRANSLATE_NOOP("freicoin", "Loading wallet..."),
QT_TRANSLATE_NOOP("freicoin", "Need to specify a port with -whitebind: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Not enough file descriptors available."),
QT_TRANSLATE_NOOP("freicoin", "Prune cannot be configured with a negative value."),
QT_TRANSLATE_NOOP("freicoin", "Prune mode is incompatible with -blockfilterindex."),
QT_TRANSLATE_NOOP("freicoin", "Prune mode is incompatible with -txindex."),
QT_TRANSLATE_NOOP("freicoin", "Pruning blockstore..."),
QT_TRANSLATE_NOOP("freicoin", "Reducing -maxconnections from %d to %d, because of system limitations."),
QT_TRANSLATE_NOOP("freicoin", "Replaying blocks..."),
QT_TRANSLATE_NOOP("freicoin", "Rescanning..."),
QT_TRANSLATE_NOOP("freicoin", "Rewinding blocks..."),
QT_TRANSLATE_NOOP("freicoin", "Section [%s] is not recognized."),
QT_TRANSLATE_NOOP("freicoin", "Signing transaction failed"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" does not exist"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" is a relative path"),
QT_TRANSLATE_NOOP("freicoin", "Specified -walletdir \"%s\" is not a directory"),
QT_TRANSLATE_NOOP("freicoin", "Specified blocks directory \"%s\" does not exist."),
QT_TRANSLATE_NOOP("freicoin", "Starting network threads..."),
QT_TRANSLATE_NOOP("freicoin", "The source code is available from %s."),
QT_TRANSLATE_NOOP("freicoin", "The specified config file %s does not exist\n"),
QT_TRANSLATE_NOOP("freicoin", "The transaction amount is too small to pay the fee"),
QT_TRANSLATE_NOOP("freicoin", "The wallet will avoid paying less than the minimum relay fee."),
QT_TRANSLATE_NOOP("freicoin", "This is experimental software."),
QT_TRANSLATE_NOOP("freicoin", "This is the minimum transaction fee you pay on every transaction."),
QT_TRANSLATE_NOOP("freicoin", "This is the transaction fee you will pay if you send a transaction."),
QT_TRANSLATE_NOOP("freicoin", "Transaction amount too small"),
QT_TRANSLATE_NOOP("freicoin", "Transaction amounts must not be negative"),
QT_TRANSLATE_NOOP("freicoin", "Transaction fee and change calculation failed"),
QT_TRANSLATE_NOOP("freicoin", "Transaction has too long of a mempool chain"),
QT_TRANSLATE_NOOP("freicoin", "Transaction must have at least one recipient"),
QT_TRANSLATE_NOOP("freicoin", "Transaction too large"),
QT_TRANSLATE_NOOP("freicoin", "Unable to bind to %s on this computer (bind returned error %s)"),
QT_TRANSLATE_NOOP("freicoin", "Unable to bind to %s on this computer. %s is probably already running."),
QT_TRANSLATE_NOOP("freicoin", "Unable to create the PID file '%s': %s"),
QT_TRANSLATE_NOOP("freicoin", "Unable to generate initial keys"),
QT_TRANSLATE_NOOP("freicoin", "Unable to generate keys"),
QT_TRANSLATE_NOOP("freicoin", "Unable to start HTTP server. See debug log for details."),
QT_TRANSLATE_NOOP("freicoin", "Unknown -blockfilterindex value %s."),
QT_TRANSLATE_NOOP("freicoin", "Unknown address type '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unknown change type '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unknown network specified in -onlynet: '%s'"),
QT_TRANSLATE_NOOP("freicoin", "Unsupported logging category %s=%s."),
QT_TRANSLATE_NOOP("freicoin", "Upgrading UTXO database"),
QT_TRANSLATE_NOOP("freicoin", "Upgrading txindex database"),
QT_TRANSLATE_NOOP("freicoin", "User Agent comment (%s) contains unsafe characters."),
QT_TRANSLATE_NOOP("freicoin", "Verifying blocks..."),
QT_TRANSLATE_NOOP("freicoin", "Verifying wallet(s)..."),
QT_TRANSLATE_NOOP("freicoin", "Wallet needed to be rewritten: restart %s to complete"),
QT_TRANSLATE_NOOP("freicoin", "Warning: unknown new rules activated (versionbit %i)"),
QT_TRANSLATE_NOOP("freicoin", "Zapping all transactions from wallet..."),
};
