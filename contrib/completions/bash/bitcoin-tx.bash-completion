# bash programmable completion for bitcoin-tx(1)
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Copyright (c) 2010-2023 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of version 3 of the GNU Affero General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

_bitcoin_tx() {
    local cur prev words=() cword
    local bitcoin_tx

    # save and use original argument to invoke bitcoin-tx for -help
    # it might not be in $PATH
    bitcoin_tx="$1"

    COMPREPLY=()
    _get_comp_words_by_ref -n =: cur prev words cword

    case "$cur" in
        load=*:*)
            cur="${cur#load=*:}"
            _filedir
            return 0
            ;;
        *=*)	# prevent attempts to complete other arguments
            return 0
            ;;
    esac

    if [[ "$cword" == 1 || ( "$prev" != "-create" && "$prev" == -* ) ]]; then
        # only options (or an uncompletable hex-string) allowed
        # parse bitcoin-tx -help for options
        local helpopts
        helpopts=$($bitcoin_tx -help | sed -e '/^  -/ p' -e d )
        COMPREPLY=( $( compgen -W "$helpopts" -- "$cur" ) )
    else
        # only commands are allowed
        # parse -help for commands
        local helpcmds
        helpcmds=$($bitcoin_tx -help | sed -e '1,/Commands:/d' -e 's/=.*/=/' -e '/^  [a-z]/ p' -e d )
        COMPREPLY=( $( compgen -W "$helpcmds" -- "$cur" ) )
    fi

    # Prevent space if an argument is desired
    if [[ $COMPREPLY == *= ]]; then
        compopt -o nospace
    fi

    return 0
} &&
complete -F _bitcoin_tx bitcoin-tx

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
# ex: ts=4 sw=4 et filetype=sh
