#!/bin/bash -eux

#
# Copyright © 2017-2019 The Freicoin developers.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of version 3 of the GNU Affero General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public
# License along with this program.  If not, see
# <https://www.gnu.org/licenses/>.
#

ESSENTIAL_PACKAGES="
ntp
nfs-common
"

GITIAN_PACKAGES="
git
apache2
apt-cacher-ng
bridge-utils
python-vm-builder
ruby
qemu-utils
lxc
"

BUILD_PACKAGES="
curl
"

export DEBIAN_FRONTEND=noninteractive

# Perform ALL security updates for the guest VM distribution.
echo "==> Updating distribution-provided package lists"
apt-get -y update
echo "==> Applying security updates & upgrading default packages"
apt-get -y dist-upgrade

# Essential packages are necessary for virtualbox/vagrant integration.
echo "==> Installing packages necessary for virtualbox/vagrant integration"
apt-get -y install $ESSENTIAL_PACKAGES

# Gitian packages are necessary for gitian-builder.
echo "==> Installing packages necessary for gitian-builder"
apt-get -y install $GITIAN_PACKAGES

# Packages required for the various build steps required to run in the host environment.
echo "==> Installing packages necessary for build steps in host environment"
apt-get -y install $BUILD_PACKAGES

#
# End of File
#
