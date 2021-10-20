Wallet changes
--------------
When creating a transaction with a fee above `-maxtxfee` (default 0.1 FRC),
the RPC commands `walletcreatefundedpst` and  `fundrawtransaction` will now fail
instead of rounding down the fee. Beware that the `feeRate` argument is specified
in FRC per kilobyte, not kria per byte.
