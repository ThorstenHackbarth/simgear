// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2017  James Turner - james@flightgear.org


#include "simgear/sg_inlines.h"
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>

namespace simgear
{

	namespace pkg
	{

		void Delegate::installStatusChanged(InstallRef aInstall, StatusCode aReason)
		{
		}

        void Delegate::dataForThumbnail(const std::string& aPackage,
                                        size_t length, const uint8_t* bytes)
        {
            SG_UNUSED(aPackage);
            SG_UNUSED(length);
            SG_UNUSED(bytes);
        }

	} // of namespace pkg
} // of namespace simgear
