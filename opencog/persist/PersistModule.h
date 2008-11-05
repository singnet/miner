/*
 * opencog/persist/PersistModule.h
 *
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * Written by Gustavo Gama <gama@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_PERSIST_MODULE_H
#define _OPENCOG_PERSIST_MODULE_H

#include <vector>
#include <string>

#include "AtomStorage.h"
#include <opencog/server/CogServer.h>
#include <opencog/server/Factory.h>
#include <opencog/server/Module.h>
#include <opencog/server/Request.h>

namespace opencog
{

class PersistModule : public Module
{
private:
    AtomStorage* store;

    DECLARE_CMD_REQUEST(PersistModule, "sql-close", do_close, 
             "Close the SQL database", 
             "Close the currently open SQL database")

    DECLARE_CMD_REQUEST(PersistModule, "sql-load", do_load,
            "Load contents of SQL database",
            "Load the contents of the SQL database to the atomtable")

    DECLARE_CMD_REQUEST(PersistModule, "sql-open", do_open,
            "Open connection to SQL storage",
            "Usage: sqlopen <dbname> <username> <auth>")

    DECLARE_CMD_REQUEST(PersistModule, "sql-store", do_store,
            "Save the atomtable on the SQL database",
            "Save the contents of the atomtable on the SQL database")

public:
    const char* id(void);

    PersistModule(void);
    virtual ~PersistModule();

    virtual void init(void);

}; // class

}  // namespace

#endif // _OPENCOG_PERSIST_MODULE_H
