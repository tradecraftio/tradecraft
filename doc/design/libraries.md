# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libfreicoin_cli*         | RPC client functionality used by *freicoin-cli* executable |
| *libfreicoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libfreicoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libfreicoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libfreicoin_node* and *libfreicoin_wallet* and also exposed as a [shared library](../shared-libraries.md). |
| *libfreicoinconsensus*    | Shared library build of static *libfreicoin_consensus* library |
| *libfreicoin_kernel*      | Consensus engine and support library used for validation by *libfreicoin_node* and also exposed as a [shared library](../shared-libraries.md). |
| *libfreicoinqt*           | GUI functionality used by *freicoin-qt* and *freicoin-gui* executables |
| *libfreicoin_ipc*         | IPC functionality used by *freicoin-node*, *freicoin-wallet*, *freicoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libfreicoin_node*        | P2P and RPC server functionality used by *freicoind* and *freicoin-qt* executables. |
| *libfreicoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libfreicoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libfreicoin_wallet*      | Wallet functionality used by *freicoind* and *freicoin-wallet* executables. |
| *libfreicoin_wallet_tool* | Lower-level wallet functionality used by *freicoin-wallet* executable. |
| *libfreicoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *freicoind* and *freicoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. Exceptions are *libfreicoin_consensus* and *libfreicoin_kernel* which have external interfaces documented at [../shared-libraries.md](../shared-libraries.md).

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libfreicoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libfreicoin_node* code lives in `src/node/` in the `node::` namespace
  - *libfreicoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libfreicoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libfreicoin_util* code lives in `src/util/` in the `util::` namespace
  - *libfreicoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "linear" }}}%%

graph TD;

freicoin-cli[freicoin-cli]-->libfreicoin_cli;

freicoind[freicoind]-->libfreicoin_node;
freicoind[freicoind]-->libfreicoin_wallet;

freicoin-qt[freicoin-qt]-->libfreicoin_node;
freicoin-qt[freicoin-qt]-->libfreicoinqt;
freicoin-qt[freicoin-qt]-->libfreicoin_wallet;

freicoin-wallet[freicoin-wallet]-->libfreicoin_wallet;
freicoin-wallet[freicoin-wallet]-->libfreicoin_wallet_tool;

libfreicoin_cli-->libfreicoin_common;
libfreicoin_cli-->libfreicoin_util;

libfreicoin_common-->libfreicoin_util;
libfreicoin_common-->libfreicoin_consensus;

libfreicoin_kernel-->libfreicoin_consensus;
libfreicoin_kernel-->libfreicoin_util;

libfreicoin_node-->libfreicoin_common;
libfreicoin_node-->libfreicoin_consensus;
libfreicoin_node-->libfreicoin_kernel;
libfreicoin_node-->libfreicoin_util;

libfreicoinqt-->libfreicoin_common;
libfreicoinqt-->libfreicoin_util;

libfreicoin_wallet-->libfreicoin_common;
libfreicoin_wallet-->libfreicoin_util;

libfreicoin_wallet_tool-->libfreicoin_util;
libfreicoin_wallet_tool-->libfreicoin_wallet;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class freicoin-qt,freicoind,freicoin-cli,freicoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libfreicoin_wallet* and *libfreicoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code still is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libfreicoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libfreicoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libfreicoin_common* should serve a similar function as *libfreicoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libfreicoin_util* and *libfreicoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for freicoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libfreicoin_kernel* is not supposed to depend on *libfreicoin_common*, only *libfreicoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libfreicoin_kernel* should only depend on *libfreicoin_util* and *libfreicoin_consensus*.

- The only thing that should depend on *libfreicoin_kernel* internally should be *libfreicoin_node*. GUI and wallet libraries *libfreicoinqt* and *libfreicoin_wallet* in particular should not depend on *libfreicoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libfreicoin_consensus*, *libfreicoin_common*, and *libfreicoin_util*, instead of *libfreicoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libfreicoinqt*, *libfreicoin_node*, *libfreicoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](`../../src/interfaces/`) abstract interfaces.

## Work in progress

- Validation code is moving from *libfreicoin_node* to *libfreicoin_kernel* as part of [The libfreicoinkernel Project #24303](https://github.com/bitcoin/bitcoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/bitcoin/bitcoin/issues/15732)
